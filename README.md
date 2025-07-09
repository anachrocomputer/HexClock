![Static Badge](https://img.shields.io/badge/MCU-ATmega328-green "MCU:ATmega328")
![Static Badge](https://img.shields.io/badge/BOARD-Arduino-green "BOARD:Arduino")
![Static Badge](https://img.shields.io/badge/DISPLAY-HMDL2416-green "DISPLAY:HMDL2416")
![Static Badge](https://img.shields.io/badge/DISPLAY-LedHex-green "DISPLAY:LedHex")

# HexClock #

This Arduino sketch implements a clock with four different display formats:
* Ordinary hours, minutes, and optionally seconds
* Octal time, 00000 to 07777
* Hexadecimal time, 0x0000 to 0xFFFF
* Decimal or percentage time, 00.00 to 99.99

The actual display is optional at present,
as four 7-segment digits on an LED display.
Six digits would only be needed to show seconds in standard time format.

## Optional Display ##

A \#define in the code SERIAL_OUTPUT should be defined to select standard
Arduino serial output at 9600 baud.
This is more useful for debugging,
but cannot co-exist with the HMDL2416 LED display.

If that \#define is not present,
the sketch will send the time in hex to either an HP HMDL2416 16-segment
four-digit display or to four PCBs of my own design ("LedHex").
The HMDL2416 display is connected to the ATmega PORT D,
and the control signals are connected to Arduino digital I/O pins.

## User Controls ##

The pushbutton on Arduino A0 is used to switch between display modes
at run-time.
Pressing it will cycle through from the inital HEX to OCTAL,
DECIMAL, and STANDARD (then back to HEX).

No user controls are provided to set the time.

## Example Times ##

| Ordinary | Hexadecimal | Octal | Percentage |
|----------|-------------|-------|------------|
| 00:00:00 | 0000        | 0000  | 00.00      |
| 01:30:00 | 1000        | 0400  | 06.25      |
| 03:00:00 | 2000        | 1000  | 12.50      |
| 04:30:00 | 3000        | 1400  | 18.75      |
| 06:00:00 | 4000        | 2000  | 25.00      |
| 07:30:00 | 5000        | 2400  | 31.25      |
| 09:00:00 | 6000        | 3000  | 37.50      |
| 10:30:00 | 7000        | 3400  | 43.75      |
| 12:00:00 | 8000        | 4000  | 50.00      |
| 13:30:00 | 9000        | 4400  | 56.25      |
| 15:00:00 | A000        | 5000  | 62.50      |
| 16:30:00 | B000        | 5400  | 68.75      |
| 18:00:00 | C000        | 6000  | 75.00      |
| 19:30:00 | D000        | 6400  | 81.25      |
| 21:00:00 | E000        | 7000  | 87.50      |
| 22:30:00 | F000        | 7400  | 93.75      |
| 23:59:59 | FFFF        | 7777  | 99.99      |

### Pinout ###

| Arduino | Signal  | HMDL2416 | HMDL2416 Pin | DS3231 Signal |
|---------|---------|----------|--------------|---------------|
| D10     | D10     | /CE1     | 1            |               |
| GND     | GND     | /CE2     | 2            |               |
| +5V     | Vcc     | /CLR     | 3            |               |
| GND     | GND     | CUE      | 4            |               |
| +5V     | Vcc     | /CU      | 5            |               |
| D7      | D7      | /WR      | 6            |               |
| D9      | D9      | A1       | 7            |               |
| D8      | D8      | A0       | 8            |               |
| +5V     | Vcc     | Vcc      | 9            | Vcc           |
| GND     | GND     | GND      | 10           | GND           |
| D0      | TxD     | D0       | 11           |               |
| D1      | RxD     | D1       | 12           |               |
| D2      | D2      | D2       | 13           |               |
| D3      | D3      | D3       | 14           |               |
| D4      | D4      | D4       | 15           |               |
| D5      | D5      | D5       | 16           |               |
| D6      | D6      | D6       | 17           |               |
| +5V     | Vcc     | /BL      | 18           |               |
| A0      | MODESW  |          |              |               |
| A4/D18  | I2C SDA |          |              | I2C SDA       |
| A5/D19  | I2C SCL |          |              | I2C SCL       |

MODESW is connected to a 10k pull-up resistor and a tact-switch to GND.
It's used as a digital input, not analog (Arduino digital pin 14).

## Real-Time Clock ##

A non-volatile real-time clock module using the DS3231 should be connected
to the Arduino's I2C pins (A4 and A5) along with 10k pull-up resistors.
This will be used to read the time and date at start-up.

A line of code to set the DS3231 is included in 'setup()',
commented-out.

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
Would be nice to support those if I had some.

Synchronise internal Arduino time with the (more accurate) DS3231 I2C
real-time clock chip.
Probably best to do this every 90 minutes/6.25%/0x1000/0400,
i.e. 16 times per day.

Change the parallel display wiring to allow use of serial pins D0 and D1 at
the same time as the HMDL2416 display is connected.
This should eliminate the need for #define SERIAL_OUTPUT.

Find a better way to set the time without needing to recompile the code.

Implement other types of serially-interfaced display: TM1622,
MAX7219, I2C 14-segment.

Advanced goal: make this into a watch.
Round OLED display?
Tiny 7-segment?
LCD for low power?

