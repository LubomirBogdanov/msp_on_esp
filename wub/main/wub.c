/*
 * wub -> wifi_uart_bridge.h
 *
 *  Created on: May 21, 2019
 *      Author: lbogdanov
 */
#include "wub.h"

TaskHandle_t wub_cmd_task_h;
TaskHandle_t wub_uart_cmd_task_h;
TaskHandle_t wub_server_task_h;
TaskHandle_t wub_gpio_transparent_task_h;
EventGroupHandle_t wifi_event_group;
const int ipv4_gotip_bit = BIT0;
QueueHandle_t uart0_queue;
wub_config_t wub_conf;
uart_config_t uart_conf;
int client_socket;
int server_socket;

static esp_err_t event_handler(void *ctx, system_event_t *event){
	char ip_buff[32];

	switch(event->event_id) {
	case SYSTEM_EVENT_STA_START:
		esp_wifi_connect();
		DEBUGOUT("SYSTEM_EVENT_STA_START\n");
		break;
	case SYSTEM_EVENT_STA_GOT_IP:
		sprintf(ip_buff, "IP: %s\n\r", ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
		uart_write_bytes(UART_NUM_0, (const char *) ip_buff, strlen((const char *) ip_buff));
		sprintf(ip_buff, "PORT: %04d\n\r", wub_conf.tcp_listen_port);
		uart_write_bytes(UART_NUM_0, (const char *) ip_buff, strlen((const char *) ip_buff));
		wub_conf.wifi_connected = 1;
		xEventGroupSetBits(wifi_event_group, ipv4_gotip_bit);
		DEBUGOUT("SYSTEM_EVENT_STA_GOT_IP\n");
		break;
	case SYSTEM_EVENT_STA_DISCONNECTED:
		if(wub_conf.wifi_connected){
			uart_write_bytes(UART_NUM_0, (const char *) CMD_WIFI_DISCONNECTED_STR, strlen((const char *) CMD_WIFI_DISCONNECTED_STR));
			wub_conf.wifi_connected = 0;
		}
		esp_wifi_connect();
		xEventGroupClearBits(wifi_event_group, ipv4_gotip_bit);
		DEBUGOUT("SYSTEM_EVENT_STA_DISCONNECTED\n");
		break;
	case SYSTEM_EVENT_STA_CONNECTED:
		uart_write_bytes(UART_NUM_0, (const char *) CMD_CONNECTED_STR, strlen((const char *) CMD_CONNECTED_STR));
		DEBUGOUT("SYSTEM_EVENT_STA_CONNECTED\n");
		break;
	case SYSTEM_EVENT_STA_STOP:
		DEBUGOUT("SYSTEM_EVENT_STA_STOP\n");
		break;
	default:
		uart_write_bytes(UART_NUM_0, (const char *) CMD_WIFI_UNKNOWN_ERROR_STR, strlen((const char *) CMD_WIFI_UNKNOWN_ERROR_STR));
		DEBUGOUT("UNHANDLED_EVENT (%d)\n", event->event_id);
		break;
	}

	return ESP_OK;
}

void wifi_init_sta(void){
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

	wifi_deinit_sta();

	tcpip_adapter_init();
	esp_event_loop_init(event_handler, NULL);
	esp_wifi_init(&cfg);
	esp_wifi_set_mode(WIFI_MODE_STA);
	esp_wifi_set_config(ESP_IF_WIFI_STA, &wub_conf.wifi_config);
	esp_wifi_start();

	xTaskCreate(wub_wifi_server_task, "wub_server_task", 8192, NULL, WUB_WIFI_TASK_PRIORITY, &wub_server_task_h);

	DEBUGOUT("Trying to connect\n\r");
	DEBUGOUT("ssid: %s\n\r", wub_conf.wifi_config.sta.ssid);
	DEBUGOUT("pass: %s\n\r", wub_conf.wifi_config.sta.password);
}

void wifi_deinit_sta(void){
	if(client_socket){
		shutdown(client_socket, 0);
		close(client_socket);
		wub_conf.transparent_mode = 0;
		client_socket = 0;
	}

	if(server_socket){
		shutdown(server_socket, 0);
		close(server_socket);
		server_socket = 0;
	}

	if(wub_server_task_h){
		vTaskDelete(wub_server_task_h);
		wub_server_task_h = NULL;
	}

	esp_wifi_stop();

	//Nothing here works to turn off radio
	//Good luck!
	//esp_wifi_set_mode(WIFI_MODE_NULL);
	//esp_wifi_deinit();
	//wifi_fpm_set_sleep_type(WIFI_MODEM_SLEEP_T);
	//wifi_fpm_open();
	//wifi_fpm_do_sleep(0xfffffff);
	//tcpip_adapter_stop(TCPIP_ADAPTER_IF_STA);
	//esp_wifi_disconnect();
	//wifi_power_save();

	vTaskDelay(100);
}

void flush_wifi_cmd_buff(void){
	wub_conf.cmd_buffer_len = 0;
	memset(wub_conf.cmd_buffer, 0x00, WUB_CMD_BUFF_SIZE);
}

void flush_uart_cmd_buff(void){
	wub_conf.uart_rx_buffer_len = 0;
	memset(wub_conf.uart_rx_buffer, 0x00, WUB_UART_RX_BUFF_SIZE);
}

void flush_wifi_rx_buff(void){
	wub_conf.wifi_rx_buffer_len = 0;
	memset(wub_conf.wifi_rx_buffer, 0x00, WUB_WIFI_RX_BUFF_SIZE);
}

int16_t find_terminator_index(char *str){
	int16_t i = 0;

	while((*str != '\n') && (*str != '\r') && (*str != '\0')){
		str++;
		i++;
	}

	if(*str == '\0'){
		i = -1;
	}

	return i;
}

void display_help(void){
	send(client_socket, (const char *)HELP_STR, strlen(HELP_STR), 0);
}

void display_help_uart(void){
	uart_write_bytes(UART_NUM_0, (const char *)HELP_STR, strlen((const char *) HELP_STR));
}

uint16_t get_idn_string(char *idn_str){
	uint32_t chip_id_int_lsb;
	uint32_t chip_id_int_msb;
	uint8_t chip_id[8];
	memset(chip_id, 0x00, 8);
	esp_wifi_get_mac(ESP_IF_WIFI_STA, chip_id);
	chip_id_int_msb = (chip_id[5] << 8);
	chip_id_int_msb |= chip_id[4];
	chip_id_int_lsb = (chip_id[3] << 24);
	chip_id_int_lsb |= (chip_id[2] << 16);
	chip_id_int_lsb |= (chip_id[1] << 8);
	chip_id_int_lsb |= chip_id[0];
	sprintf(idn_str, "%s,%s,%08x%08x,HW%d.%d,SW%d.%d\n\r", IDN_MANUFACTURER_STR, IDN_MODEL_STR, chip_id_int_msb, chip_id_int_lsb,
			WUB_HW_REVISION, WUB_HW_SUBREVISION, WUB_SW_REVISION, WUB_SW_SUBREVISION);

	return strlen(idn_str);
}

void display_idn(void){
	char idn_str[128];
	uint16_t idn_str_size;
	idn_str_size = get_idn_string(idn_str);
	send(client_socket, (const char *)idn_str, idn_str_size, 0);
}

void display_idn_uart(void){
	char idn_str[128];
	uint16_t idn_str_size;
	idn_str_size = get_idn_string(idn_str);
	uart_write_bytes(UART_NUM_0, (const char *)idn_str, idn_str_size);
}

void display_ready(void){
	uart_write_bytes(UART_NUM_0, (const char *)CMD_UART_READY_STR, strlen(CMD_UART_READY_STR));
}

void pin_init_as_inputs(void){
	gpio_config_t io_conf;

	io_conf.intr_type = GPIO_INTR_DISABLE;
	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pin_bit_mask = (GPIO_PIN_0 | GPIO_PIN_2);
	io_conf.pull_down_en = 0;
	io_conf.pull_up_en = 0;
	gpio_config(&io_conf);
}

void pin_init_as_outputs(void){
	gpio_config_t io_conf;

	io_conf.intr_type = GPIO_INTR_DISABLE;
	io_conf.mode = GPIO_MODE_OUTPUT;
	io_conf.pin_bit_mask = (GPIO_PIN_0 | GPIO_PIN_2);
	io_conf.pull_down_en = 0;
	io_conf.pull_up_en = 0;
	gpio_config(&io_conf);

	pin_rst_set(0);
	pin_boot_set(0);
}

void pin_rst_set(uint8_t value){
	if(value){
		gpio_set_level(GPIO_PIN_0_, 1);
	}
	else{
		gpio_set_level(GPIO_PIN_0_, 0);
	}
}

void pin_boot_set(uint8_t value){
	if(value){
		gpio_set_level(GPIO_PIN_2_, 1);
	}
	else{
		gpio_set_level(GPIO_PIN_2_, 0);
	}
}

void reset_target(void){
	DEBUGOUT("reset_target\n\r");

	pin_init_as_outputs();

	//Different than what is described
	//in the user guide, only this way
	//works for MSP430F5529
	pin_boot_set(0);
	pin_rst_set(0);
	vTaskDelay(WUB_DEFAULT_GPIO_DELAY);
	pin_rst_set(1);
	vTaskDelay(WUB_DEFAULT_GPIO_DELAY);
	pin_boot_set(1);
	vTaskDelay(WUB_DEFAULT_GPIO_DELAY);
	pin_boot_set(0);
	pin_rst_set(0);
	vTaskDelay(WUB_RESET_DELAY);

	pin_init_as_inputs();
}

void halt_target(void){
	DEBUGOUT("halt_target\n\r");

	pin_init_as_outputs();

	//Hold target in reset
	pin_boot_set(0);
	pin_rst_set(0);
}

void enter_boot(void){
	pin_init_as_outputs();

	vTaskDelay(WUB_DEFAULT_GPIO_DELAY);
	vTaskDelay(WUB_DEFAULT_GPIO_DELAY);
	vTaskDelay(WUB_DEFAULT_GPIO_DELAY);

	switch(wub_conf.boot_mode){
	case BSL_UART_DEDICATED_JTAG:
		pin_boot_set(1);
		vTaskDelay(WUB_DEFAULT_GPIO_DELAY);
		vTaskDelay(WUB_DEFAULT_GPIO_DELAY);
		vTaskDelay(WUB_DEFAULT_GPIO_DELAY);
		pin_boot_set(0);
		vTaskDelay(WUB_DEFAULT_GPIO_DELAY);
		pin_boot_set(1);
		vTaskDelay(WUB_DEFAULT_GPIO_DELAY);
		vTaskDelay(WUB_DEFAULT_GPIO_DELAY);
		pin_boot_set(0);
		vTaskDelay(WUB_DEFAULT_GPIO_DELAY);
		pin_rst_set(1);
		vTaskDelay(WUB_DEFAULT_GPIO_DELAY);
		vTaskDelay(WUB_DEFAULT_GPIO_DELAY);
		break;
	case BSL_UART_SHARED_JTAG_SBW:
		pin_boot_set(1);
		vTaskDelay(WUB_DEFAULT_GPIO_DELAY);
		pin_boot_set(0);
		vTaskDelay(WUB_DEFAULT_GPIO_DELAY);
		pin_boot_set(1);
		vTaskDelay(WUB_DEFAULT_GPIO_DELAY);
		pin_rst_set(1);
		vTaskDelay(WUB_DEFAULT_GPIO_DELAY);
		pin_boot_set(0);
		vTaskDelay(WUB_DEFAULT_GPIO_DELAY);
		break;
	}
}

void uart_init(uart_config_t *uart_config){
	uart_param_config(UART_NUM_0, uart_config);
	uart_driver_install(UART_NUM_0, WUB_UART_RX_BUFF_SIZE, 0, 32, &uart0_queue);
	uart_conf = *uart_config;
}

void uart_apply_config(void){
	uart_param_config(UART_NUM_0, &uart_conf);
}

void init_defaults_params(void){
	client_socket = 0;
	server_socket = 0;
	wub_conf.transparent_mode = 0;
	wub_conf.boot_mode = BSL_UART_SHARED_JTAG_SBW;
	wub_conf.transparent_timeout = WUB_DEFAULT_TRANSP_TIMEOUT;
	wub_conf.wifi_connected = 0;
}

void wifi_init_defaults(void){
	strcpy((char *)wub_conf.wifi_config.sta.ssid, WUB_DEFAULT_WIFI_SSID);
	strcpy((char *)wub_conf.wifi_config.sta.password, WUB_DEFAULT_WIFI_PASS);
	wub_conf.tcp_listen_port = WUB_DEFAULT_WIFI_SERVER_PORT;
}

wub_cmd_e wub_wifi_cmd_parse(uint32_t *param_numeric, char *param_string){
	int16_t cmd_terminator_index;
	wub_cmd_e cmd_converted = CMD_NONE;
	char *ptr;
	int field_counter = 0;
	uint8_t terminate_parse = 0;
	char cmd_buff[WUB_CMD_MAX_LEN];
	char param_buff[WUB_PARAM_MAX_LEN];

	cmd_terminator_index = find_terminator_index(wub_conf.wifi_rx_buffer);

	if(cmd_terminator_index >= 0){
		//Received a full message or end of partial message. Accumulate and
		//process it.
		wub_conf.cmd_buffer_len += strlen(wub_conf.wifi_rx_buffer);
		if(wub_conf.cmd_buffer_len >= WUB_CMD_BUFF_SIZE){
			DEBUGOUT("wub error: command too long!\n\r");
			flush_wifi_cmd_buff();
			goto err;
		}
		else{
			wub_conf.wifi_rx_buffer[cmd_terminator_index] = '\0';
			strcat(wub_conf.cmd_buffer, wub_conf.wifi_rx_buffer);
		}

		ptr = strtok(wub_conf.cmd_buffer, " ");

		while (ptr != NULL){
			switch(field_counter){
			case 0:
				strcpy(cmd_buff, ptr);
				break;
			case 1:
				strcpy(param_buff, ptr);
				break;
			case 2:
				terminate_parse = 1;
				break;
			}

			field_counter++;

			if(terminate_parse){
				break;
			}

			ptr = strtok (NULL, " ");
		}

		if(!strcmp(cmd_buff, CMD_TRANSPARENT_STR)){
			if(!strcmp(param_buff, CMD_PARAM_ON_STR)){
				cmd_converted = CMD_TRANSPARENT_ON;
			}
		}
		else if(!strcmp(cmd_buff, CMD_SET_BSL_TYPE_STR)){
			if(!strcmp(param_buff, CMD_PARAM_SHARED_STR)){
				cmd_converted = CMD_SET_BSL_SHARED;
			}
			else if(!strcmp(param_buff, CMD_PARAM_DEDICATED_STR)){
				cmd_converted = CMD_SET_BSL_DEDICATED;
			}
		}
		else if(!strcmp(cmd_buff, CMD_ENTER_BSL_STR)){
			cmd_converted = CMD_ENTER_BSL;
		}
		else if(!strcmp(cmd_buff, CMD_SET_TRANSPARENT_TIMEOUT_STR)){
			cmd_converted = CMD_SET_TIMEOUT;
			*param_numeric = atoi(param_buff);
		}
		else if(!strcmp(cmd_buff, CMD_HELP_STR)){
			cmd_converted = CMD_HELP;
		}
		else if(!strcmp(cmd_buff, CMD_HELLO_STR)){
			cmd_converted = CMD_HELLO;
		}
		else if(!strcmp(cmd_buff, CMD_RSTT_STR)){
			cmd_converted = CMD_RSTT;
		}
		else if(!strcmp(cmd_buff, CMD_HALT_STR)){
			cmd_converted = CMD_HALT;
		}
		else if(!strcmp(cmd_buff, CMD_SET_UART_BAUDRATE_STR)){
			cmd_converted = CMD_SET_UART_BAUDRATE;
			*param_numeric = atoi(param_buff);
		}
		else if(!strcmp(cmd_buff, CMD_SET_UART_WORD_STR)){
			cmd_converted = CMD_SET_UART_WORD_LEN;
			*param_numeric = atoi(param_buff);

			switch(*param_numeric){
			case 5:
				*param_numeric = UART_DATA_5_BITS;
				break;
			case 6:
				*param_numeric = UART_DATA_6_BITS;
				break;
			case 7:
				*param_numeric = UART_DATA_7_BITS;
				break;
			case 8:
				*param_numeric = UART_DATA_8_BITS;
				break;
			}
		}
		else if(!strcmp(cmd_buff, CMD_SET_UART_PARITY_STR)){
			cmd_converted = CMD_SET_UART_PARITY;

			if(!strcmp(param_buff, CMD_PARAM_ODD_STR)){
				*param_numeric = UART_PARITY_ODD;
			}
			else if(!strcmp(param_buff, CMD_PARAM_EVEN_STR)){
				*param_numeric = UART_PARITY_EVEN;
			}
			else if(!strcmp(param_buff, CMD_PARAM_DISABLED_STR)){
				*param_numeric = UART_PARITY_DISABLE;
			}
		}
		else if(!strcmp(cmd_buff, CMD_SET_UART_STOP_STR)){
			cmd_converted = CMD_SET_UART_STOP_BIT;

			if(!strcmp(param_buff, CMD_PARAM_ONE_STR)){
				*param_numeric = UART_STOP_BITS_1;
			}
			else if(!strcmp(param_buff, CMD_PARAM_ONE_AND_HALF_STR)){
				*param_numeric = UART_STOP_BITS_1_5;
			}
			else if(!strcmp(param_buff, CMD_PARAM_TWO_STR)){
				*param_numeric = UART_STOP_BITS_2;
			}
		}
		else if(!strcmp(cmd_buff, CMD_UART_APPLY_CONFIG_STR)){
			cmd_converted = CMD_UART_APPLY_CONFIG;
		}
		else if(!strcmp(cmd_buff, CMD_RESTART_WUB_STR)){
			cmd_converted = CMD_RESTART_WUB;
		}
		else if(!strcmp(cmd_buff, CMD_WIFI_SET_SSID_STR)){
			cmd_converted = CMD_SET_SSID;
			strcpy(param_string, param_buff);
		}
		else if(!strcmp(cmd_buff, CMD_WIFI_SET_PASS_STR)){
			cmd_converted = CMD_SET_SSID_PASS;
			strcpy(param_string, param_buff);
		}
		else if(!strcmp(cmd_buff, CMD_WIFI_SET_PORT_STR)){
			cmd_converted = CMD_SET_SOCK_PORT;
			*param_numeric = atoi(param_buff);;
		}
		else if(!strcmp(cmd_buff, CMD_WIFI_START_STR)){
			cmd_converted = CMD_START_WIFI;
		}
		else if(!strcmp(cmd_buff, CMD_WIFI_IDN_STR)){
			cmd_converted = CMD_IDN;
		}

		flush_wifi_cmd_buff();
	}
	else{
		//Received a partial message. Accumulate it.
		wub_conf.cmd_buffer_len += strlen(wub_conf.wifi_rx_buffer);
		if(wub_conf.cmd_buffer_len >= WUB_CMD_BUFF_SIZE){
			DEBUGOUT("wub error: command too long!\n\r");
			flush_wifi_cmd_buff();
		}
		else{
			strcat(wub_conf.cmd_buffer, wub_conf.wifi_rx_buffer);
		}
	}

err:
	return cmd_converted;
}

void pin_init_transparent(uint8_t pin_number){
	gpio_config_t io_conf;
	io_conf.intr_type = GPIO_INTR_NEGEDGE;
    io_conf.pin_bit_mask = (1 << pin_number);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = 1;
    io_conf.pull_down_en = 0;
    gpio_config(&io_conf);

    if(wub_gpio_transparent_task_h){
    	vTaskDelete(wub_gpio_transparent_task_h);
    	wub_gpio_transparent_task_h = NULL;
    }

    xTaskCreate(wub_gpio_transparent_task, "wub_gpio_transparent_task", 2048, NULL, WUB_GPIO_TRANSPARENT_PRIORITY, &wub_gpio_transparent_task_h);

    DEBUGOUT("pin_init_transparent %d\n\r", pin_number);
}

void pin_deinit_transparent(void){
    if(wub_gpio_transparent_task_h){
    	vTaskDelete(wub_gpio_transparent_task_h);
    	wub_gpio_transparent_task_h = NULL;
    }
}

//Can't get interrupts to work, so
//I'll use polling
void wub_gpio_transparent_task(void *arg){
    uint8_t pin_level_new = 0;
    uint8_t pin_level_old = 0;

    DEBUGOUT("wub_gpio_transparent_task\n\r");

    while(1) {
    	pin_level_new = gpio_get_level(wub_conf.gpio_transparent);
    	if(pin_level_new != pin_level_old){
    		pin_level_old = pin_level_new;

    		if(pin_level_new){
    			DEBUGOUT("1\n\r");
    			wub_conf.transparent_mode = 1;
    		}
    		else{
    			DEBUGOUT("0\n\r");
    			wub_conf.transparent_mode = 0;
    		}

    		vTaskDelay(WUB_GPIO_TRANSPARENT_DEBOUNCE);
    	}
    	vTaskDelay(1);
    }

    wub_gpio_transparent_task_h = NULL;
    vTaskDelete(NULL);
}

void wub_uart_rx_event_task(void *pvParameters){
	uart_event_t event;
	DEBUGOUT("wub_uart_event_task\n\r");

	while(1) {
		if (xQueueReceive(uart0_queue, (void *)&event, (portTickType)portMAX_DELAY)) {
			flush_uart_cmd_buff();

			switch (event.type) {
			case UART_DATA:
				wub_conf.uart_rx_buffer_len = event.size;
				uart_read_bytes(UART_NUM_0, wub_conf.uart_rx_buffer, wub_conf.uart_rx_buffer_len, portMAX_DELAY);

				if(wub_conf.transparent_mode){
					send(client_socket, wub_conf.uart_rx_buffer, wub_conf.uart_rx_buffer_len, 0);
				}
				else{
					xTaskNotifyGive(wub_uart_cmd_task_h);
				}
				break;
			default:
				break;
			}
		}
	}

	vTaskDelete(NULL);
}

void wub_uart_cmd_exec_task(void *pvParameters){
	int16_t cmd_terminator_index;
	int field_counter = 0;
	uint8_t terminate_parse = 0;
	char cmd_buff[WUB_CMD_MAX_LEN];
	char param_buff[WUB_PARAM_MAX_LEN];
	char string_buff[WUB_CMD_MAX_LEN+WUB_PARAM_MAX_LEN+4];
	char *ptr;
	uint8_t uart_data_bits = 0;
	uint8_t uart_parity = 0;
	uint8_t uart_stop_bits = 0;

	DEBUGOUT("wub_uart_cmd_task\n\r");

	while(1) {
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		cmd_terminator_index = find_terminator_index((char *)wub_conf.uart_rx_buffer);

		if(cmd_terminator_index >= 0){
			wub_conf.uart_rx_buffer[cmd_terminator_index] = '\0';

			strcpy(string_buff, (char *)wub_conf.uart_rx_buffer);

			ptr = strtok(string_buff, " ");

			while (ptr != NULL){
				switch(field_counter){
				case 0:
					strcpy(cmd_buff, ptr);
					break;
				case 1:
					strcpy(param_buff, ptr);
					break;
				case 2:
					terminate_parse = 1;
					break;
				}

				field_counter++;

				if(terminate_parse){
					break;
				}

				ptr = strtok (NULL, " ");
			}

			if(!strcmp(cmd_buff, CMD_UART_SET_SSID_STR)){
				strcpy((char *)wub_conf.wifi_config.sta.ssid, param_buff);
				uart_write_bytes(UART_NUM_0, (const char *) CMD_EXEC_DONE_STR, strlen((const char *) CMD_EXEC_DONE_STR));
				DEBUGOUT("wub: new ssid: %s\n\r", wub_conf.wifi_config.sta.ssid);
			}
			else if(!strcmp(cmd_buff, CMD_UART_SET_PASS_STR)){
				strcpy((char *)wub_conf.wifi_config.sta.password, param_buff);
				uart_write_bytes(UART_NUM_0, (const char *) CMD_EXEC_DONE_STR, strlen((const char *) CMD_EXEC_DONE_STR));
				DEBUGOUT("wub: new pass: %s\n\r", wub_conf.wifi_config.sta.password);
			}
			else if(!strcmp(cmd_buff, CMD_UART_SET_PORT_STR)){
				wub_conf.tcp_listen_port = atoi(param_buff);
				uart_write_bytes(UART_NUM_0, (const char *) CMD_EXEC_DONE_STR, strlen((const char *) CMD_EXEC_DONE_STR));
				DEBUGOUT("wub: new port: %d\n\r", wub_conf.tcp_listen_port);
			}
			else if(!strcmp(cmd_buff, CMD_UART_WIFI_START_STR)){
				wifi_init_sta();
				uart_write_bytes(UART_NUM_0, (const char *) CMD_EXEC_DONE_STR, strlen((const char *) CMD_EXEC_DONE_STR));
			}
			else if(!strcmp(cmd_buff, CMD_UART_WIFI_STOP_STR)){
				wifi_deinit_sta();
				uart_write_bytes(UART_NUM_0, (const char *) CMD_EXEC_DONE_STR, strlen((const char *) CMD_EXEC_DONE_STR));
			}
			else if(!strcmp(cmd_buff, CMD_UART_SET_TRAN_STR)){
				wub_conf.gpio_transparent = atoi(param_buff);
				pin_init_transparent(wub_conf.gpio_transparent);
				uart_write_bytes(UART_NUM_0, (const char *) CMD_EXEC_DONE_STR, strlen((const char *) CMD_EXEC_DONE_STR));
			}
			else if(!strcmp(cmd_buff, CMD_UART_SET_TRAN_OFF_STR)){
				pin_deinit_transparent();
				uart_write_bytes(UART_NUM_0, (const char *) CMD_EXEC_DONE_STR, strlen((const char *) CMD_EXEC_DONE_STR));
			}
			else if(!strcmp(cmd_buff, CMD_UART_RESTART_WUB_STR)){
				esp_restart();
			}
			else if(!strcmp(cmd_buff, CMD_UART_IDN_STR)){
				display_idn_uart();
			}
			else if(!strcmp(cmd_buff, CMD_UART_HELP_STR)){
				display_help_uart();
			}
			else if(!strcmp(cmd_buff, CMD_UART_POWER_OFF_WUB_STR)){
				esp_deep_sleep(0);
			}

			else if(!strcmp(cmd_buff, CMD_SET_UART_BAUDRATE_STR)){
				uart_conf.baud_rate = atoi(param_buff);
				uart_write_bytes(UART_NUM_0, (const char *) CMD_EXEC_DONE_STR, strlen((const char *) CMD_EXEC_DONE_STR));
			}
			else if(!strcmp(cmd_buff, CMD_SET_UART_WORD_STR)){
				uart_data_bits = atoi(param_buff);

				switch(uart_data_bits){
				case 5:
					uart_data_bits = UART_DATA_5_BITS;
					break;
				case 6:
					uart_data_bits = UART_DATA_6_BITS;
					break;
				case 7:
					uart_data_bits = UART_DATA_7_BITS;
					break;
				case 8:
					uart_data_bits = UART_DATA_8_BITS;
					break;
				}

				uart_conf.data_bits = (uart_word_length_t) uart_data_bits;
				uart_write_bytes(UART_NUM_0, (const char *) CMD_EXEC_DONE_STR, strlen((const char *) CMD_EXEC_DONE_STR));
			}
			else if(!strcmp(cmd_buff, CMD_SET_UART_PARITY_STR)){
				if(!strcmp(param_buff, CMD_PARAM_ODD_STR)){
					uart_parity = UART_PARITY_ODD;
				}
				else if(!strcmp(param_buff, CMD_PARAM_EVEN_STR)){
					uart_parity = UART_PARITY_EVEN;
				}
				else if(!strcmp(param_buff, CMD_PARAM_DISABLED_STR)){
					uart_parity = UART_PARITY_DISABLE;
				}

				uart_conf.parity = (uart_parity_t) uart_parity;
				uart_write_bytes(UART_NUM_0, (const char *) CMD_EXEC_DONE_STR, strlen((const char *) CMD_EXEC_DONE_STR));
			}
			else if(!strcmp(cmd_buff, CMD_SET_UART_STOP_STR)){
				if(!strcmp(param_buff, CMD_PARAM_ONE_STR)){
					uart_stop_bits = UART_STOP_BITS_1;
				}
				else if(!strcmp(param_buff, CMD_PARAM_ONE_AND_HALF_STR)){
					uart_stop_bits = UART_STOP_BITS_1_5;
				}
				else if(!strcmp(param_buff, CMD_PARAM_TWO_STR)){
					uart_stop_bits = UART_STOP_BITS_2;
				}

				uart_conf.stop_bits = (uart_stop_bits_t) uart_stop_bits;
				uart_write_bytes(UART_NUM_0, (const char *) CMD_EXEC_DONE_STR, strlen((const char *) CMD_EXEC_DONE_STR));
			}
			else if(!strcmp(cmd_buff, CMD_UART_APPLY_CONFIG_STR)){
				uart_apply_config();
			}
			else{
				uart_write_bytes(UART_NUM_0, (const char *) CMD_UART_UNKNOWN_COMMAND_STR, strlen((const char *) CMD_UART_UNKNOWN_COMMAND_STR));
			}
		}
		else{
			uart_write_bytes(UART_NUM_0, (const char *) CMD_UART_UNKNOWN_ERROR_STR, strlen((const char *) CMD_UART_UNKNOWN_ERROR_STR));
			DEBUGOUT("wub error: command must end with newline or carriage return! %s\n\r", wub_conf.uart_rx_buffer);
		}

		field_counter = 0;
		memset(string_buff, '\0', WUB_CMD_MAX_LEN+WUB_PARAM_MAX_LEN+4);
	}

	vTaskDelete(NULL);
}

void wub_wifi_cmd_exec_task(void *pvParameters){
	wub_cmd_e current_cmd;
	uint32_t param_val;
	char *hello_msg = "\n\rHello, world! This is a test!\n\r";
	char param_buff[WUB_PARAM_MAX_LEN];

	DEBUGOUT("wub_wifi_cmd_task\n\r");

	flush_wifi_cmd_buff();

	while(1){
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		if(wub_conf.transparent_mode){
			uart_write_bytes(UART_NUM_0, (const char *) wub_conf.wifi_rx_buffer, wub_conf.wifi_rx_buffer_len);
		}
		else{
			current_cmd = wub_wifi_cmd_parse(&param_val, param_buff);

			switch(current_cmd){
			case CMD_TRANSPARENT_ON:
				wub_conf.transparent_mode = 1;
				send(client_socket, (char *)CMD_EXEC_DONE_STR, strlen(CMD_EXEC_DONE_STR), 0);
				DEBUGOUT("CMD_TRANSPARENT_ON\n\r");
				break;
			case CMD_SET_BSL_SHARED:
				wub_conf.boot_mode = BSL_UART_SHARED_JTAG_SBW;
				send(client_socket, (char *)CMD_EXEC_DONE_STR, strlen(CMD_EXEC_DONE_STR), 0);
				DEBUGOUT("CMD_SET_BSL_SHARED\n\r");
				break;
			case CMD_SET_BSL_DEDICATED:
				wub_conf.boot_mode = BSL_UART_DEDICATED_JTAG;
				send(client_socket, (char *)CMD_EXEC_DONE_STR, strlen(CMD_EXEC_DONE_STR), 0);
				DEBUGOUT("CMD_SET_BSL_DEDICATED\n\r");
				break;
			case CMD_ENTER_BSL:
				enter_boot();
				send(client_socket, (char *)CMD_EXEC_DONE_STR, strlen(CMD_EXEC_DONE_STR), 0);
				DEBUGOUT("CMD_ENTER_BSL\n\r");
				break;
			case CMD_SET_TIMEOUT:
				wub_conf.transparent_timeout = param_val;
				send(client_socket, (char *)CMD_EXEC_DONE_STR, strlen(CMD_EXEC_DONE_STR), 0);
				DEBUGOUT("CMD_SET_TIMEOUT\n\r");
				break;
			case CMD_RSTT:
				reset_target();
				send(client_socket, (char *)CMD_EXEC_DONE_STR, strlen(CMD_EXEC_DONE_STR), 0);
				DEBUGOUT("CMD_RSTT\n\r");
				break;
			case CMD_HALT:
				halt_target();
				send(client_socket, (char *)CMD_EXEC_DONE_STR, strlen(CMD_EXEC_DONE_STR), 0);
				DEBUGOUT("CMD_HALT\n\r");
				break;
			case CMD_SET_UART_BAUDRATE:
				uart_conf.baud_rate = param_val;
				send(client_socket, (char *)CMD_EXEC_DONE_STR, strlen(CMD_EXEC_DONE_STR), 0);
				DEBUGOUT("CMD_SET_UART_BAUDRATE\n\r");
				break;
			case CMD_SET_UART_WORD_LEN:
				uart_conf.data_bits = (uart_word_length_t) param_val;
				send(client_socket, (char *)CMD_EXEC_DONE_STR, strlen(CMD_EXEC_DONE_STR), 0);
				DEBUGOUT("CMD_SET_UART_WORD_LEN\n\r");
				break;
			case CMD_SET_UART_PARITY:
				uart_conf.parity = (uart_parity_t) param_val;
				send(client_socket, (char *)CMD_EXEC_DONE_STR, strlen(CMD_EXEC_DONE_STR), 0);
				DEBUGOUT("CMD_SET_UART_PARITY\n\r");
				break;
			case CMD_SET_UART_STOP_BIT:
				uart_conf.stop_bits = (uart_stop_bits_t) param_val;
				send(client_socket, (char *)CMD_EXEC_DONE_STR, strlen(CMD_EXEC_DONE_STR), 0);
				DEBUGOUT("CMD_SET_UART_STOP_BIT\n\r");
				break;
			case CMD_UART_APPLY_CONFIG:
				uart_apply_config();
				send(client_socket, (char *)CMD_EXEC_DONE_STR, strlen(CMD_EXEC_DONE_STR), 0);
				DEBUGOUT("CMD_UART_APPLY_CONFIG\n\r");
				break;
			case CMD_RESTART_WUB:
				DEBUGOUT("CMD_UART_APPLY_CONFIG\n\r");
				esp_restart();
				break;
			case CMD_SET_SSID:
				strcpy((char *)wub_conf.wifi_config.sta.ssid, param_buff);
				send(client_socket, (char *)CMD_EXEC_DONE_STR, strlen(CMD_EXEC_DONE_STR), 0);
				DEBUGOUT("CMD_SET_SSID\n\r");
				break;
			case CMD_SET_SSID_PASS:
				strcpy((char *)wub_conf.wifi_config.sta.password, param_buff);
				send(client_socket, (char *)CMD_EXEC_DONE_STR, strlen(CMD_EXEC_DONE_STR), 0);
				DEBUGOUT("CMD_SET_SSID_PASS\n\r");
				break;
			case CMD_SET_SOCK_PORT:
				wub_conf.tcp_listen_port = param_val;
				send(client_socket, (char *)CMD_EXEC_DONE_STR, strlen(CMD_EXEC_DONE_STR), 0);
				DEBUGOUT("CMD_SET_SOCK_PORT\n\r");
				break;
			case CMD_START_WIFI:
				flush_wifi_cmd_buff();
				wifi_init_sta();
				DEBUGOUT("CMD_START_WIFI\n\r");
				break;
			case CMD_HELP:
				display_help();
				DEBUGOUT("CMD_HELP\n\r");
				break;
			case CMD_HELLO:
				send(client_socket, hello_msg, strlen(hello_msg), 0);
				DEBUGOUT("CMD_HELLO\n\r");
				break;
			case CMD_IDN:
				display_idn();
				break;
			case CMD_NONE:
				//Do nothing
				//Fall through
			default:
				send(client_socket, CMD_NONE_STR, strlen(CMD_NONE_STR), 0);
				DEBUGOUT("%s\n\r", CMD_NONE_STR);
			}
		}
	}
}

void wub_wifi_server_task(void *pvParameters){
	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;
	uint addr_len = sizeof(client_addr);
	int err;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(wub_conf.tcp_listen_port);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	DEBUGOUT("wub_server_task\n\r");

	xEventGroupWaitBits(wifi_event_group, ipv4_gotip_bit, false, true, portMAX_DELAY);

	while (1) {
		server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
		if (server_socket < 0) {
			DEBUGOUT("Unable to create socket: errno %d!\n\r", errno);
			break;
		}

		err = bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
		if (err != 0) {
			DEBUGOUT("Socket unable to bind: errno %d!\n\r", errno);
			break;
		}

		err = listen(server_socket, 1);
		if (err != 0) {
			DEBUGOUT("Error occured during listen: errno %d!\n\r", errno);
			break;
		}
		DEBUGOUT("Socket listening...\n\r");

		while(1){
			client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);
			if (client_socket < 0) {
				DEBUGOUT("Unable to accept connection: errno %d!\n\r", errno);
				break;
			}
			DEBUGOUT("Connection accepted!\n\r");

			uart_write_bytes(UART_NUM_0, (const char *) CMD_UART_CLIENT_CONNECTED, strlen((const char *) CMD_UART_CLIENT_CONNECTED));

			while(1){
				wub_conf.wifi_rx_buffer_len = recv(client_socket, wub_conf.wifi_rx_buffer, WUB_WIFI_RX_BUFF_SIZE-1, 0);

				if (wub_conf.wifi_rx_buffer_len < 0) {
					DEBUGOUT("recv failed: errno %d\n\r", errno);
					break;
				}
				else if (wub_conf.wifi_rx_buffer_len == 0) {
					DEBUGOUT("Connection closed!\n\r");
					break;
				}
				else {

					if(!wub_conf.transparent_mode){
						wub_conf.wifi_rx_buffer[wub_conf.wifi_rx_buffer_len] = 0;
					}

					//DEBUGOUT("==================================\n\r");
					//DEBUGOUT("recv: (%d) %s", wub_conf.wifi_rx_buffer_len, wub_conf.wifi_rx_buffer);
					//DEBUGOUT("==================================\n\r");

					xTaskNotifyGive(wub_cmd_task_h);
				}
			}

			if (client_socket != -1) {
				DEBUGOUT("Shutting down socket...\n\r");
				shutdown(client_socket, 0);
				close(client_socket);
				client_socket = 0;
				wub_conf.transparent_mode = 0;
				uart_write_bytes(UART_NUM_0, (const char *) CMD_UART_CLIENT_DISCONNECTED, strlen((const char *) CMD_UART_CLIENT_DISCONNECTED));
			}
		}

	}

	DEBUGOUT("Delete wub_server_task ...\n\r");
	wub_conf.transparent_mode = 0;
	wub_server_task_h = NULL;
	vTaskDelete(NULL);
}

