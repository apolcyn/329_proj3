#include <msp430.h> 

/*
 * main.c
 */
int main(void) {
    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer

    CCTL0 = CCIE;

    // 16Mhz SMCLK
    if (CALBC1_16MHZ==0xFF)	     // If calibration constant erased
    {
        while(1);                // do not load, trap CPU!!
    }
    DCOCTL = 0;                  // Select lowest DCOx and MODx settings
    BCSCTL1 = CALBC1_1MHZ;      // Set range
    DCOCTL = CALDCO_1MHZ;       // Set DCO step + modulation

    TACTL = TASSEL_2 + MC_1;
    TACCR0 = 10000; // sample voltage every 10ms

    P2DIR |= 0xFF;
    P2OUT = 0;

    CACTL2 |= BIT4; // set PCA3,2,1 to '010', selects CA2, where ramp input is
    CACTL2 &= ~(BIT5 + BIT3);

    CACTL2 |= BIT2; // set PCA4,0 to '01', selects CA0 to store sampled input on
    //CACTL2 &= ~(BIT6 + BIT2);

    //CAPD |= BIT0 + BIT2; // bring  CA0 and CA2 out on ports P1.0 and P1.2

   // P2SEL &= ~(BIT7 + BIT6);
   // P2SEL2 &= ~(BIT7 + BIT6);

    CACTL1 &= ~(BIT5 + BIT4);

    CACTL2 |= BIT1; // turn on comparator output filter

    //CACTL1 |= BIT6;

 /*   P1DIR |= BIT2;
    P1OUT |= BIT6;
    P1OUT |= BIT2;*/



    int i = 0;
    while(1) {
    	//P2OUT = i;
    	i = (i + 1) % 256;
        __delay_cycles(400);
        if(i == 20 || i == 180) {
            CACTL2 |= BIT7;        // turn on CASHORT
            //CACTL2 |= BIT2;
            __delay_cycles(10000);
        }
        CACTL2 &= ~BIT7;   // turn off CASHORT
    }



	return 0;
}

int max(int a, int b) {
	if(a > b)
		return a;
	return b;
}

#pragma vector=TIMER0_A0_VECTOR
__interrupt void something(void) {
	CACTL2 &= ~(BIT5 + BIT4 + BIT3); // select analog input on mux
	CACTL2 |= BIT4;

	CACTL2 |= BIT7;   // turn on and off T-gate
	__delay_cycles(100);
	CACTL2 &= ~BIT7;

	CACTL1 |= BIT3;  // turn on comparator

	CACTL2 &= ~(BIT5 + BIT4 + BIT3);  // select ramp function on mux
	CACTL2 |= BIT4 + BIT3;

	int i = 0;
	P2OUT = 0;
	while(i < 256 && (CACTL2 & BIT0)) {
		P2OUT = i++;
	}
	P2OUT = 0;

	// 'i' is now (level where ramp > sample) + 1, unless ramp of '0' > sample
    i = max(1, i);

}
