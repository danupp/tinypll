#include "refpll.h"
#include "usart.h"

uint32_t FTW, nco_reg;

const uint8_t costable[256] =
  {254, 254, 254, 254, 253, 253, 253, 252, 252, 251, 250, 249, 249, 248, 247, 245, 244, 243, 242, 240, 239, 238, 236, 234, 233, 231, 229, 227, 225, 223, 221, 219, 217, 215, 212, 210, 208, 205, 203, 200, 198, 195, 192, 190, 187, 184, 181, 178, 176, 173, 170, 167, 164, 161, 158, 155, 152, 149, 146, 143, 139, 136, 133, 130, 127, 124, 121, 118, 115, 111, 108, 105, 102, 99, 96, 93, 90, 87, 84, 81, 78, 76, 73, 70, 67, 64, 62, 59, 56, 54, 51, 49, 46, 44, 42, 39, 37, 35, 33, 31, 29, 27, 25, 23, 21, 20, 18, 16, 15, 14, 12, 11, 10, 9, 7, 6, 5, 5, 4, 3, 2, 2, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 2, 2, 3, 4, 5, 5, 6, 7, 9, 10, 11, 12, 14, 15, 16, 18, 20, 21, 23, 25, 27, 29, 31, 33, 35, 37, 39, 42, 44, 46, 49, 51, 54, 56, 59, 62, 64, 67, 70, 73, 76, 78, 81, 84, 87, 90, 93, 96, 99, 102, 105, 108, 111, 115, 118, 121, 124, 127, 130, 133, 136, 139, 143, 146, 149, 152, 155, 158, 161, 164, 167, 170, 173, 176, 178, 181, 184, 187, 190, 192, 195, 198, 200, 203, 205, 208, 210, 212, 215, 217, 219, 221, 223, 225, 227, 229, 231, 233, 234, 236, 238, 239, 240, 242, 243, 244, 245, 247, 248, 249, 249, 250, 251, 252, 252, 253, 253, 253, 254, 254, 254};

ISR(TCB0_INT_vect) { // ~70 kHz @ 10 MHz
  TCB0.INTFLAGS = 0x01; // clear flag
  PORTA.OUT |= TP1;
  DAC0.DATA = costable[nco_reg >> 24];
  nco_reg += FTW;
  PORTA.OUT &= ~TP1;
}

