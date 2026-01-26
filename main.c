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

#define NOT_CONFIGURED 0
#define NO_REFERENCE 1
#define RUNNING 2

volatile uint8_t tick = 0;
char buffer[10];

#define saved_freq_ref (uint16_t*)0
#define saved_freq_VCXO (uint32_t*)8

ISR(TCA0_OVF_vect) { // 5s (at 10 MHz) PIT
  TCA0.SINGLE.INTFLAGS = 0x01; // clear flag
  tick = 1;
}

void configure() {
  uint32_t freq_VCXO=0;
  uint16_t ref_alt;
  uint16_t ref_choice;
  
  while (1) {
    wdr();
    _delay_ms(50);
    PORTC.OUT |= LED;
    _delay_ms(50);
    PORTC.OUT &= ~LED;
    if (USART_LineReceived()) {
      USART_Transmit_String("\n\rVCXO f in Hz?\n\r> ");
      while(1) {
	wdr();
	_delay_ms(50);
	PORTC.OUT |= LED;
	_delay_ms(50);
	PORTC.OUT &= ~LED;
	if (USART_LineReceived()) {
	  sscanf(USART_getRxBuff(),"%lu",&freq_VCXO);
	  if(!freq_VCXO) {
	    _PROTECTED_WRITE (RSTCTRL.SWRR,0x01); // issue software reset
	  }
	  else {
	    USART_Transmit_String("\n\rRef?\n\r");
	    USART_Transmit_String("1  - 10*\n\r");
	    USART_Transmit_String("2  - 12*\n\r");
	    USART_Transmit_String("3  - 12.8\n\r");
	    USART_Transmit_String("4  - 13\n\r");
	    USART_Transmit_String("5  - 14.4\n\r");
	    USART_Transmit_String("6  - 16\n\r");
	    USART_Transmit_String("7  - 19.2\n\r");
	    USART_Transmit_String("8  - 20*\n\r");
	    USART_Transmit_String("9  - 24\n\r");
	    USART_Transmit_String("10 - 25\n\r");
	    USART_Transmit_String("11 - 26*\n\r");
	    USART_Transmit_String("12 - 32\n\r");
	    USART_Transmit_String("13 - 36\n\r");
	    USART_Transmit_String("14 - 38.4\n\r");
	    USART_Transmit_String("15 - 40*\n\r");
	    USART_Transmit_String("16 - 48\n\r");
	    USART_Transmit_String("     MHz\n\r");
	    USART_Transmit_String("Or 0 for auto det of *\n\r> ");
	    while(1) {
	      wdr();
	      _delay_ms(50);
	      PORTC.OUT |= LED;
	      _delay_ms(50);
	      PORTC.OUT &= ~LED;
	      if (USART_LineReceived()) {
		sscanf(USART_getRxBuff(),"%u",&ref_choice);
		switch (ref_choice) {
		case 1:
		  ref_alt = 10;
		  break;
		case 2:
		  ref_alt = 12;
		  break;
		case 3:
		  ref_alt = 128;
		  break;
		case 4:
		  ref_alt = 13;
		  break;
		case 5:
		  ref_alt = 144;
		  break;
		case 6:
		  ref_alt = 16;
		  break;
		case 7:
		  ref_alt = 192;
		  break;
		case 8:
		  ref_alt = 20;
		  break;
		case 9:
		  ref_alt = 24;
		  break;
		case 10:
		  ref_alt = 25;
		  break;
		case 11:
		  ref_alt = 26;
		  break;
		case 12:
		  ref_alt = 32;
		  break;
		case 13:
		  ref_alt = 36;
		  break;
		case 14:
		  ref_alt = 384;
		  break;
		case 15:
		  ref_alt = 40;
		  break;
		case 16:
		  ref_alt = 48;
		  break;
		case 0:
		default:
		  ref_alt = 255;
		  break;
		}
		//USART_Transmit_String("Saving to EEPROM and restarting.\n\r");
		USART_Transmit_String("\n\r**\n\r");
		eeprom_write_dword(saved_freq_VCXO, freq_VCXO);
		eeprom_write_word(saved_freq_ref, ref_alt);
		wdr();
		_delay_ms(1000);
		_PROTECTED_WRITE (RSTCTRL.SWRR,0x01); // issue software reset
	      }
	    }
	  }
	}
      }
    }
  }
}  


