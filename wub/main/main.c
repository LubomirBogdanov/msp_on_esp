/*
 * main.c
 *
 *  Created on: May 27, 2019
 *      Author: lbogdanov
 */
#include "freertos/FreeRTOS.h"
#include "nvs_flash.h"
#include "wub.h"

extern TaskHandle_t wub_cmd_task_h;
extern TaskHandle_t wub_uart_cmd_task_h;
extern TaskHandle_t wub_server_task_h;
extern TaskHandle_t wub_gpio_transparent_task_h;
extern EventGroupHandle_t wifi_event_group;

void app_main(){
	uart_config_t uart_default_config = {
			.baud_rate = WUB_DEFAULT_UART_BAUD,
			.data_bits = WUB_DEFAULT_UART_BITS,
			.parity    = WUB_DEFAULT_UART_PARITY,
			.stop_bits = WUB_DEFAULT_UART_STOP_BIT,
			.flow_ctrl = WUB_DEFAULT_UART_FLOW_CTRL
	};
	esp_err_t ret;

	ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}

	gpio_install_isr_service(0);
	pin_init_as_inputs();
	uart_init(&uart_default_config);
	wifi_init_defaults();
	init_defaults_params();

	wub_uart_cmd_task_h = NULL;
	wub_cmd_task_h = NULL;
	wub_server_task_h = NULL;
	wub_gpio_transparent_task_h = NULL;

	wifi_event_group = xEventGroupCreate();

	xTaskCreate(wub_uart_rx_event_task, "wub_uart_rx_event_task", 2048, NULL, WUB_UART_RX_TASK_PRIORITY, NULL);
	xTaskCreate(wub_uart_cmd_exec_task, "wub_uart_cmd_exec_task", 2048, NULL, WUB_UART_CMD_TASK_PRIORITY, &wub_uart_cmd_task_h);
	xTaskCreate(wub_wifi_cmd_exec_task, "wub_wifi_cmd_exec_task", 4096, NULL, WUB_CMD_TASK_PRIORITY, &wub_cmd_task_h);
	xTaskCreate(wub_wifi_server_task, "wub_wifi_server_task", 8192, NULL, WUB_WIFI_TASK_PRIORITY, &wub_server_task_h);

	wifi_init_sta();
	display_ready();
}


