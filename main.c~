#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#define F_CPU 10000000
#include <util/delay.h>
#include <stdio.h>

#define LED PIN1_bm
#define OPAMP_ENABLE PIN1_bm

#define wdr() __asm__ __volatile__ ("wdr")

/* 
Calculate FTW (Frequency Tuning Word) as
fout/DIV_vco=(fref/DIV_ref)*(FTW/2^32)
FTW = (fout/DIV_vco)/(fref/DIV_ref)*2^32
where 
fout = desired vco frequency to lock at
DIV_vco = division ratio, typ 1024, set for RTC PIT
DIV_ref, reference division ration, set by TCA0.SINGLE.PER+1
fref = external reference frequency
*/

//#define FTW 716963840;  
//#define FTW 664098133
//#define FTW 672836267
//#define FTW 825753600
//#define FTW 834491733
//#define FTW 819636907  9.38
//#define FTW 828375040  // 9.48 MHz at 256 div and 19.2 MHz
#define FTW 671088640  // 12.5 kHz -> 12.8 MHz at 1024 div

uint32_t nco_reg;
uint8_t cosval;
const uint8_t costable[256] =
  {207, 207, 207, 207, 207, 206, 206, 206, 205, 205, 205, 204, 204, 203, 202, 202, 201, 200, 199, 198, 198, 197, 196, 195, 194, 192, 191, 190, 189, 188, 186, 185, 184, 182, 181, 179, 178, 176, 175, 173, 171, 170, 168, 166, 165, 163, 161, 159, 158, 156, 154, 152, 150, 148, 146, 145, 143, 141, 139, 137, 135, 133, 131, 129, 127, 125, 123, 121, 119, 117, 115, 113, 111, 109, 108, 106, 104, 102, 100, 98, 96, 95, 93, 91, 89, 88, 86, 84, 83, 81, 79, 78, 76, 75, 73, 72, 70, 69, 68, 66, 65, 64, 63, 62, 60, 59, 58, 57, 56, 56, 55, 54, 53, 52, 52, 51, 50, 50, 49, 49, 49, 48, 48, 48, 47, 47, 47, 47, 47, 47, 47, 47, 47, 48, 48, 48, 49, 49, 49, 50, 50, 51, 52, 52, 53, 54, 55, 56, 56, 57, 58, 59, 60, 62, 63, 64, 65, 66, 68, 69, 70, 72, 73, 75, 76, 78, 79, 81, 83, 84, 86, 88, 89, 91, 93, 95, 96, 98, 100, 102, 104, 106, 108, 109, 111, 113, 115, 117, 119, 121, 123, 125, 127, 129, 131, 133, 135, 137, 139, 141, 143, 145, 146, 148, 150, 152, 154, 156, 158, 159, 161, 163, 165, 166, 168, 170, 171, 173, 175, 176, 178, 179, 181, 182, 184, 185, 186, 188, 189, 190, 191, 192, 194, 195, 196, 197, 198, 198, 199, 200, 201, 202, 202, 203, 204, 204, 205, 205, 205, 206, 206, 206, 207, 207, 207, 207};

ISR(TCA0_OVF_vect) { // 80 kHz @ 10 MHz 
  TCA0.SINGLE.INTFLAGS = 0x01; // clear flag
  //PORTA.OUT |= LED;
  DAC0.DATA = costable[(nco_reg >> 24)];
  nco_reg += FTW;
  //PORTA.OUT &= ~LED;
}