uint8_t refpll_start (uint32_t freq_VCXO ) {

  uint8_t cpudiv;
  uint16_t freq_ref_kHz = 0, timer_top;
  long double factor;

#ifdef DEBUG
  char buffer[30];
  USART_Transmit_String("Refpll.\n");
#endif
  
  // Use RTC as counter for EXTCLOCK to determine if there is a reference frequency

#ifdef DEBUG
  sprintf(buffer,"Ext ref?\n", RTC.CNT);
  USART_Transmit_String(buffer);
#endif

  while(RTC.STATUS);
  RTC.CLKSEL = 0x03; // Use EXTCLK
  //while(RTC.PITSTATUS);
  RTC.CNT = 0;
  RTC.CTRLA = (0xd << 3) | 0x01; // Enable, 8192 div
  _delay_ms(100);
  RTC.CTRLA = 0x00; // stop
  
#ifdef DEBUG
  sprintf(buffer,"RTC.CNT: %u\n", RTC.CNT);
  USART_Transmit_String(buffer);
#endif

  if (RTC.CNT > 50 && RTC.CNT < 70) {
    freq_ref_kHz = 5000;
    USART_Transmit_String("Ref at 5 MHz found.\n");
    cpudiv = 0x00; // 1
    timer_top = (uint16_t)0x0090; // 5e6/(0x90+1) = 34.482.. kHz
    factor = 124554.051584; // 2^32/(5e6/(0x90+1))
   }
  else if (RTC.CNT > 100 && RTC.CNT < 135) {
    freq_ref_kHz = 10000;
    USART_Transmit_String("Ref at 10 MHz found.\n");
    cpudiv = 0x00; // 1
    timer_top = (uint16_t)0x0090; // 10e6/(0x90+1) = 68.965.. kHz
    factor = 62277.025792; // 2^32/(10e6/(0x90+1))
   }
  else if (RTC.CNT > 134  && RTC.CNT < 160) {
    freq_ref_kHz = 12000;
    USART_Transmit_String("Ref at 12 MHz found.\n");
    cpudiv = 0x00; // 1
    timer_top = (uint16_t)0x0095; // 12e6/(0x95+1) = 80 kHz
    factor = 53687.0912; // 2^32/80e3 = 53687.0912
  }
  else if (RTC.CNT > 200 && RTC.CNT < 300) {
    freq_ref_kHz = 20000;
    USART_Transmit_String("Ref at 20 MHz found.\n");
    cpudiv = 0x01; // 2
    timer_top = (uint16_t)0x0090; // 10e6/(0x90+1) = 68.965.. kHz
    factor = 62277.025792; // 2^32/(10e6/(0x90+1))
  }
  else if (RTC.CNT > 300 && RTC.CNT < 350) {
    freq_ref_kHz = 26000;
    USART_Transmit_String("Ref at 26 MHz found.\n");
    cpudiv = 0x01; // 2
    timer_top = (uint16_t)0x095; // 13e6/(0x95+1) = 86.666 kHz
    factor = 49557.31450; // 2^32/(13e6/(0x95+1))
  }
  else if (RTC.CNT > 430 && RTC.CNT < 520) {
    freq_ref_kHz = 40000;
    USART_Transmit_String("Ref at 40 MHz found.\n");
    cpudiv = 0x03; // 4
    timer_top = (uint16_t)0x0090; // 10e6/(0x90+1) = 68.965.. kHz
    factor = 62277.025792; // 2^32/(10e6/(0x90+1))
  }
  else {  // no valid ref
    USART_Transmit_String("No valid ref found.\n");
    return (0);
  }

#ifdef DEBUG
  //USART0.BAUD = (int16_t) 4167;  // 9600 bauds at 10 MHz
  sprintf(buffer,"Ref detected at: %u kHz\n", freq_ref_kHz);
  USART_Transmit_String(buffer);
#endif
  
  _PROTECTED_WRITE(CLKCTRL.XOSC32KCTRLA,0x05);  // Ext on TOSC1, Enable
  
  while(RTC.STATUS);
  RTC.CLKSEL = 0x02; // Use XOSC32K/TOSC1 - i.e. VCXO
  
  uint16_t div_vcxo;
  uint8_t PIT_DIV;

  if ((freq_VCXO > 5000000) && (freq_VCXO <= 10000000)) {
    if (freq_ref_kHz == 5000 ) {
      div_vcxo=1024;
      PIT_DIV = EVSYS_ASYNCCH3_PIT_DIV1024_gc;
    }
    else {
      div_vcxo=512;
      PIT_DIV = EVSYS_ASYNCCH3_PIT_DIV512_gc;
    }
  }
  else if ((freq_VCXO > 10000000) && (freq_VCXO <= 23000000)) {
    if (freq_ref_kHz == 5000 ) {
      div_vcxo=2048;
      PIT_DIV = EVSYS_ASYNCCH3_PIT_DIV2048_gc;
    }
    else {
      div_vcxo=1024;
      PIT_DIV = EVSYS_ASYNCCH3_PIT_DIV1024_gc;
    }
  }
  else if ((freq_VCXO > 23000000) && (freq_VCXO <= 46000000)) {
    if (freq_ref_kHz == 5000 ) {
      div_vcxo=4096;
      PIT_DIV = EVSYS_ASYNCCH3_PIT_DIV4096_gc;
    }
    else {
      div_vcxo=2048;
      PIT_DIV = EVSYS_ASYNCCH3_PIT_DIV2048_gc;
    }
  }
  else if ((freq_VCXO > 46000000) && (freq_VCXO <= 92000000)) {
    if (freq_ref_kHz == 5000 ) {
      div_vcxo=8192;
      PIT_DIV = EVSYS_ASYNCCH3_PIT_DIV8192_gc;
    }
    else {
      div_vcxo=4096;
      PIT_DIV = EVSYS_ASYNCCH3_PIT_DIV4096_gc;
    }
  }
  else if (freq_VCXO > 92000000) {
    div_vcxo=8192;
    PIT_DIV = EVSYS_ASYNCCH3_PIT_DIV8192_gc;
  }
  else {
    if (freq_ref_kHz == 5000 ) {
      div_vcxo=512;
      PIT_DIV = EVSYS_ASYNCCH3_PIT_DIV512_gc;
    }
    else {
      div_vcxo=256;
      PIT_DIV=EVSYS_ASYNCCH3_PIT_DIV256_gc;
    }
  }

  FTW = (uint32_t)(round(((long double)freq_VCXO/div_vcxo)*factor));
  
#ifdef DEBUG
  sprintf(buffer,"FTW: %lu \n", FTW);
  USART_Transmit_String(buffer);
#endif

  _PROTECTED_WRITE (CLKCTRL.MCLKCTRLB,cpudiv); // Prescaler according to ref freq
  _PROTECTED_WRITE (CLKCTRL.MCLKCTRLA,0x03); // EXTCLK for CPU

  _delay_ms(100);

  VREF.CTRLA = 0b00000010; // 2.5V for DAC0 and AC0
  DAC0.CTRLA = 0b01000001; // OUTEN, ENABLE 

  AC0.MUXCTRLA = 0b00000001; // Use AIN1 and AINP0
  //AC0.MUXCTRLA = 0b00000010; // Use VREF and AINP0
  AC0.CTRLA = 0b00000011; // pos edge, 10 mV hyst, AC enable
  
  _PROTECTED_WRITE(CLKCTRL.XOSC32KCTRLA,0x05);  // Ext on TOSC1, Enable

  while(RTC.STATUS);
  RTC.CLKSEL = 0x02; // Use XOSC32K/TOSC1
  while(RTC.PITSTATUS);
  RTC.PITCTRLA = 0x01; // Enable, no interrupt

  EVSYS.ASYNCCH3 = PIT_DIV;
  EVSYS.ASYNCUSER2 = 0x06; // ch3 to CCL LUT0 EV0
  
  CCL.SEQCTRL0 = 0x00;
  CCL.LUT0CTRLB = 0x63; // input 1 ac out, input 0 LUT0 event0
  CCL.TRUTH0 = 0b00000110; // XOR
  CCL.LUT0CTRLA = 0b00001001; // enable with outen
  CCL.CTRLA = 0x01;

  TCB0.CCMP = timer_top; // Set depending on ref freq
  TCB0.INTCTRL = 0x01; // interrupt on
  TCB0.CTRLA = 0x01; // div 1, enable
  
  return(1);
}

void refpll_stop() {
  TCB0.INTCTRL = 0x00; // interrupt off
}
