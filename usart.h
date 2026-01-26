#include "pins.h"
#include <avr/io.h>
#include <avr/interrupt.h>

#define wdr() __asm__ __volatile__ ("wdr")

void USART_init();
void USART_Transmit_String(char*);
uint8_t USART_LineReceived();
char USART_getLastChar();
char* USART_getRxBuff();
uint8_t USART_RXed();