void main () {
  PORTA.DIR = PIN1_bm | PIN5_bm; // outputs
  PORTB.DIR = PIN1_bm; 
  PORTA.PIN7CTRL = 0x04; // digital input disable for AINP0/PA7
  //PORTA.PIN2CTRL = 0x04; // digital input disable for EVOUT0/PA2
  PORTB.PIN0CTRL = 0x04; // digital input disable for PB0

  PORTA.OUT = 0x00;
  PORTB.OUT = 0x00;

  PORTA.DIR |= PIN2_bm | PIN4_bm;
  PORTA.OUT &= ~PIN2_bm;
  PORTA.OUT |= PIN4_bm; 

  _delay_ms(200);
  PORTA.OUT |= LED;
  _delay_ms(100);
  PORTA.OUT &= ~LED;

  if (RSTCTRL.RSTFR & RSTCTRL_WDRF_bm) {  // Watchdog reset occured, we lost clock
    RSTCTRL.RSTFR |= RSTCTRL_WDRF_bm; // clear flag
    _delay_ms(333);
  }

  _PROTECTED_WRITE (WDT.CTRLA,0x07); // 0.512s WDT on

  _PROTECTED_WRITE (CLKCTRL.MCLKCTRLA,0x03); // EXTCLK for CPU

   while (!(CLKCTRL.MCLKSTATUS & CLKCTRL_EXTS_bm)) {  // wait for ext clock
    wdr();
    _delay_ms(25);
    // _PROTECTED_WRITE (RSTCTRL.SWRR, RSTCTRL_SWRE_bm); // no ext clock - issue software reset
  }
  _PROTECTED_WRITE (CLKCTRL.MCLKCTRLB,0x00); // No prescaler

  _delay_ms(50);
  wdr();

  _delay_ms(10);

  //PORTA.OUT |= LED;
  //for(int i=0; i<10; i++) {
  //  _delay_ms(100);
  //  wdr();
  //}
  PORTA.OUT |= LED;
  //PORTA.OUT &= ~LED;  

  /*
  CPU_CCP = 0xd8;
  CLKCTRL.MCLKCTRLA = 0x03; // EXTCLK
  CPU_CCP = 0xd8;
  CLKCTRL.MCLKCTRLB = 0x00; // No prescaler
  */
  
  VREF.CTRLA = 0b00000010; // 2.5V for DAC0 and AC0
  DAC0.CTRLA = 0b01000001; // OUTEN, ENABLE 

  AC0.MUXCTRLA = 0b00000010; // Use VREF for neg input
  AC0.CTRLA = 0b011100011; // OUTEN (PA5), pos edge, 10mV hyst, AC enable
  
  TCA0.SINGLE.PER = 0x007c;
  TCA0.SINGLE.INTCTRL = 0x01;
  TCA0.SINGLE.CTRLA = 0x01; // Div 1, enable
  
  //CPU_CCP = 0xd8;
  _PROTECTED_WRITE(CLKCTRL.XOSC32KCTRLA,0x05);  // Ext on TOSC1, Enable

  /*
  // Set up schmitt trig oscillator
  PORTA.PIN1CTRL = 0x80; // PA1 inverted
  EVSYS.ASYNCCH0 = 0x0B; // PA1 to ch0
  EVSYS.ASYNCUSER8 = 0x03; // ch0 to EVOUT0/PA2
  PORTMUX.CTRLA |= 0x01; // enable EVOUT0
  */
  
  while(RTC.STATUS);
  RTC.CLKSEL = 0x02; // Use XOSC32K/TOSC1
  while(RTC.PITSTATUS);
  RTC.PITCTRLA = 0x01; // Enable, no interrupt

  //EVSYS.ASYNCCH3 = 0x0f; // PIT_DIV256 to ch3
  EVSYS.ASYNCCH3 = 0x0d; // PIT_DIV1024 to ch3
  EVSYS.ASYNCUSER2 = 0x06; // ch3 to CCL LUT0 EV0
  
  CCL.SEQCTRL0 = 0x00;
  CCL.LUT0CTRLB = 0x63; // input 1 ac out, input 0 LUT0 event0
  CCL.TRUTH0 = 0b00000110; // XOR
  CCL.LUT0CTRLA = 0b00001001; // enable with outen
  //CCL.LUT0CTRLA = 0b00000001; // enable
  CCL.CTRLA = 0x01;
  
  PORTA.DIR &= ~PIN2_bm;

  PORTB.OUT |= OPAMP_ENABLE;

  // Wide loop
  //EVSYS.ASYNCCH1 = 0x01; // CCL LUT0
  //EVSYS.ASYNCUSER8 = 0x04; // ch1 to EVOUT0/PA2
  //PORTMUX.CTRLA |= 0x01; // enable EVOUT0

  _delay_us(10);

  sei();

  //wide loop for 5s
  
  for(int i=0; i<10; i++) {
    _delay_ms(100);
    wdr();
  }
  cli(); 

  
   
  
  // Now narrow loop
  //PORTMUX.CTRLA &= ~0x01; // disable EVOUT0
  //PORTA.DIR &= ~PIN2_bm;
  //CCL.CTRLA = 0x00;
  //CCL.LUT0CTRLA = 0b00000000; // disable
  //CCL.LUT0CTRLA = 0b00001001; // enable with outen
  //CCL.CTRLA = 0x01;
  
  sei();
  
  while(1) {
    _delay_ms(10);
    wdr();
  }
  
}
