#!/bin/bash

GCCPATH="$HOME/devel/avr/toolchain/avr8-gnu-toolchain-linux_x86_64/bin"
BPATH="$HOME/devel/avr/toolchain/Microchip.ATtiny_DFP.3.3.272.atpack_FILES/gcc/dev/attiny816"
IPATH="$HOME/devel/avr/toolchain/Microchip.ATtiny_DFP.3.3.272.atpack_FILES/include"

$GCCPATH/avr-gcc -mmcu=attiny816 -B $BPATH -I $IPATH -O1 main.c refpll.c usart.c -o tinypll.elf

$GCCPATH/avr-size tinypll.elf
$GCCPATH/avr-objcopy -O ihex tinypll.elf tinypll.hex
