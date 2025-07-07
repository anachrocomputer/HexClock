/* HexClock --- display time in hex and percentage          2024-06-14 */

#define SERIAL_OUTPUT

#define DISPLAY_HEXLED

/*
 * Hex time runs from 0x0000 at midnight to 0x8000 at midday and then 
 * to 0xFFFF just before the next midnight. 6am is 0x4000 and 6pm 
 * is 0xC000.
 * 
 * One hexond is 1.318359375 seconds (86400 / 65536).
 * 
 * Percentage time (or decimal time) is 00.00 at midnight and 50.00
 * at midday, counting up to 99.99 just before the next midnight.
 * 6am is 25.00 and 6pm is 75.00.
 * 
 * 0.01% of a day is 100 uDays or 8.64 seconds (86400 / 10000).
 * 
 * Hex time ticks up at exactly the same instant as ordinary seconds
 * every 675 seconds or eleven and a quarter minutes. This corresponds
 * to 512 hexonds. This conjunction happens 128 times per day.
 * 
 * Percentage time ticks up at exactly the same instant as ordinary
 * seconds every 216 seconds or 3 minutes 36 seconds. This corresponds
 * to 0.25% of a day, making the conjunction occur 400 times per day.
 * 
 * Hex and percentage conjunctions can occur at exactly the same time,
 * e.g. at 6am, midday, and 6pm. These double conjunctions happen 16
 * times per day, every 90 minutes.
 */

#define SECONDS_PER_DAY (24UL * 60UL * 60UL)  // 86400
#define USEC_PER_HEXOND  (1318359) //  1.318359 seconds
#define USEC_PER_OCTOND (21093750) // 21.09375 seconds
#define USEC_PER_SECOND  (1000000) //  1.0 seconds

#define NDIGITS    (4)   // This program uses a four-digit display

#ifdef DISPLAY_HEXLED
// Pin connections from LED board to Arduino board
// 1 VLED +5V (LED power, OK to use Arduino +5V for a single digit)
// 2 VCC  +5V (TLC5916 chip power)
// 3 SDI  D11 (Serial Data In on TLC5916)
// 4 SDO  no connection (Serial Data out on TLC5916, unused)
// 5 LE   D10 (Latch Enable on TLC5916)
// 6 OE   GND (Output Enable, unused in this test, GND to enable)
// 7 SCK  D13 (Serial Clock to TLC5916)
// 8 GND  GND (Ground to both LEDs and chip)
//
#define LE_PIN 10   // Arduino digital pin 10
#define SDA_PIN 11  // Arduino digital pin 11
#define SCK_PIN 13  // Arduino digital pin 13
/* OE pin grounded */

#define MODE_PIN (14)    // Pin A0 is also D14

// 'F' is #defined as a macro by Arduino framework. Use an int instead
const int F = (1 << 14);

// Usual 7-segment names
#define A  (1 << 8)
#define B  (1 << 0)
#define C  (1 << 2)
#define D  (1 << 4)
#define E  (1 << 11)
//#define F  (1 << 14)
#define G  (1 << 10)
#define DP (1 << 12)

// Additional dot names
#define H (1 << 1)
#define I (1 << 3)
#define J (1 << 9)
#define K (1 << 6)
#define L (1 << 15)
#define M (1 << 13)
#define COLON (1 << 5)

// LED layout on PCB:
//
//     H C C I
//     B     D
//     B     D
//     J G G K
//     A     E
//     A     E
//     L F F M DP

// Table of 14-segment digits 0-9 and A-F
const unsigned int HDSPsegtab[16] = {
  A | B | C | D | E | F | J | K,         // 0
  D | E | I | K | M,                     // 1
  A | C | D | F | G | H | L | M,         // 2
  C | D | E | F | G | H | L,             // 3
  B | D | E | G | H | J | K | M,         // 4
  B | C | E | F | G | H | I | J | L,     // 5
  A | B | C | E | F | G | J,             // 6
  C | D | E | H | I | K | M,             // 7
  A | B | C | D | E | F | G,             // 8
  B | C | D | E | F | G | K,             // 9
  A | B | C | D | E | G | J | K | L | M, // A
  A | B | C | D | E | F | G | H | J | L, // B
  A | B | C | F | I | J | M,             // C
  A | B | C | D | E | F | H | J | K | L, // D
  A | B | C | F | G | H | I | J | L | M, // E
  A | B | C | G | H | I | J | L          // F
};
#else
// Direct port I/O for HMDL2416 display
#include <avr/io.h>

