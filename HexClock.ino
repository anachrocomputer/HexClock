/* HexClock --- display time in hex and percentage          2024-06-14 */

//#define SERIAL_OUTPUT

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

// Direct port I/O for HMDL2416 display
#include <avr/io.h>

#define WR_PIN 7
#define A0_PIN 8
#define A1_PIN 9
#define CS_PIN 10

// I2C setup for DS3231 real-time clock chip
#include <Wire.h>

#define DS3231_ADDR    (0x68)

char *Dayname[8] = {
  "", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"
};

uint32_t SecondsPastMidnight = 0UL;
uint16_t OctTime = 0U;
uint16_t HexTime = 0U;
uint16_t DecTime = 0U;


uint32_t asSeconds(const int hours, const int minutes, const int seconds)
{
  return ((uint32_t)hours * (60UL * 60UL)) + ((uint32_t)minutes * 60UL) + (uint32_t)seconds;
}


void ShowTime(const bool hexConjunction, const bool decConjunction)
{
  int hour, minute, second;
  char buf[64];

  const char h = hexConjunction? '*': ' ';
  const char d = decConjunction? '*': ' ';
  
  hour = SecondsPastMidnight / (60UL * 60UL);
  minute = (SecondsPastMidnight - (hour * 60UL * 60UL)) / 60UL;
  second = SecondsPastMidnight % 60;

#ifdef SERIAL_OUTPUT
  snprintf(buf, sizeof (buf), "%04x %c %04o %c %02d:%02d:%02d %c %04d\n", HexTime, ' ', OctTime, h, hour, minute, second, d, DecTime);
  Serial.print(buf);
  Serial.flush();
#else
  snprintf(buf, sizeof (buf), "%04X", HexTime);
  int i;
  for (i = 0; i < 4; i++) {
    HMDL2416Write(3 - i, buf[i]);
  }
#endif
}


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

  Wire.begin();
  //DS3231Set (2025, 6, 23, 22, 24, 00, 1);

  for (reg = 0; reg < 0x13; reg++) {  
    Wire.beginTransmission(DS3231_ADDR);
    Wire.write((byte)reg);
    Wire.endTransmission();
  
    Wire.requestFrom(DS3231_ADDR, 1);
    val = Wire.read();
    Wire.endTransmission();
    
    ds3231[reg] = val;
    
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
  // HexTime = (SecondsPastMidnight * 65536UL) / SECONDS_PER_DAY;
  // but this causes a 32-bit integer overflow. So we divide both
  // constants by 128 to get 512 and 675
  OctTime = (SecondsPastMidnight * 4096UL) / SECONDS_PER_DAY;
  HexTime = (SecondsPastMidnight * 512UL) / 675UL;
  DecTime = (SecondsPastMidnight * 10000UL) / SECONDS_PER_DAY;
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

  now = millis();
  
  oPeriod = micros() + USEC_PER_OCTOND;
  uPeriod = micros() + USEC_PER_HEXOND;
  mPeriod = now + 1000UL;
  dPeriod = now + 8640UL;
  
  while (1) {
    now = millis();
    
    if (micros() > oPeriod) {
      oPeriod = micros() + USEC_PER_OCTOND;
      if (OctTime < 4095u)
        OctTime++;
      else
        OctTime = 0u;
      octUpdate = true;
      //Serial.println("OCTOND");
    }
    
    if (micros() > uPeriod) {
      uPeriod = micros() + USEC_PER_HEXOND;
      if (HexTime < 65535u)
        HexTime++;
      else
        HexTime = 0u;
      hexUpdate = true;
      //Serial.println("HEXOND");
    }

    if (now > mPeriod) {
      mPeriod = now + 1000UL;
      if (SecondsPastMidnight < (SECONDS_PER_DAY - 1))
        SecondsPastMidnight++;
      else
        SecondsPastMidnight = 0UL;
      stdUpdate = true;
      //Serial.println("SECOND");
    }

    if (now > dPeriod) {
      dPeriod = now + 8640UL;
      if (DecTime < 9999u)
        DecTime++;
      else
        DecTime = 0u;
      decUpdate = true;
      //Serial.println("100 uDays");
    }

    // Conjunction logic does not work because updates do not occur
    // on exactly the same millisecond
    if (octUpdate || hexUpdate || decUpdate || stdUpdate) {
      ShowTime(hexUpdate && stdUpdate, decUpdate && stdUpdate);
      octUpdate = false;
      hexUpdate = false;
      decUpdate = false;
      stdUpdate = false;
    }
  }
}
