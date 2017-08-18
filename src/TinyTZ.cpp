/*
  TinyTZ.cpp - Small Arduino library for Timezone handling.
  Copyright (c) 2017 Timo Kokkonen <tjko@iki.fi>. 

  TintyTZ contains code and ideas from GNU libc library:
  Copyright (C) 1991-2012 Free Software Foundation, Inc.

  This file is part of TinyTZ Library.

  TinyTZ is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  TinyTZ is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <Arduino.h>
#include <time.h>
#include "TinyTZ.h"


void __tzset_compute_change (tz_rule *rule, int year);

#if TINY_PARSER
int __parse_TZ_string(const char *str);
# define tinytz_parse_tz(x)  __parse_TZ_string(x) 
#else
void __tzset_parse_tz (const char *tz);
# define tinytz_parse_tz(x)  __tzset_parse_tz(x)
#endif


TinyTimezone TinyTZ;

tz_rule TinyTimezone::tz_rules[2];


TinyTimezone::TinyTimezone() {
  char tmp[4] = { 'U','T','C',0 };;
  tinytz_parse_tz(tmp);
  set_dst(avr_dst);
  set_zone(0);
}

void TinyTimezone::setTZ(const char *tz) {
  tinytz_parse_tz(tz);
  set_zone(tz_rules[0].offset);
#if 0
  Serial.print(F("TinyTZ.setTZ: "));
  Serial.println(tz);
    
  for (int i = 0; i < 2; i++) {
    Serial.print(F("TZ: "));
    Serial.print(i);
    Serial.print(':');
    Serial.print(tz_rules[i].name);
    Serial.print(',');
    Serial.print(tz_rules[i].offset);
    Serial.print(',');
    Serial.print(tz_rules[i].type);
    Serial.print(',');
    Serial.print(tz_rules[i].m);
    Serial.print(',');
    Serial.print(tz_rules[i].n);
    Serial.print(',');
    Serial.print(tz_rules[i].d);
    Serial.print(',');
    Serial.print(tz_rules[i].secs);
    Serial.println();
  }
#endif
}

int TinyTimezone::isdst(uint32_t timer) {
  struct tm t;
  time_t atime = timer - UNIX_OFFSET;;
  int isdst;

  if (tz_rules[0].offset == tz_rules[1].offset)
    return 0;

  gmtime_r(&atime, &t);
  __tzset_compute_change(&tz_rules[0], 1900 + t.tm_year);
  __tzset_compute_change(&tz_rules[1], 1900 + t.tm_year);

    /* We have to distinguish between northern and southern
      hemisphere.  For the latter the daylight saving time
      ends in the next year.  */
  if (__builtin_expect (tz_rules[0].change > tz_rules[1].change, 0))
    isdst = (timer < tz_rules[1].change || timer >= tz_rules[0].change);
  else
    isdst = (timer >= tz_rules[0].change && timer < tz_rules[1].change);

  return isdst;
}


int TinyTimezone::avr_dst(const uint32_t * timer, int32_t * z) {
  int dst = isdst(*timer + *z + UNIX_OFFSET);

  return (dst ? (tz_rules[1].offset - tz_rules[0].offset) : 0);
}


/* eof :-) */ 
