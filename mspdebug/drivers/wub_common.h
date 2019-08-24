/*
 * wub common
 *
 *  Created on: June 04, 2019
 *      Author: lbogdanov
 *
 *      file version 1.0
 */

#ifndef WUB_COMMON_H_
#define WUB_COMMON_H_

#define WUB_SW_REVISION					1
#define WUB_SW_SUBREVISION				2
#define WUB_HW_REVISION					1
#define WUB_HW_SUBREVISION				0

#define IDN_MANUFACTURER_STR			"WUB"
#define IDN_MODEL_STR					"ESP8266"

//Parameters
#define CMD_PARAM_ON_STR				"ON"
#define CMD_PARAM_OFF_STR				"OFF"
#define CMD_PARAM_SHARED_STR			"SHARED"
#define CMD_PARAM_DEDICATED_STR 		"DEDIC"
#define CMD_PARAM_ODD_STR				"ODD"
#define CMD_PARAM_EVEN_STR				"EVEN"
#define CMD_PARAM_DISABLED_STR			"DISABLED"
#define CMD_PARAM_ONE_STR				"ONE"
#define CMD_PARAM_ONE_AND_HALF_STR		"ONE_AND_HALF"
#define CMD_PARAM_TWO_STR				"TWO"
//Wifi commands
#define CMD_TRANSPARENT_STR				"TRAN" //TRAN {ON}
#define CMD_SET_BSL_TYPE_STR			"BSLT" //BSLT {SHARED|DEDICATED}
#define CMD_ENTER_BSL_STR				"BOOT" //BOOT
#define CMD_SET_TRANSPARENT_TIMEOUT_STR	"SETT" //SETT <numeric value>
#define CMD_HELP_STR					"HELP" //HELP
#define CMD_HALT_STR					"HALT" //HALT
#define CMD_HELLO_STR					"HELLo" //HELLo
#define CMD_RSTT_STR					"RSTT" //RSTT
#define CMD_SET_UART_BAUDRATE_STR		"SETB" //SETB <numeric value>
#define CMD_SET_UART_WORD_STR			"SETW" //SETW <numeric value>
#define CMD_SET_UART_PARITY_STR			"SETP" //SETP {ODD|EVEN|DISABLED}
#define CMD_SET_UART_STOP_STR			"SETS" //SETS {ONE|ONE_AND_HALF|TWO}
#define CMD_UART_APPLY_CONFIG_STR		"UARA" //UARA
#define CMD_RESTART_WUB_STR				"WUBR" //WUBR
#define CMD_WIFI_SET_SSID_STR			"SSID" //SSID <ip value>
#define CMD_WIFI_SET_PASS_STR			"PASS" //PASS <string>
#define CMD_WIFI_SET_PORT_STR			"PORT" //PORT <numeric value>
#define CMD_WIFI_START_STR				"WIFS" //WIFS
#define CMD_WIFI_IDN_STR				"*IDN?"//*IDN?
//UART commands
#define CMD_UART_SET_SSID_STR			"SSID" //SSID <ip value>
#define CMD_UART_SET_PASS_STR			"PASS" //PASS <string>
#define CMD_UART_SET_PORT_STR			"PORT" //PORT <numeric value>
#define CMD_UART_WIFI_START_STR			"WIFS" //WIFS
#define CMD_UART_WIFI_STOP_STR			"WIFT" //WIFT
#define CMD_UART_HELP_STR				"HELP" //HELP
#define CMD_UART_SET_TRAN_STR			"TRPI" //TRPI (transparent pin)
#define CMD_UART_SET_TRAN_OFF_STR		"TRPO" //TRPO (transparent pin off)
#define CMD_UART_IDN_STR				"*IDN?"//*IDN?
#define CMD_UART_RESTART_WUB_STR		"WUBR" //WUBR
#define CMD_UART_POWER_OFF_WUB_STR		"PWRO" //PWRO
//Replies
#define CMD_EXEC_DONE_STR				"DONE\n\r" //Reply from server that the requested cmd has been executed
#define CMD_NONE_STR					"NONE, try HELP\n\r"
#define CMD_CONNECTED_STR				"CONN\n\r"
#define CMD_WIFI_DISCONNECTED_STR		"DISC\n\r"
#define CMD_WIFI_UNKNOWN_ERROR_STR		"WERR\n\r"
#define CMD_UART_UNKNOWN_ERROR_STR		"UERR\n\r"
#define CMD_UART_UNKNOWN_COMMAND_STR	"ERRU\n\r"
#define CMD_UART_READY_STR				"READy\n\r"
#define CMD_UART_CLIENT_CONNECTED		"CLIC\n\r"
#define CMD_UART_CLIENT_DISCONNECTED	"CLID\n\r"

