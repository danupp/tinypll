#include "pins.h"

#include <util/delay.h>
#include <stdint.h>
#include <avr/io.h>          
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>

#ifdef DEBUG
#include "usart.h"
#include <stdio.h>
#include <string.h>
#endif

#define wdr() __asm__ __volatile__ ("wdr")

uint8_t refpll_start(uint32_t);

void refpll_stop();
