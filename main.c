#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#define F_CPU 10000000
#include <util/delay.h>
#include <stdio.h>
#define LED PIN1_bm
#define DEBUG_1 PIN1_bm

#define DEBUG

#ifdef DEBUG
#include <stdio.h>
#include <string.h>
#endif

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
//#define FTW 671088640  // 12.5 kHz -> 12.8 MHz at 1024 div
//#define FTW 1006632960  // 19.2e6/1024 at 10/125 MHz

volatile uint8_t full_count = 0;
volatile uint16_t capt;
uint32_t FTW, nco_reg;
uint8_t phase;
const uint8_t costable[256] =
  {207, 207, 207, 207, 207, 206, 206, 206, 205, 205, 205, 204, 204, 203, 202, 202, 201, 200, 199, 198, 198, 197, 196, 195, 194, 192, 191, 190, 189, 188, 186, 185, 184, 182, 181, 179, 178, 176, 175, 173, 171, 170, 168, 166, 165, 163, 161, 159, 158, 156, 154, 152, 150, 148, 146, 145, 143, 141, 139, 137, 135, 133, 131, 129, 127, 125, 123, 121, 119, 117, 115, 113, 111, 109, 108, 106, 104, 102, 100, 98, 96, 95, 93, 91, 89, 88, 86, 84, 83, 81, 79, 78, 76, 75, 73, 72, 70, 69, 68, 66, 65, 64, 63, 62, 60, 59, 58, 57, 56, 56, 55, 54, 53, 52, 52, 51, 50, 50, 49, 49, 49, 48, 48, 48, 47, 47, 47, 47, 47, 47, 47, 47, 47, 48, 48, 48, 49, 49, 49, 50, 50, 51, 52, 52, 53, 54, 55, 56, 56, 57, 58, 59, 60, 62, 63, 64, 65, 66, 68, 69, 70, 72, 73, 75, 76, 78, 79, 81, 83, 84, 86, 88, 89, 91, 93, 95, 96, 98, 100, 102, 104, 106, 108, 109, 111, 113, 115, 117, 119, 121, 123, 125, 127, 129, 131, 133, 135, 137, 139, 141, 143, 145, 146, 148, 150, 152, 154, 156, 158, 159, 161, 163, 165, 166, 168, 170, 171, 173, 175, 176, 178, 179, 181, 182, 184, 185, 186, 188, 189, 190, 191, 192, 194, 195, 196, 197, 198, 198, 199, 200, 201, 202, 202, 203, 204, 204, 205, 205, 205, 206, 206, 206, 207, 207, 207, 207};

#ifdef DEBUG
void USART_Transmit_String(char *buffer) {
  uint8_t n = 0;
  while(buffer[n]) {
    while (!(USART0.STATUS & USART_DREIF_bm));
    USART0.TXDATAL = buffer[n];
    n++;
  }
}
#endif

ISR(TCA0_OVF_vect) { // 80 kHz @ 10 MHz 
  TCA0.SINGLE.INTFLAGS = 0x01; // clear flag
  //PORTA.OUT |= LED;
  DAC0.DATA = costable[phase]; //(nco_reg >> 24)];
  nco_reg += FTW;
  phase = (nco_reg >> 24);
  //PORTA.OUT &= ~LED;
}

ISR(TCD0_OVF_vect) { 
  capt = TCA0.SINGLE.CNT;
  TCD0.INTFLAGS = TCD_OVF_bm; // clear flag
  full_count++;
}

ISR(RTC_PIT_vect) { 
  if (0==full_count) {
    TCA0.SINGLE.CTRLA = 0x01; // Enable counting with ext clk
    PORTB.OUT |= DEBUG_1;
  }
  capt = TCA0.SINGLE.CNT+1;  // compensate for lost time?
  RTC.PITINTFLAGS = 0x01; // clear flag
  full_count++;
}