#define HELP_STR	"-------Wi-Fi commands:--------\n\r"															\
					"TRAN {ON} - Set transparent mode ON.\n\r"														\
					"BSLT {SHARED|DEDICATED} - select BSL entry sequence type - shared or dedicated.\n\r"			\
		    		"BOOT - initiate MSP430 boot sequence.\n\r"														\
					"RSTT - reset target. SBWTDIO (RST) must be tied to Vdd with pull-up.\n\r"						\
		    		"SETT <numeric value> - set timeout for the transparent mode. (not implemented yet)\n\r"	    \
					"HALT - hold the target in reset.\n\r"															\
					"HELLo - display a warm greeting over TCP/IP to test connection.\n\r"							\
					"-------Uart commands:--------\n\r"																\
					"WIFT - stop the server and disconnect from wi-fi, this is UART command only.\n\r"				\
					"TRPI <numeric value> - init pin (0 - 16) as input that will toggle the TRAN ON/OFF state.\n\r"	\
					"TRPO - deinit pin transparency, use TRAN command only.\n\r"	                                \
					"PWRO - turn wifi-uart-bridge power off, wake-up only through its reset pin\n\r"				\
					"-------Common commands:--------\n\r"															\
					"*IDN? - request WUB identification number: <manufac>,<model>,<serno>,HW<1.0>,SW<1.0>.\n\r"     \
					"HELP - display this help over wifi.\n\r"														\
					"WUBR - restart wifi-uart-bridge without restarting the target.\n\r"  							\
					"SSID <string> - change the SSID name over the current network.\n\r"							\
					"PASS <string> - change the network passord over the current network.\n\r"						\
					"PORT <numeric value> - change the listen port over the current network.\n\r"					\
					"WIFS - start a new server using the above parameters over the current network.\n\r"			\
					"SETB <numeric value> - set uart baudrate, see ESP8266 datasheet for valid values.\n\r"			\
					"SETW <numeric value> - set uart word length, valid values are 5, 6, 7 and 8.\n\r"				\
					"SETP {ODD|EVEN|NONE} - set uart parity, valid values are the strings ODD, EVEN or NONE.\n\r"	\
					"SETS {ONE|ONE_AND_HALF|TWO} - set uart stop bit.\n\r"											\
					"UARA - UART apply config with the new settings.\n\r"											\
					"-------Reply--------\n\r"																		\
					"READy - WUB is ready for operation.\n\r"														\
					"DONE - the requested command has been executed. Some commands do not have a reply.\n\r"		\
					"NONE - the requested command does not exist or an error has occured.\n\r"						\
					"CONN - the wub has connected successfully to the wi-fi network.\n\r"							\
					"DISC - the wub has been disconnected from the wi-fi network.\n\r"	     						\
					"WERR - An unknown error has occured with the wi-fi adapter.\n\r"								\
					"UERR - An unknown error has occured on the UART interface.\n\r"								\
					"ERRU - An unknown command has been requested on the UART interface.\n\r"						\
					"CLIC - A client has connected to the WUB server.\n\r"											\
					"CLID - A client has disconnected from the WUB server. Transparency is also turned off.\n\r"	\

typedef enum{
	BSL_UART_SHARED_JTAG_SBW,
	BSL_UART_DEDICATED_JTAG
}bsl_type_e;

typedef enum{
	CMD_TRANSPARENT_ON,
	CMD_SET_BSL_SHARED,
	CMD_SET_BSL_DEDICATED,
	CMD_ENTER_BSL,
	CMD_SET_TIMEOUT,
	CMD_HELP,
	CMD_HELLO,
	CMD_RSTT,
	CMD_HALT,
	CMD_SET_UART_BAUDRATE,
	CMD_SET_UART_WORD_LEN,
	CMD_SET_UART_PARITY,
	CMD_SET_UART_STOP_BIT,
	CMD_UART_APPLY_CONFIG,
	CMD_RESTART_WUB,
	CMD_SET_SSID,
	CMD_SET_SSID_PASS,
	CMD_SET_SOCK_PORT,
	CMD_START_WIFI,
	CMD_IDN,
	CMD_PWRO,
	CMD_NONE
}wub_cmd_e;

#endif
