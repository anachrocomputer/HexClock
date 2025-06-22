/* HexClock --- display time in hex and percentage          2024-06-14 */

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
  
  snprintf(buf, sizeof (buf), "%04x %04o %c %02d:%02d:%02d %c %04d\n", HexTime, OctTime, h, hour, minute, second, d, DecTime);
  Serial.print(buf);
  Serial.flush();
}


void setup(void)
{
  int hour, minute, second;

  Serial.begin(9600);

  // 18:12 is the latest time that can be converted to hex without
  // overflowing the 32-bit intermediate result. To make this setup
  // code work correctly for any time of day, we need 64-bit arithmetic
  hour = 18;
  minute = 12;
  second = 0;
  
  SecondsPastMidnight = asSeconds(hour, minute, second);
  OctTime = (SecondsPastMidnight * 4096UL) / SECONDS_PER_DAY;
  HexTime = (SecondsPastMidnight * 65536UL) / SECONDS_PER_DAY;
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
      OctTime++;
      octUpdate = true;
      //Serial.println("OCTOND");
    }
    
    if (micros() > uPeriod) {
      uPeriod = micros() + USEC_PER_HEXOND;
      HexTime++;
      hexUpdate = true;
      //Serial.println("HEXOND");
    }

    if (now > mPeriod) {
      mPeriod = now + 1000UL;
      SecondsPastMidnight++;
      stdUpdate = true;
      //Serial.println("SECOND");
    }

    if (now > dPeriod) {
      dPeriod = now + 8640UL;
      DecTime++;
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
