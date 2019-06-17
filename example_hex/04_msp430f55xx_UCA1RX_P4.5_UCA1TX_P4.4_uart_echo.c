#include <msp430.h>

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
    char *str = (char *)"Starting uart echo (115200-8-N-1): ";
    char ch;

    WDTCTL = WDTPW | WDTHOLD;

    P4SEL |= BIT4 | BIT5; // P4.4 = UCA1TXD, P4.5 = UCA1RXD
    UCA1CTL1 |= UCSWRST; //Hold in reset
    UCA1CTL1 |= UCSSEL_2; // SMCLK
    UCA1BR0 = 9; //1MHz <-> 115200
    UCA1BR1 = 0; //1MHz <-> 115200
    UCA1MCTL |= (UCBRS_1 | UCBRF_0); // Modulation UCBRSx=1, UCBRFx=0
    UCA1CTL1 &= ~UCSWRST; //Release from reset

    __disable_interrupt();

    display_msg(str);

    while(1){
        ch = get_char();
        put_char(ch);
    }
}
