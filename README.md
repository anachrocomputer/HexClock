![Static Badge](https://img.shields.io/badge/MCU-ATmega328-green "MCU:ATmega328")
![Static Badge](https://img.shields.io/badge/BOARD-Arduino-green "BOARD:Arduino")

# HexClock #

This Arduino sketch implements a clock with four different display formats:
* Ordinary hours, minutes, and seconds
* Octal time, 00000 to 07777
* Hexadecimal time, 0x0000 to 0xFFFF
* Decimal or percentage time, 00.00 to 99.99

The actual display will be added later as four 7-segment digits
or four decimal digit displays.
Six digits would only be needed to show seconds in standard time format.

## TODO ##

The display will work with any type of 7-segment display,
e.g. vacuum fluorescent, LED, LCD,
Panaplex, Numitron,
electromechanical flip-type.
If we limit ourselves to octal times (0000-7777) then the display will be
compatible with Nixie tubes,
edge-lit digits,
projection displays,
and any other type of decimal (0-9) digit displays.

Some Numitron displays have Russian Postcode digits,
with two additional diagonal segments within the usual 7-segment arrangement.

## Example Times ##

| Ordinary | Hexadecimal | Octal | Percentage |
|----------|-------------|-------|------------|
| 00:00:00 | 0000        | 0000  | 00.00      |
| 03:00:00 | 2000        | 1000  | 12.50      |
| 06:00:00 | 4000        | 2000  | 25.00      |
| 09:00:00 | 6000        | 3000  | 37.50      |
| 12:00:00 | 8000        | 4000  | 50.00      |
| 15:00:00 | A000        | 5000  | 62.50      |
| 18:00:00 | C000        | 6000  | 75.00      |
| 21:00:00 | E000        | 7000  | 87.50      |
| 23:59:59 | FFFF        | 7777  | 99.99      |

