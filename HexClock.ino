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
#define USEC_PER_HEXOND (1318359) // 1.318359 seconds

uint32_t SecondsPastMidnight = 0UL;
uint16_t HexTime = 0UL;
uint16_t DecTime = 0UL;


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
  
  snprintf(buf, sizeof (buf), "%04x %c %02d:%02d:%02d %c %04d\n", HexTime, h, hour, minute, second, d, DecTime);
  Serial.print(buf);
  Serial.flush();
}


void setup(void)
{
  int hour, minute, second;

  Serial.begin(9600);
  
  hour = 12;
  minute = 0;
  second = 0;
  
  SecondsPastMidnight = asSeconds(hour, minute, second);
  HexTime = (SecondsPastMidnight * 65536UL) / SECONDS_PER_DAY;
  DecTime = (SecondsPastMidnight * 10000UL) / SECONDS_PER_DAY;
}


void loop(void)
{
  uint32_t uPeriod;
  uint32_t mPeriod;
  uint32_t dPeriod;

  uPeriod = micros() + USEC_PER_HEXOND;
  mPeriod = millis() + 1000UL;
  dPeriod = millis() + 8640UL;
  
  while (1) {
    if (micros() > uPeriod) {
      uPeriod = micros() + USEC_PER_HEXOND;
      HexTime++;
      ShowTime(true, false);
      //Serial.println("HEXOND");
    }

    if (millis() > mPeriod) {
      mPeriod = millis() + 1000UL;
      SecondsPastMidnight++;
      ShowTime(false, false);
      //Serial.println("SECOND");
    }

    if (millis() > dPeriod) {
      dPeriod = millis() + 8640UL;
      DecTime++;
      ShowTime(false, true);
      //Serial.println("100 uDays");
    }
  }
}
