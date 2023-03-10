/*
  tiny_parser.cpp - small footprint TZ string parser
  Copyright (c) 2017 Timo Kokkonen <tjko@iki.fi>. 

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

#define min(a, b)    ((a) < (b) ? (a) : (b))
#define max(a, b)    ((a) > (b) ? (a) : (b))

#define tz_rules TinyTimezone::tz_rules


long __parse_TZ_offset(char *str, int *hours, int *mins, int *secs) {
  const char separator[] = ":";
  char *s = str;
  char *saveptr;
  long o;

  *hours = 0;
  *mins = 0;
  *secs = 0;
  
  s = strtok_r(str, separator, &saveptr);
  if (s) {
    *hours = atoi(s);

    s = strtok_r(NULL, separator, &saveptr);
    if (s) {
      *mins = atoi(s);

      s = strtok_r(NULL, separator, &saveptr);
      if (s) {
	*secs = atoi(s);
      }
    }
  }

  o = labs(*hours)*3600 + labs(*mins)*60 + labs(*secs);
  return ((*hours < 0 ? 1 : -1) * o);
}


int __parse_TZ_string(const char *str) {
    const char separator1[] = ",";
    const char separator2[] = "/";
    const char separator3[] = ".";
    
    byte state = 0;
    char offset[10];
    char buf[65];    // parse only first 64 bytes of TZ string... 
    char *s, *e, *saveptr, *saveptr2, *saveptr3;
    byte l = min(sizeof(buf)-1, strlen(str));
    int n,hours,mins,secs;
    long o;


    // reset tz structure to unnamed "UTC"...
    memset(tz_rules, 0, sizeof(tz_rules));

    memcpy(buf, str, l);
    buf[l]=0;

    // basic sanity check on the TZ string...
    if (l < 3 || buf[0] == ':') return -1;


    // get the first part of the string...
    if (!(s = strtok_r(buf, separator1, &saveptr))) 
      return -2;
    l=strlen(s);

    // extract timezone name
    e = s;  
    while ((e < s+l) && (*e != '+' && *e != '-' && ! (*e >= '0' && *e <= '9'))) e++;
    if (e == s || e-s < 3) return -3;
    n = min(e-s, TZ_NAME_MAX_LEN);
    memcpy(tz_rules[0].name, s, n);
    tz_rules[0].name[n]=0;

    // extract offset: [+|-]hh[:mm[:ss]]
    l -= e-s;
    s = e;
    while ((e < s+l) && (*e == '+' || *e == '-' || *e == ':' || (*e >= '0' && *e <= '9'))) e++;
    if (e == s) return -4; // no offset
    if (e-s > 9) return -5; // offset is too long
    memcpy(offset, s, e-s);
    offset[e-s]=0;
    tz_rules[0].offset = __parse_TZ_offset(offset, &hours, &mins, &secs);
    
    // extrat DST timezone name
    l -= e-s;
    s=e;
    while ((e < s+l) && (*e != '+' && *e != '-' && ! (*e >= '0' && *e <= '9'))) e++;
    if (e == s || e-s < 3) return -6;
    n = min(e - s, TZ_NAME_MAX_LEN);
    memcpy(tz_rules[1].name, s, n);
    tz_rules[1].name[n]=0;

    // extract DST offset: [+|-]hh[:mm[:ss]]
    l -= e-s;
    s = e;
    while ((e < s+l) && (*e == '+' || *e == '-' || *e == ':' || (*e >= '0' && *e <= '9'))) e++;
    if (e-s > 9) return -7; // offset is too long
    if (e == s) {
      // if no offset for DST assume +1 hours
      tz_rules[1].offset = tz_rules[0].offset + 3600;
    } else {
      memcpy(offset, s, e-s);
      offset[e-s]=0;
      tz_rules[1].offset = __parse_TZ_offset(offset, &hours, &mins, &secs);
    }


    // parse DST start & end definitions...

    for (int i = 0; i < 2; i++) {
      s = strtok_r(NULL, separator1, &saveptr);
      if (!s) return -8;

      s = strtok_r(s, separator2, &saveptr2);
      if (!s) return -9;
      if (*s == 'M') {
	tz_rules[i].type = M; 
        s = strtok_r(s+1, separator3, &saveptr3);
        if (!s) return -10;
        tz_rules[i].m = atoi(s);
        s = strtok_r(NULL, separator3, &saveptr3);
        if (!s) return -11;
        tz_rules[i].n = atoi(s);
        s = strtok_r(NULL, separator3, &saveptr3);
        if (!s) return -12;
        tz_rules[i].d = atoi(s);
      } else if (*s == 'J') {
        tz_rules[i].type = J1;
        tz_rules[i].d = atoi(s+1);
      } else {
        tz_rules[i].type = J0;
        tz_rules[i].d = atoi(s);
      }

      s = strtok_r(NULL, separator2, &saveptr2);
      if (s)
	      tz_rules[i].secs = labs( __parse_TZ_offset(s, &hours, &mins, &secs));
      else
	      tz_rules[i].secs = 7200;
    }

    
    return 0;
}


/* eof :-) */ 