#define WR_PIN 7
#define A0_PIN 8
#define A1_PIN 9
#define CS_PIN 10
#endif

// I2C setup for DS3231 real-time clock chip
#include <Wire.h>

#define DS3231_ADDR    (0x68)

char *Dayname[8] = {
  "", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"
};

enum {HEX_DISPLAY, OCT_DISPLAY, DEC_DISPLAY, STD_DISPLAY};

uint32_t SecondsPastMidnight = 0UL;
uint16_t OctTime = 0U;
uint16_t HexTime = 0U;
uint16_t DecTime = 0U;


uint32_t asSeconds(const int hours, const int minutes, const int seconds)
{
  return ((uint32_t)hours * (60UL * 60UL)) + ((uint32_t)minutes * 60UL) + (uint32_t)seconds;
}


void ShowTime(const int displayFormat, const bool hexConjunction, const bool decConjunction)
{
  int i;
  int hour, minute, second;
  char buf[64];
#ifdef DISPLAY_HEXLED
  unsigned int digit;
  unsigned int segs[NDIGITS];           // 16-bit unsigned binary
#endif

  const char o = ' ';
  const char h = hexConjunction? '*': ' ';
  const char d = decConjunction? '*': ' ';
  
  hour = SecondsPastMidnight / (60UL * 60UL);
  minute = (SecondsPastMidnight - (hour * 60UL * 60UL)) / 60UL;
  second = SecondsPastMidnight % 60;

#ifdef SERIAL_OUTPUT
  snprintf(buf, sizeof (buf), "%04x %c %04o %c %02d:%02d:%02d %c %04d\n", HexTime, h, OctTime, o, hour, minute, second, d, DecTime);
  Serial.print(buf);
  Serial.flush();
#endif

  switch (displayFormat)
  {
  case OCT_DISPLAY:
    snprintf(buf, sizeof (buf), "%04o", OctTime);
    break;
  case HEX_DISPLAY:
    snprintf(buf, sizeof (buf), "%04X", HexTime);
    break;
  case DEC_DISPLAY:
    snprintf(buf, sizeof (buf), "%04d", DecTime);
    break;
  case STD_DISPLAY:
    snprintf(buf, sizeof (buf), "%02d%02d", hour, minute);
    break;
  }
  
  for (i = 0; i < NDIGITS; i++) {
#ifdef DISPLAY_HEXLED
    if (buf[i] > '9')
      digit = buf[i] - 'A' + 10;
    else
      digit = buf[i] - '0';
      
    segs[i] = HDSPsegtab[digit];   // Pick up segment pattern from table
#else
    HMDL2416Write(3 - i, buf[i]);
#endif
  }

#ifdef DISPLAY_HEXLED
  switch (displayFormat) {
    case OCT_DISPLAY:
    case HEX_DISPLAY:
      break;
    case DEC_DISPLAY:
      segs[1] |= DP;
      break;
    case STD_DISPLAY:
      segs[1] |= COLON;
      break;
  }
  
  LedHex_send(segs, NDIGITS);     // Send to displays
#endif
}


#ifdef DISPLAY_HEXLED
/* LedHex_send --- send 'ndig' 16-bit patterns to the LED driver chip */

