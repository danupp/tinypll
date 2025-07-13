#include "usart.h"

char rx_buff[30];
volatile uint8_t process_usart_flag, rx_ptr=0;

ISR (USART0_RXC_vect) {

  unsigned char udr_rx;

  /* Get and return received data from buffer */
  if (USART0.RXDATAH & USART_FERR_bm)
    udr_rx = USART0.RXDATAL;
  else {
    udr_rx = USART0.RXDATAL;
    USART0.TXDATAL = udr_rx;
    
    if (udr_rx=='\n') {
      process_usart_flag = 1;
      rx_ptr=0;
    }
    else if (rx_ptr < 28) {
      rx_buff[rx_ptr]=udr_rx;
      rx_ptr++;
    }
  }
}

void USART_init() {
// Baud rate compensated with factory stored frequency error
   int8_t sigrow_val = SIGROW.OSC20ERR3V;
   int32_t baud_reg_val = (int32_t)1389;  // 9600 baud at 3.33MHz

   baud_reg_val *= (1024 + sigrow_val);
   baud_reg_val /= 1024;
   USART0.BAUD = (int16_t) baud_reg_val;

   PORTA.DIR &= ~RXD;
   PORTA.DIR |= TXD; 

   PORTMUX.CTRLB |= 0x01; // Use alternative pins, i.e TXD on PA1
   USART0.CTRLA = 0b10000000; // RXCIE
   USART0.CTRLB = 0b11000000; // TX enable
}

void USART_Transmit_String(char *buffer) {
  uint8_t n = 0;
  while(buffer[n]) {
    wdr();
    while (!(USART0.STATUS & USART_DREIF_bm));
    USART0.TXDATAL = buffer[n];
    n++;
  }
}

char USART_getLastChar() { // return last char in buffer
  if (rx_ptr) {
    return (rx_buff[rx_ptr-1]);
  }
  return(0);
}

uint8_t USART_LineReceived() {
  uint8_t flag = process_usart_flag;

  process_usart_flag = 0;
  return(flag);
}

char* USART_getRxBuff() {
  return (rx_buff);
}
