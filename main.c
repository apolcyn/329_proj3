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
    BCSCTL1 = CALBC1_16MHZ;      // Set range
    DCOCTL = CALDCO_16MHZ;       // Set DCO step + modulation

    TACTL = TASSEL_2 + MC_1 + ID_3;
    TACCR0 = 6000; // sample voltage every 50ms

    P2DIR |= 0xFF;
    P2OUT = 0;

    CACTL2 |= BIT2; // set PCA4,0 to '01', selects CA0 to store sampled input on

    P2SEL &= ~(BIT7 + BIT6);
    P2SEL2 &= ~(BIT7 + BIT6);

    CACTL1 &= ~(BIT5 + BIT4);

    CACTL2 |= BIT1; // turn on comparator output filter

    // SPI setup
    P1DIR |= BIT4;               // Will use BIT4 to activate /CE on the DAC

    P1SEL  = BIT7 + BIT5;        // These two lines dedicate P1.7 and P1.5
    P1SEL2 = BIT7 + BIT5;        // for UCB0SIMO and UCB0CLK respectively

    // SPI Setup
    // clock inactive state = low,
    // MSB first, 8-bit SPI master,
    // 4-pin active Low STE, synchronous
    //
    // 4-bit mode disabled for now
    UCB0CTL0 |= UCCKPL + UCMSB + UCMST + UCSYNC;

    UCB0CTL1 |= UCSSEL_2;               // UCB0 will use SMCLK as the basis for
                                        // the SPI bit clock

    // Sets SMCLK divider to 16,
    // hence making SPI SCLK equal
    // to SMCLK/16 = 1MHz
    UCB0BR0 |= 0x01;                    // (low divider byte)
    UCB0BR1 |= 0x00;                    // (high divider byte)

    UCB0CTL1 &= ~UCSWRST;               // **Initialize USCI state machine**
                                        // SPI now Waiting for something to
                                        // be placed in TXBUF.


    __enable_interrupt();

    P1DIR |= BIT1;
    P1OUT |= BIT1;

	return 0;
}

int max(int a, int b) {
	if(a > b)
		return a;
	return b;
}

/* Writes the proper voltage to the DAC. */
void Drive_DAC(unsigned int level){
  unsigned int DAC_Word = 0;

  DAC_Word = (0x3000) | (level & 0x0FFF);   // 0x1000 sets DAC for Write
                                            // to DAC, Gain = 1, /SHDN = 1
                                            // and put 12-bit level value
                                            // in low 12 bits.

  P1OUT &= ~BIT4;                    // Clear P1.4 (drive /CS low on DAC)
                                     // Using a port output to do this for now

  UCB0TXBUF = (DAC_Word >> 8);       // Shift upper byte of DAC_Word
                                     // 8-bits to right

  while (!(IFG2 & UCB0TXIFG));       // USCI_A0 TX buffer ready?

  UCB0TXBUF = (unsigned char)
		       (DAC_Word & 0x00FF);  // Transmit lower byte to DAC

  while (!(IFG2 & UCB0TXIFG));       // USCI_A0 TX buffer ready?
  //__delay_cycles(5);               // Delay 100 16 MHz SMCLK periods
                                     // (6.25 us) to allow SIMO to complete
                                     // 6.52 us was the lowest amount of time
                                     // that we found we could wait for.

  P1OUT |= BIT4;                     // Set P1.4   (drive /CS high on DAC)
  return;
}

#pragma vector=TIMER0_A0_VECTOR
__interrupt void something(void) {
	CACTL2 &= ~(BIT5 + BIT4 + BIT3); // select analog input on mux
	CACTL2 |= BIT4;

	CACTL1 |= BIT3;  // turn on comparator

	CACTL2 |= BIT7;   // turn on and off T-gate
	__delay_cycles(10000);
	CACTL2 &= ~BIT7;

	__delay_cycles(1600);

	CACTL2 &= ~(BIT5 + BIT4 + BIT3);  // select ramp function on mux
	CACTL2 |= BIT4 + BIT3;

	int i = 0;
	P2OUT = 0;
	__delay_cycles(160);

	while(i < 256  && (CACTL2 & BIT0)) {
		P2OUT = i++;
		__delay_cycles(30);
	}
	P2OUT = 0; //37360, 2.335 ms, 40k cycles, 2.5ms

	// 'i' is now (level where ramp > sample) + 1, unless ramp of '0' > sample
    i = max(1, i) - 1;
    Drive_DAC(i << 4);
}
