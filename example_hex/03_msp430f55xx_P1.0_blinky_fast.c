#include <msp430.h>

int main(void){
  volatile unsigned long i;

  WDTCTL = WDTPW | WDTHOLD;
  P1DIR |= BIT0;                            // P1.0 set as output

  while(1){
    P1OUT |= BIT0;
    for(i=2000;i>0;i--);
    P1OUT &= ~BIT0;
    for(i=2000;i>0;i--);
  }
}