int main (void) {            

  uint32_t freq_VCXO;
  uint16_t freq_ref;
  uint16_t adval, adval_last = 0x0000;
  uint8_t i = 0;
  uint8_t state;
  
  PORTA.OUT = 0x00;
  PORTB.OUT = 0x00;             
  PORTC.OUT = 0x00;
  
  PORTA.DIR = ~(RXD | COMP_IN | REF_IN | DAC_OUT | PD_OUT);
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
       USART_Transmit_String("\n\rWDT restart\n\r");
   }
#endif

   _delay_ms(100);

   
   PORTC.OUT |= LED;
   _delay_ms(500);
   PORTC.OUT &= ~LED;
   _delay_ms(500);
   PORTC.OUT |= LED;
   _delay_ms(500);
   PORTC.OUT &= ~LED;
   _delay_ms(500);

   state = NOT_CONFIGURED;
   
   USART_Transmit_String("\n\r***\n\r");
   USART_Transmit_String("Press ent to conf\n\r");

   //freq_VCXO = (((uint32_t)eeprom_read_word(FREQ_H))<<16) | (uint32_t)eeprom_read_word(FREQ_L);
   freq_VCXO = (uint32_t)eeprom_read_dword(saved_freq_VCXO);
   freq_ref = (uint16_t)eeprom_read_word(saved_freq_ref);
   
   if (freq_VCXO != 0xFFFFFFFF) {
     USART_Transmit_String("VCXO ");
     sprintf(buffer,"%lu",freq_VCXO);
     USART_Transmit_String(buffer);
     USART_Transmit_String(" Hz\n\rref ");
     if (freq_ref < 100) {
       sprintf(buffer,"%u",freq_ref);
       USART_Transmit_String(buffer);
       USART_Transmit_String(" MHz\n\r");
     }
     else if (freq_ref != 255) {
       sprintf(buffer,"%u.%u",freq_ref/10, freq_ref%10);
       USART_Transmit_String(buffer);
       USART_Transmit_String(" MHz\n\r");
     }
     else {
       USART_Transmit_String("auto\n\r");
     }
     _delay_ms(100);
     if (refpll_start(freq_VCXO, freq_ref)) {  // Starts refpll. If ref available turn on LED indefinitely and set up peripherals to sample voltage later. 
       // Start TCA0 as slow PIT
       PORTA.DIR |= PD_OUT;  // Set to output
       // Arm ADC for measurement of tuning voltage
       /* ADC0.MUXPOS = 0x0b;  // MONI */
       /* ADC0.CTRLC = 0x11; // vdd ref, ./4 prescaler */
       /* ADC0.CTRLA = 0x01; // ADC enable */
       state = RUNNING;
       PORTC.OUT |= LED;
     }
     else {
       state = NO_REFERENCE;
     
       TCA0.SINGLE.PER = 0xaa00;
       TCA0.SINGLE.INTCTRL = 0x01;
       TCA0.SINGLE.CTRLA = 0x0f; // Div 1024, enable  
     }
   }

   _PROTECTED_WRITE (WDT.CTRLA,0x09); // 2s WDT on
   
   sei();
   
   while(1) {
     wdr();
     if (USART_RXed()) {
       refpll_stop();
       _PROTECTED_WRITE (CLKCTRL.MCLKCTRLB,(0x08 << 1) | 0x01); // prescaler 6
       _PROTECTED_WRITE (CLKCTRL.MCLKCTRLA,0x00); // OSC20M for CPU
       TCA0.SINGLE.CTRLA = 0x00;
       configure();
     }
     if (tick) { 
       tick = 0;
       /* if (state == RUNNING) { // Then sample tune voltage and check if we are stable */
       /* 	 ADC0.COMMAND = 0x01; */
       /* 	 while (ADC0.COMMAND); */
       /* 	 adval = ADC0.RES; */
       /* 	 //USART0.BAUD = (int16_t) 4167;  // 9600 baud at 10 MHz */
       /* 	 //sprintf(buffer,"%X\n\r",adval); */
       /* 	 //USART_Transmit_String(buffer); */
       /* 	 int diff = adval - adval_last; */
       /* 	 if(diff < 50 && diff > -50) */
       /* 	   PORTC.OUT |= LED; */
       /* 	 else */
       /* 	   PORTC.OUT &= ~LED; */
       /* 	 adval_last = adval; */
       /* } */
       /* else */
       if (state == NO_REFERENCE) {
	 _PROTECTED_WRITE (RSTCTRL.SWRR,0x01); // issue software reset
       }
     }
   }
}