void main () {
  
  #ifdef DEBUG
  char buffer[20];
  #endif

  uint16_t freq_ref_kHz;
  int16_t diff_100kHz;
  uint32_t freq_VCXO_kHz, freq_VCXO_100kHz;


  PORTA.DIR = PIN1_bm | PIN5_bm; // outputs
  PORTB.DIR = PIN1_bm | PIN2_bm; 
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
  _delay_ms(200);
  PORTA.OUT &= ~LED;

  _PROTECTED_WRITE (CLKCTRL.MCLKCTRLB,(0x08 << 1) | 0x01); // Prescaler 6

  #ifdef DEBUG
  // UART for debug

  // Baud rate compensated with factory stored frequency error
  int8_t sigrow_val = SIGROW.OSC20ERR3V;
  int32_t baud_reg_val = (int32_t)1389;  // 9600 baud at 3.33MHz

  baud_reg_val *= (1024 + sigrow_val);
  baud_reg_val /= 1024;
  USART0.BAUD = (int16_t) baud_reg_val;

  //PORTMUX.CTRLA = 0x01; // Use alternative pins, i.e TXD on PA1
  //USART0.CTRLA = 0b10000000; // RXCIE
  USART0.CTRLB = 0b01000000; // TX enable

  USART_Transmit_String("Start\n");
  #endif

  if (RSTCTRL.RSTFR & RSTCTRL_WDRF_bm) {  // Watchdog reset occured, we lost clock
    RSTCTRL.RSTFR |= RSTCTRL_WDRF_bm; // clear flag
    #ifdef DEBUG
    USART_Transmit_String("Restart - no ref clock\n");
    #endif
    _delay_ms(333);
  }

  _PROTECTED_WRITE (WDT.CTRLA,0x07); // 0.512s WDT on

  //_PROTECTED_WRITE (CLKCTRL.OSC20MCTRLA,0x02); // 20MHz clock always on

  _PROTECTED_WRITE (CLKCTRL.MCLKCTRLA,0x03); // EXTCLK for CPU

   while (!(CLKCTRL.MCLKSTATUS & CLKCTRL_EXTS_bm)) {  // wait for ext clock
    wdr();
    _delay_ms(25);
    // _PROTECTED_WRITE (RSTCTRL.SWRR, RSTCTRL_SWRE_bm); // no ext clock - issue software reset
  }

  _PROTECTED_WRITE (CLKCTRL.MCLKCTRLB,0x00); // No prescaler

  _delay_ms(50);

  #ifdef DEBUG
  USART0.BAUD = (int16_t) 4167;  // 9600 bauds at 10 MHz
  USART_Transmit_String("Ref clock detected\n");
  #endif
  _delay_ms(50);
  wdr();

  _delay_ms(10);

  // **** Ok we have ref clock, but at what freq?
  // Let TCD0 run at 20 MHz int osc and count ref at TCA0

  TCD0.INTCTRL = TCD_OVF_bm;
  TCD0.CMPBCLR = 0x0FFF;
  while (!(TCD0.STATUS & TCD_ENRDY_bm));
  TCD0.CTRLA = 0x01; // Start counting with internal 20 MHz clock
  
  //PORTB.OUT &= ~DEBUG_1;

  TCA0.SINGLE.CTRLA = 0x00;

  sei();
  
  while(1) {
    wdr();
    if (1 == full_count && 0x00 == TCA0.SINGLE.CTRLA) {
      TCA0.SINGLE.CTRLA = 0x01; // Enable counting with ext clk
      //PORTB.OUT |= DEBUG_1;
    }
    else if (2 == full_count) {
      TCA0.SINGLE.CTRLA = 0x00;  // Stop
      break;
    }
  }

  cli();

  PORTB.OUT &= ~DEBUG_1;

  while (!(TCD0.STATUS & TCD_ENRDY_bm));
  TCD0.CTRLA = 0x00; // Stop

  #define REF_10MHZ_LOWT 1884
  #define REF_10MHZ_HIGHT 2212
  #define REF_20MHZ_LOWT 3768
  #define REF_20MHZ_HIGHT 4423
  #define REF_26MHZ_LOWT 4899
  #define REF_26MHZ_HIGHT 5751

  uint16_t TCA_PER;

  if ((TCA0.SINGLE.CNT > REF_10MHZ_LOWT) && (TCA0.SINGLE.CNT < REF_10MHZ_HIGHT)) {
    freq_ref_kHz = 10000;
    TCA_PER = (uint16_t)0x007c; // 10e6/(0x7c+1) = 80 kHz
  }
  else if ((TCA0.SINGLE.CNT > REF_20MHZ_LOWT) && (TCA0.SINGLE.CNT < REF_20MHZ_HIGHT)) {
    freq_ref_kHz = 20000;
    TCA_PER = (uint16_t)0x00f9; // 10e6/(0xf9+1) = 80 kHz
  }
  else if ((TCA0.SINGLE.CNT > REF_26MHZ_LOWT) && (TCA0.SINGLE.CNT < REF_26MHZ_HIGHT)) {
    freq_ref_kHz = 26000;
    TCA_PER = (uint16_t)0x0144; // 10e6/(0x144+1) = 80 kHz
  }

  #ifdef DEBUG
  sprintf(buffer,"CNT: %u\n", TCA0.SINGLE.CNT);
  USART_Transmit_String(buffer);
  sprintf(buffer,"Ref: %u kHz\n", freq_ref_kHz);
  USART_Transmit_String(buffer);
  #endif

  //_PROTECTED_WRITE (CLKCTRL.OSC20MCTRLA,0x00); // 20MHz clock off

  // *** Now try to measure the VCXO frequency using PIT and TCA0

  

  _PROTECTED_WRITE(CLKCTRL.XOSC32KCTRLA,0x05);  // Ext on TOSC1, Enable

  while(RTC.STATUS);
  RTC.CLKSEL = 0x02; // Use XOSC32K/TOSC1 - i.e. VCXO
  
  //capt = 0;
  uint8_t div = 0x08; // 512
  uint8_t stop = 2;
  do {
    full_count = 0;
    TCA0.SINGLE.CNT = 0;
    if (div < 0x0e)
      div++;
    else
      stop++;
    #ifdef DEBUG
    sprintf(buffer,"Try VCXO meas with div=%x and stop=%d\n", div, stop);
    USART_Transmit_String(buffer);
    #endif

    //while(RTC.PITSTATUS);
    RTC.PITINTCTRL = 0x01;
    while(RTC.PITSTATUS);
    RTC.PITCTRLA = (div << 3) | 0x01; // Enable, 2^(div+1) cycles to interrupt

    sei();
  
    while(1) {
      wdr();
      if (1 == full_count) {
        //TCA0.SINGLE.CTRLA = 0x01; // Enable counting with ext clk
        
      }
      else if (stop == full_count) {
        TCA0.SINGLE.CTRLA = 0x00; // Stop
        PORTB.OUT &= ~DEBUG_1;
        break;
      }
    }

    cli();
  } while (capt < 30000 && stop < 10);

  while(RTC.PITSTATUS);
  RTC.PITCTRLA = 0x00;
  //while(RTC.PITSTATUS);
  RTC.PITINTCTRL = 0x00;
  #ifdef DEBUG
  sprintf(buffer,"capt=%u\n", capt);
  USART_Transmit_String(buffer);
  #endif

  freq_VCXO_kHz = (uint32_t)(round(((long double)(((uint32_t)0x00000002<<div)*(stop-1))/capt)*freq_ref_kHz));
  freq_VCXO_100kHz = (uint32_t)round((double)freq_VCXO_kHz/100)*100;
  diff_100kHz = freq_VCXO_kHz-freq_VCXO_100kHz;

  long double sampfact;
  uint16_t div_vcxo;
  uint8_t PIT_DIV;

  if ((freq_VCXO_100kHz > 5000) && (freq_VCXO_100kHz <= 10000)) {
    div_vcxo=512;
    PIT_DIV = EVSYS_ASYNCCH3_PIT_DIV512_gc;
  }
  else if ((freq_VCXO_100kHz > 10000) && (freq_VCXO_100kHz <= 23000)) {
    div_vcxo=1024;
    PIT_DIV = EVSYS_ASYNCCH3_PIT_DIV1024_gc;
  }
  else if ((freq_VCXO_100kHz > 23000) && (freq_VCXO_100kHz <= 46000)) {
    div_vcxo=2048;
    PIT_DIV = EVSYS_ASYNCCH3_PIT_DIV2048_gc;
  }
  else if ((freq_VCXO_100kHz > 46000) && (freq_VCXO_100kHz <= 92000)) {
    div_vcxo=4096;
    PIT_DIV = EVSYS_ASYNCCH3_PIT_DIV4096_gc;
  }
  else if (freq_VCXO_100kHz > 92000) {
    div_vcxo=8192;
    PIT_DIV = EVSYS_ASYNCCH3_PIT_DIV8192_gc;
  }
  else
    div_vcxo=EVSYS_ASYNCCH3_PIT_DIV256_gc;

  FTW = (uint32_t)(round(((long double)freq_VCXO_100kHz*(float)1000/div_vcxo)*(long double)53687.0912));
  // 2^32/80e3 = 53687.0912
  //FTW = (fout/DIV_vco)/(fref/DIV_ref)*2^32

  #ifdef DEBUG
  sprintf(buffer,"f: %lu kHz\n", freq_VCXO_kHz);
  USART_Transmit_String(buffer);
  sprintf(buffer,"fr: %lu kHz\n", freq_VCXO_100kHz);
  USART_Transmit_String(buffer);
  sprintf(buffer,"diff: %u kHz\n", diff_100kHz);
  USART_Transmit_String(buffer);
  sprintf(buffer,"FTW: %lu\n", FTW);
  USART_Transmit_String(buffer);
  #endif

  //uint16_t extra_freqs_kHz[2] = {8192, 10368, 12288, 15360, 16384, 18432, 20736, 24572, 32768, 49152}

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
  
  TCA0.SINGLE.PER = TCA_PER; // Set depending on ref freq
  //TCA0.SINGLE.PER = 0x007c;
  TCA0.SINGLE.INTCTRL = 0x01;
  TCA0.SINGLE.CTRLA = 0x01; // Div 1, enable
  
  //CPU_CCP = 0xd8;
  _PROTECTED_WRITE(CLKCTRL.XOSC32KCTRLA,0x05);  // Ext on TOSC1, Enable
  
  while(RTC.STATUS);
  RTC.CLKSEL = 0x02; // Use XOSC32K/TOSC1
  while(RTC.PITSTATUS);
  RTC.PITCTRLA = 0x01; // Enable, no interrupt

  EVSYS.ASYNCCH3 = PIT_DIV;
  //EVSYS.ASYNCCH3 = 0x0d; // PIT_DIV1024 to ch3
  EVSYS.ASYNCUSER2 = 0x06; // ch3 to CCL LUT0 EV0
  
  CCL.SEQCTRL0 = 0x00;
  CCL.LUT0CTRLB = 0x63; // input 1 ac out, input 0 LUT0 event0
  CCL.TRUTH0 = 0b00000110; // XOR
  CCL.LUT0CTRLA = 0b00001001; // enable with outen
  //CCL.LUT0CTRLA = 0b00000001; // enable
  CCL.CTRLA = 0x01;

  // Wide loop
  PORTA.DIR |= PIN2_bm;
  EVSYS.ASYNCCH1 = 0x01; // CCL LUT0
  EVSYS.ASYNCUSER8 = 0x04; // ch1 to EVOUT0/PA2
  PORTMUX.CTRLA |= 0x01; // enable EVOUT0

  _delay_us(10);
  
  sei();
  
  while(1) {
    //_delay_ms(10);
    wdr();
  }
  
}
