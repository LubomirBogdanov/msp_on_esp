/*
 * wub -> wifi_uart_bridge.h
 *
 *  Created on: May 21, 2019
 *      Author: lbogdanov
 */

#ifndef WIFI_UART_BRIDGE_H_
#define WIFI_UART_BRIDGE_H_

#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "driver/gpio.h"
#include "lwip/sockets.h"
#include "unistd.h"
#include "driver/uart.h"
#include "wub_common.h"
#include "esp_sleep.h"

//#define WUB_DEBUG
#define GPIO_PIN_0						0x01
#define GPIO_PIN_2						0x04
#define GPIO_PIN_0_						GPIO_NUM_0
#define GPIO_PIN_2_						GPIO_NUM_2
#define WUB_DEFAULT_GPIO_DELAY 			(10 / portTICK_RATE_MS)
#define WUB_RESET_DELAY					(200 / portTICK_RATE_MS)
#define WUB_DEFAULT_WIFI_SSID   		CONFIG_ESP_WIFI_SSID
#define WUB_DEFAULT_WIFI_PASS   		CONFIG_ESP_WIFI_PASSWORD
#define WUB_DEFAULT_WIFI_SERVER_PORT	5556
#define WUB_WIFI_RX_BUFF_SIZE			512
#define WUB_UART_RX_BUFF_SIZE			512
#define WUB_CMD_BUFF_SIZE				512
#define WUB_CMD_MAX_LEN					32
#define WUB_PARAM_MAX_LEN				32
#define WUB_WIFI_TASK_PRIORITY			1
#define WUB_UART_RX_TASK_PRIORITY		1
#define WUB_CMD_TASK_PRIORITY			5
#define WUB_UART_CMD_TASK_PRIORITY		5
#define WUB_MAIN_TASK_PRIORITY			5
#define WUB_GPIO_TRANSPARENT_PRIORITY	5
#define WUB_GPIO_TRANSPARENT_DEBOUNCE	10
#define WUB_DEFAULT_TRANSP_TIMEOUT		1000
#define WUB_DEFAULT_UART_BAUD			CONFIG_CONSOLE_UART_BAUDRATE
#define WUB_DEFAULT_UART_PARITY			UART_PARITY_EVEN
#define WUB_DEFAULT_UART_BITS			UART_DATA_8_BITS
#define WUB_DEFAULT_UART_STOP_BIT		UART_STOP_BITS_1
#define WUB_DEFAULT_UART_FLOW_CTRL		UART_HW_FLOWCTRL_DISABLE

#ifdef WUB_DEBUG
	#define DEBUGOUT(...) printf(__VA_ARGS__)
#else
	#define DEBUGOUT(...)
#endif

typedef struct{
	int wifi_rx_buffer_len;
	char wifi_rx_buffer[WUB_WIFI_RX_BUFF_SIZE];
	uint16_t cmd_buffer_len;
	char cmd_buffer[WUB_CMD_BUFF_SIZE];
	uint16_t uart_rx_buffer_len;
	uint8_t uart_rx_buffer[WUB_UART_RX_BUFF_SIZE];
	bsl_type_e boot_mode;
	uint8_t transparent_mode;
	uint32_t transparent_timeout;
	wifi_config_t wifi_config;
	uint16_t tcp_listen_port;
	uint8_t wifi_connected;
	uint8_t gpio_transparent;
}wub_config_t;

void wifi_init_sta(void);
void wifi_deinit_sta(void);
void enter_boot(void);
void reset_target(void);
void halt_target(void);
void pin_init_as_inputs(void);
void pin_init_as_outputs(void);
void pin_rst_set(uint8_t value);
void pin_boot_set(uint8_t value);
void pin_init_transparent(uint8_t pin_number);
void pin_deinit_transparent(void);
void uart_init(uart_config_t *uart_config);
void uart_apply_config(void);
void display_help(void);
void display_help_uart(void);
uint16_t get_idn_string(char *idn_str);
void display_idn(void);
void display_idn_uart(void);
void display_ready(void);

wub_cmd_e wifi_cmd_parse(uint32_t *param_numeric);
void flush_wifi_cmd_buff(void);
void flush_uart_cmd_buff(void);
void flush_wifi_rx_buff(void);
int16_t find_terminator_index(char *str);
void wifi_init_defaults(void);
void init_defaults_params(void);

wub_cmd_e wub_wifi_cmd_parse(uint32_t *param_numeric, char *param_string);
void wub_uart_rx_event_task(void *pvParameters);
void wub_uart_cmd_exec_task(void *pvParameters);
void wub_wifi_cmd_exec_task(void *pvParameters);
void wub_wifi_server_task(void *pvParameters);
void wub_gpio_transparent_task(void *arg);

#endif /* WIFI_UART_BRIDGE_H_ */
