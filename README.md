Description  
===================================================================   
The project aims at flashing MSP430-based microcontrollers over  
the Internet.  
  
The project is based on mspdebug v0.25 and ESP8266_RTOS_SDK v3.1.0  
(2019.01.31).  
  
The ESP8266 firmware project (named wub) must be placed in:  
    
/path/to/espressif/ESP8266_RTOS_SDK/examples/wifi  
      
for the version 3.1.0 of the SDK (idf-style).  

Supported ESP8266 modules are:  

ESP8266-01, ESP8266-02, ESP8266-03, ESP8266-04, ESP8266-06,  
ESP8266-07, ESP8266-08, ESP8266-09, ESP8266-11, ESP8266-12 E/Q/D,  
ESP8266-13, ESP8266-14, Olimex MOD-WiFi-ESP8266-DEV  

Modules not supported:  

ESP8266-05, ESP8266-10, ESP8266-14  

The WUB supports SCPI commands over the network and over the UART,  
that same UART that is used for flashing the MSP430. The WUB can  
be made transparent without flashing the MSP430. In that case  
the WUB can be used for printf debugging over the Internet. Check  
the SCPI commands below. For a tcp-ip client you can use telnet,  
netcat, putty, tera term and so on.  


!!! NOTE: the default UART settings are 9600-8-E-1 !!!  


INSTALL  
===================================================================
//MSPDebug  
To install the modified mspdebug follow these steps. Tested on  
Ubuntu 18.04 (64-bit):  

sudo apt install libreadline-dev  
sudo apt install libusb-dev  
cd msp_on_esp  
cd mspdebug  

The mspdebug requires libmsp430.so on Linux, so follow this how-to:  

https://dlbeer.co.nz/articles/slac460y/index.html  

Which is a bit different on Ubuntu 18.04:  

sudo apt install libboost-all-dev  
sudo apt install libhidapi-dev  
mkdir libmsp430  
cd libmsp430  
unzip ../slac460y.zip  
patch -p1 < ../slac460y-linux.patch  
make  
sudo make install  

//Wifi-to-UART-Bridge (WUB)  
Make sure you're running a FLASH or ROM-based UART BSL on your  
MSP430 device. Make the following connections (example given for   
MSP430F5529):  
<pre>  
                                    o 3V3      
                                    |                                                                                
                                    | _ _GND  
                                    |  |
                                    >  _
                            47kOhm  >  _  1nF
                                    >  |
ESP8266 module                      >  |  MSP430F5529
----------------                    |  |  ----------------
               |         4 X 1kOhm  |  |  |
     IO0 (RST) |--------/VVVVV\-----*--*--| SBWTDIO (!RST)
     IO2 (BOOT)|--------/VVVVV\-----------| SBWTCK (BOOT)
    IO1 (U0TxD)|--------/VVVVV\-----------| P1.2 (RxD)
    IO3 (U0RxD)|--------/VVVVV\-----------| P1.1 (TxD)
               |                          |
               |                          |
----------------                          ----------------
</pre>  

Note: omit resistors if the respective MSP430 pins are not used in  
the application circuit and if those pins are dedicated to firmware  
updates only.   

Example usage  
===================================================================
Go to the wub directory and type:  

make menuconfig  

Then select "example config -> SSID" and "example config -> password"  
to enter your wi-fi router's network name and password (WPA2). The  
router must use the DHCP. Check your wi-fi router support web page   
and search for the newly connected device. Find your WUB device  
ip address. By default the WUB uses port 5556.   

//WUB flash example projects for MSP430F5529  
./mspdebug flash-bsl-wifi-bridge --wub-ip {wub-ip} --wub-port {wub-port} --long-password 'prog msp_on_esp/example_hex/01_msp430f55xx_P1.0_heartbeat.txt'  
./mspdebug flash-bsl-wifi-bridge --wub-ip {wub-ip} --wub-port {wub-port} --long-password 'prog msp_on_esp/example_hex/02_msp430f55xx_P1.0_blinky_slow.txt'  
./mspdebug flash-bsl-wifi-bridge --wub-ip {wub-ip} --wub-port {wub-port} --long-password 'prog msp_on_esp/example_hex/03_msp430f55xx_P1.0_blinky_fast.txt'  
./mspdebug flash-bsl-wifi-bridge --wub-ip {wub-ip} --wub-port {wub-port} --long-password 'prog msp_on_esp/example_hex/04_msp430f55xx_UCA1RX_P4.5_UCA1TX_P4.4_uart_echo.txt'  
./mspdebug flash-bsl-wifi-bridge --wub-ip {wub-ip} --wub-port {wub-port} --long-password 'prog msp_on_esp/example_hex/05_msp430f55xx_picojpeg_13k_flash.txt'  
./mspdebug flash-bsl-wifi-bridge --wub-ip {wub-ip} --wub-port {wub-port} --long-password 'prog msp_on_esp/example_hex/06_msp430f55xx_mean_val_64k.txt'  
./mspdebug flash-bsl-wifi-bridge --wub-ip {wub-ip} --wub-port {wub-port} --long-password 'prog msp_on_esp/example_hex/07_msp430f55xx_mean_val_128k.txt'  

//Reset  
./mspdebug flash-bsl-wifi-bridge --wub-ip 10.42.0.49 --wub-port 5556 --wifi-bridge-skip-init 'reset'  

On my computer I used:  

./mspdebug flash-bsl-wifi-bridge --wub-ip 10.42.0.49 --wub-port 5556 --long-password 'prog /home/user/Desktop/msp_on_esp/example_hex/07_msp430f55xx_mean_val_128k.txt'  

because the wi-fi router gave the IP 10.41.0.49 to my board.  

Supported WUB SCPI commands  
===================================================================  
TRAN {ON} - Set transparent mode on. The WUB will stop receiving SCPI commands until the TCP connection is closed. When this happens, WUB commands will be accessible again.  
BSLT {SHARED|DEDICATED} - select BSL entry sequence type - shared or dedicated.  
BOOT - initiate MSP430 boot sequence.  
RSTT - reset target. SBWTDIO (RST) must be tied to Vdd with pull-up because when the command finishes, both pins IO0 and IO2 will be configured as inputs. The WUB detaches itself this way. The UART pins continue to operate as UART, though.  
HELP - display a list of commands over wi-fi.  
HALT - hold the target in reset.  
SETB <numeric value> - set UART baudrate, see ESP8266 datasheet for valid values.  
SETW <numeric value> - set UART word length, valid values are 5, 6, 7 and 8.  
SETP {ODD|EVEN|NONE} - set UART parity, valid values are the strings ODD, EVEN or NONE.	  
SETS {ONE|ONE_AND_HALF|TWO} - set UART stop bit.  
UARA - apply UART configuration with the new settings.  
WUBR - restart wifi-uart-bridge without restarting the target.  
HELLo - display a warm greeting over TCP/IP to test connection.	 
SSID <string> - change the SSID name over the current network, this command is dangerous!  
PASS <string> - change the network password over the current network, this command is dangerous!  
PORT <numeric value> - change the listen port over the current network.  
WIFS - start a new server using the above parameters over the current network.  

