#!/bin/bash

~/devel/avr/toolchain/pyupdi/pyupdi.py -d tiny816 -c /dev/ttyACM0 -f tinypll.hex -b 115200
