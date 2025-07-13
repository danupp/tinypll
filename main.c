// targets an ATtiny816

#define wdr() __asm__ __volatile__ ("wdr")

#include "pins.h"
#include "refpll.h"
#include "usart.h"

#include <avr/io.h>          
#include <avr/interrupt.h>
#include <avr/eeprom.h>
//#include <avr/sleep.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

volatile uint8_t sample = 0;

#define saved_freq_VCXO (uint32_t*)8

ISR(TCA0_OVF_vect) { // 6.7s (at 10 MHz) PIT
  TCA0.SINGLE.INTFLAGS = 0x01; // clear flag
  sample = 1;
}

int main (void) {            

  char buffer[30];
  char rxchar=0;
  uint32_t freq_VCXO;
  int16_t adval, adval_last = 0x0000;
  
  PORTA.OUT = 0x00;
  PORTB.OUT = 0x00;             
  PORTC.OUT = 0x00;
  
  PORTA.DIR = ~(RXD | COMP_IN | REF_IN | DAC_OUT);
  PORTB.DIR = ~(MONI | COMP_REF | OSC_IN);            
  PORTC.DIR = 0xff;
  
  PORTA.PIN7CTRL = 0x04; // digital input disable for COMP_IN
  PORTA.PIN6CTRL = 0x04; // digital input disable for DAC_OUT
  PORTA.PIN4CTRL = 0x04; // digital input disable for PD_OUT
  PORTB.PIN4CTRL = 0x04; // digital input disable for COMP_REF
  PORTB.PIN0CTRL = 0x04; // digital input disable for MONI
  
  ADC0.MUXPOS = 0x0b;  // MONI / AIN11
  
  _delay_ms(500);
  USART_init();

#ifdef DEBUG
   if (RSTCTRL.RSTFR & RSTCTRL_WDRF_bm) {  // Watchdog reset occured
       RSTCTRL.RSTFR |= RSTCTRL_WDRF_bm; // clear flag
       USART_Transmit_String("\nWDT restart\n");
   }
#endif

   //_PROTECTED_WRITE (CLKCTRL.MCLKCTRLB,(0x08 << 1) | 0x01); // prescaler 6

   _delay_ms(100);

   
   PORTC.OUT |= LED;
   _delay_ms(500);
   wdr();
   PORTC.OUT &= ~LED;
   _delay_ms(500);
   wdr();
   PORTC.OUT |= LED;
   _delay_ms(500);
   wdr();
   PORTC.OUT &= ~LED;
   _delay_ms(500);

   _PROTECTED_WRITE (WDT.CTRLA,0x09); // 2s WDT on
   
   USART_Transmit_String("\n***\n");
   USART_Transmit_String("Press c + enter to config.\n");

   //freq_VCXO = (((uint32_t)eeprom_read_word(FREQ_H))<<16) | (uint32_t)eeprom_read_word(FREQ_L);
   freq_VCXO = (uint32_t)eeprom_read_dword(saved_freq_VCXO); 
   USART_Transmit_String("VCXO frequency ");
   sprintf(buffer,"%lu",freq_VCXO);
   USART_Transmit_String(buffer);
   USART_Transmit_String(" loaded from EEPROM.\n");
   
   if (refpll_start(freq_VCXO)) {  // Starts refpll. If ref available turn on LED indefinitely and set up peripherals to sample voltage later. 
      wdr();
     PORTC.OUT |= LED;
      // Start TCA0 as slow PIT 
     TCA0.SINGLE.PER = 0xffff;
     TCA0.SINGLE.INTCTRL = 0x01;
     TCA0.SINGLE.CTRLA = 0x0f; // Div 1024, enable
     // Arm ADC for measurement of tuning voltage
     ADC0.MUXPOS = 0x08;  // MONI
     ADC0.CTRLC = 0x11; // vdd ref, ./4 prescaler
     ADC0.CTRLA = 0x01; // ADC enable
   }
   else
     PORTA.DIR &= ~PD_OUT;  // Tristate

   sei();

   //while(1)
   //  wdr();
   
   while(1) {
     wdr();
     if (USART_LineReceived()) {
       if(*USART_getRxBuff() == 'c' || *USART_getRxBuff() == 'C') {
	 refpll_stop();
	 USART_Transmit_String("\nEnter xosc frequency in Hz\n> ");
	 while(1) {
	   wdr();
	   if (USART_LineReceived()) {
	     sscanf(USART_getRxBuff(),"%lu",&freq_VCXO);
	     if(freq_VCXO) {
	       USART_Transmit_String("Saving ");
	       sprintf(buffer,"%lu",freq_VCXO);
	       USART_Transmit_String(buffer);
	       USART_Transmit_String(" to EEPROM and restarting.\n");
	       //eeprom_write_word(FREQ_H,(uint16_t)(freq_VCXO >> 16));
	       //eeprom_write_word(FREQ_L,(uint16_t)freq_VCXO);
	       eeprom_write_dword(saved_freq_VCXO, freq_VCXO);
	       _delay_ms(100);
	       _PROTECTED_WRITE (RSTCTRL.SWRR,0x01); // issue software reset
	     }
	   }
	 }
       }
     }
     if (sample) { // Then sample tune voltage and check if we are stable
       sample = 0;
       ADC0.COMMAND = 0x01;
       while (ADC0.COMMAND);
       adval = ADC0.RES;
       int diff = adval - adval_last;
       if(diff < 50 && diff > -50)
	 PORTC.OUT |= LED;
       else
	 PORTC.OUT &= ~LED;
       adval_last = adval;
     }
   }
}