void LedHex_send(const unsigned int leds[], const int ndig)
{
  // Straightforward implementation using digitalWrite(). Could be made
  // faster by using direct port I/O or SPI hardware.
  int i;
  int d;
  
  digitalWrite(LE_PIN, LOW);
  digitalWrite(SCK_PIN, LOW);

  // Send 16 bits to each pair of TLC5916 chips per digit
  for (d = 0; d < ndig; d++) {
    for (i = 0; i < 16; i++) {
      // Send one bit on SDA pin
      if (leds[d] & (1u << i))
        digitalWrite(SDA_PIN, HIGH);
      else
        digitalWrite(SDA_PIN, LOW);
      
      delayMicroseconds(1);

      // One microsecond HIGH pulse on SCK to clock the bit along
      digitalWrite(SCK_PIN, HIGH);
      
      delayMicroseconds(1);
      
      digitalWrite(SCK_PIN, LOW);  
    }
  }

  // One microsecond HIGH pulse on LE to latch all bits into all the digits
  digitalWrite(LE_PIN, HIGH);
  
  delayMicroseconds(1);
    
  digitalWrite(LE_PIN, LOW);
}
#else
void HMDL2416Write(int digit, int ch)
{
  switch (digit) {
  case 0:
    digitalWrite(A0_PIN, LOW);
    digitalWrite(A1_PIN, LOW);
    break;
  case 1:
    digitalWrite(A0_PIN, HIGH);
    digitalWrite(A1_PIN, LOW);
    break;
  case 2:
    digitalWrite(A0_PIN, LOW);
    digitalWrite(A1_PIN, HIGH);
    break;
  case 3:
    digitalWrite(A0_PIN, HIGH);
    digitalWrite(A1_PIN, HIGH);
    break;
  }
  
  PORTD = ch | 0x80;
  
  digitalWrite(CS_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(WR_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(WR_PIN, HIGH);
  delayMicroseconds(2);
  digitalWrite(CS_PIN, HIGH);
}
#endif


int bcd2bin(int bcd)
{
  return (((bcd >> 4) * 10) + (bcd & 0x0f));
}


int bin2bcd(int bin)
{
  return (((bin / 10) << 4) + (bin % 10));
}


void DS3231Set(int year, int month, int day, int hour, int minute, int second, int dow)
{
  byte ds3231[0x13];
  int reg;
  
  ds3231[0] = bin2bcd(second);
  ds3231[1] = bin2bcd(minute);
  ds3231[2] = bin2bcd(hour);
  ds3231[3] = bin2bcd(dow);
  ds3231[4] = bin2bcd(day);
  ds3231[5] = bin2bcd(month);
  ds3231[6] = bin2bcd(year % 100);
  
  if (year > 1999)
    ds3231[5] |= 0x80;
  
  for (reg = 0; reg < 0x07; reg++) {  
    Wire.beginTransmission(DS3231_ADDR);
    Wire.write((byte)reg);
    Wire.write(ds3231[reg]);
    Wire.endTransmission();
  }
}


void setup(void)
{
  int reg, val;
  byte ds3231[0x13];
  int year, month, day, dow;
  int hour, minute, second;
  char buf[64];

#ifdef SERIAL_OUTPUT
  Serial.begin(9600);
#endif
#ifdef DISPLAY_HEXLED
  pinMode(LE_PIN, OUTPUT);
  pinMode(SDA_PIN, OUTPUT);
  pinMode(SCK_PIN, OUTPUT);
  
  digitalWrite(LE_PIN, LOW);
  digitalWrite(SCK_PIN, LOW);
#else
  int i;
  
  pinMode(WR_PIN, OUTPUT);
  pinMode(A0_PIN, OUTPUT);
  pinMode(A1_PIN, OUTPUT);
  pinMode(CS_PIN, OUTPUT);
  
  for (i = 0; i < 7; i++)
    pinMode(i, OUTPUT);
  
  digitalWrite(WR_PIN, HIGH);
  digitalWrite(CS_PIN, HIGH);
  digitalWrite(A0_PIN, LOW);
  digitalWrite(A1_PIN, LOW);
  
  for (i = 0; i < 7; i++)
    digitalWrite(i, LOW);
#endif

  // Input pin for mode selection
  pinMode(MODE_PIN, INPUT);
  
  Wire.begin();
  //DS3231Set (2025, 7, 7, 21, 30, 00, 1);

  for (reg = 0; reg < 0x13; reg++) {  
    Wire.beginTransmission(DS3231_ADDR);
    Wire.write((byte)reg);
    Wire.endTransmission();
  
    Wire.requestFrom(DS3231_ADDR, 1);
    val = Wire.read();
    Wire.endTransmission();

    if (val >= 0)
      ds3231[reg] = val;
    else {
      ds3231[reg] = 0;
      Serial.print("DS3231 reg ");
      Serial.print(reg);
      Serial.println(": read failed");
    }
    
    //Serial.print("reg = ");
    //Serial.print(reg, HEX);
    //Serial.print(", val = ");
    //Serial.println(val, HEX);
  }
  
  second = bcd2bin(ds3231[0]);
  minute = bcd2bin(ds3231[1]);
  hour = bcd2bin(ds3231[2] & 0x3f);
  
  dow = ds3231[3] & 0x07;
  
  day = bcd2bin(ds3231[4]);
  month = bcd2bin(ds3231[5] & 0x1f);
  year = bcd2bin(ds3231[6]);
  
  if (ds3231[5] & 0x80)
    year += 2000;
  else
    year += 1900;
  
#ifdef SERIAL_OUTPUT
  snprintf(buf, 64, "%04d-%02d-%02dT%02d:%02d:%02d %s\n",
                     year, month, day, hour, minute, second, Dayname[dow]);
  
  Serial.print(buf);
#endif

  // Start just before midnight to test wrap-around
  //hour = 23;
  //minute = 59;
  //second = 0;

  SecondsPastMidnight = asSeconds(hour, minute, second);

  // We really need to write:
  // HexTime = ((SecondsPastMidnight * 65536UL) + (SECONDS_PER_DAY / 2UL)) / SECONDS_PER_DAY;
  // but this causes a 32-bit integer overflow. So we divide both
  // constants by 128 to get 512 and 675
  OctTime = ((SecondsPastMidnight * 4096UL) + (SECONDS_PER_DAY / 2UL)) / SECONDS_PER_DAY;
  HexTime = ((SecondsPastMidnight * 512UL) + (675UL / 2UL)) / 675UL;
  DecTime = ((SecondsPastMidnight * 10000UL) + (SECONDS_PER_DAY / 2UL)) / SECONDS_PER_DAY;
}


void loop(void)
{
  uint32_t oPeriod;
  uint32_t uPeriod;
  uint32_t mPeriod;
  uint32_t dPeriod;
  uint32_t now;
  bool octUpdate = false;
  bool hexUpdate = false;
  bool decUpdate = false;
  bool stdUpdate = false;
  int mode;
  int oldMode;
  int displayFormat = HEX_DISPLAY;

  now = micros();
  
  oPeriod = now;
  uPeriod = now;
  mPeriod = now;
  dPeriod = now;

  mode = digitalRead(MODE_PIN);
  oldMode = mode;
  
  while (1) {
    now = micros();
    
    if ((now - oPeriod) > USEC_PER_OCTOND) {
      oPeriod = now;
      if (OctTime < 4095u)
        OctTime++;
      else
        OctTime = 0u;
      octUpdate = true;
      //Serial.println("OCTOND");
    }
    
    if ((now - uPeriod) > USEC_PER_HEXOND) {
      uPeriod = now;
      if (HexTime < 65535u)
        HexTime++;
      else
        HexTime = 0u;
      hexUpdate = true;
      //Serial.println("HEXOND");
    }

    if ((now - mPeriod) > USEC_PER_SECOND) {
      mPeriod = now;
      if (SecondsPastMidnight < (SECONDS_PER_DAY - 1))
        SecondsPastMidnight++;
      else
        SecondsPastMidnight = 0UL;
      stdUpdate = true;
      //Serial.println("SECOND");
    }

    if ((now - dPeriod) > 8640000UL) {
      dPeriod = now;
      if (DecTime < 9999u)
        DecTime++;
      else
        DecTime = 0u;
      decUpdate = true;
      //Serial.println("100 uDays");
    }

    if (octUpdate || hexUpdate || decUpdate || stdUpdate) {
      ShowTime(displayFormat, (SecondsPastMidnight % 675UL) == 0, (SecondsPastMidnight % 216UL) == 0);
      octUpdate = false;
      hexUpdate = false;
      decUpdate = false;
      stdUpdate = false;
    }

    // Read the display mode switch
    mode = digitalRead(MODE_PIN);

    if (mode != oldMode) {
      //Serial.print("mode = ");
      //Serial.println(mode);
      
      if (mode == LOW) {
        if (displayFormat == STD_DISPLAY)
          displayFormat = HEX_DISPLAY;
        else
          displayFormat++;
      }
      
      oldMode = mode;
      delay(10);      // Debounce delay
    }
  }
}
