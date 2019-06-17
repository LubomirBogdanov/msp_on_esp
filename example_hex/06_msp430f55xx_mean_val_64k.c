#include <stdio.h>
#include <stdint.h>
#include <msp430.h>
#include "rand_arr.h"

char get_char(void){
    while (!(UCA1IFG & UCRXIFG)) { }
    return UCA1RXBUF;
}

void put_char(char ch){
    while (!(UCA1IFG & UCTXIFG)) { }
    UCA1TXBUF = ch;
}

void display_msg(char *message){
    while(*message != '\0'){
        put_char(*(message++));
    }
}

int main(void){
    char *start_msg = (char *)"Starting mean algorithm ...";
    char *stop_msg = (char *)"end!\n\r";
    char result_buff[80];
    unsigned long i, j;
    uint64_t mean = 0;
    uint32_t sub_mean = 0;
    volatile uint16_t k;

    WDTCTL = WDTPW | WDTHOLD;

    P1DIR |= BIT0;

    P4SEL |= BIT4 | BIT5; // P4.4 = UCA1TXD, P4.5 = UCA1RXD
    UCA1CTL1 |= UCSWRST; //Hold in reset
    UCA1CTL1 |= UCSSEL_2; // SMCLK
    UCA1BR0 = 9; //1MHz <-> 115200
    UCA1BR1 = 0; //1MHz <-> 115200
    UCA1MCTL |= (UCBRS_1 | UCBRF_0); // Modulation UCBRSx=1, UCBRFx=0
    UCA1CTL1 &= ~UCSWRST; //Release from reset

    __disable_interrupt();

    while(1){
        P1OUT |= BIT0;
        display_msg(start_msg);

        for(i = 0; i < 50000; i+=100){
            for(j = 0; j < 100; j++){
                sub_mean += rand_arr[i + j];
            }
            sub_mean /= 100;
            mean += sub_mean;
            sub_mean = 0;
        }

        sprintf(result_buff, "result: %032llu\n", mean);
        mean = 0;
        sub_mean = 0;

        P1OUT &= ~BIT0;
        display_msg(stop_msg);
        display_msg(result_buff);

        for(k = 0; k < 50000; k ++){ }
    }
}
