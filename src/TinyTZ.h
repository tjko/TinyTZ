/*
  TinyTZ.h - Small Arduino library for Timezone handling.
  Copyright (c) 2017 Timo Kokkonen <tjko@iki.fi>. 
  All Rights Reserved.

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

#ifndef TINYTZ_H
#define TINYTZ_H

#include <Arduino.h>

/* Set TINY_PARSER to select POSIX TZ string parser:
     0 = GNU C Library parser
     1 = TinyTZ parser
  
  TinyTZ parser is some 2.5kB smaller (flash usage) and uses some 
  40 bytes less SRAM, but might not be as robust as the GNU libc one.
*/     
#define TINY_PARSER 1


#define TZ_NAME_MAX_LEN 8

/* This structure contains all the information about a
   timezone given in the POSIX standard TZ envariable.  */

typedef enum { J0, J1, M } tz_rule_type;


typedef struct {
  char name[TZ_NAME_MAX_LEN+1];

  /* When to change.  */
  tz_rule_type type;          /* Interpretation of:  */
  uint16_t m, n, d;           /* Month, week, day.  */
  uint32_t secs;              /* Time of day.  */

  int32_t offset;            /* Seconds east of GMT (west if < 0).  */

  /* We cache the computed time of change for a
     given year so we don't have to recompute it.  */
  uint32_t change;       /* When to change to this zone.  */
  int16_t computed_for;    /* Year above is computed for.  */
} tz_rule;

//extern tz_rule tz_rules[2];

class TinyTimezone
{
 public:
  static tz_rule tz_rules[2];

  TinyTimezone();
  
  static void setTZ(const char *tz = NULL);
  static int avr_dst(const uint32_t * timer, int32_t * z);
  const char* timezone(int isdst = 0) {
    return tz_rules[(isdst ? 1 : 0)].name;
  }
  long offset(int isdst = 0) {
    return tz_rules[(isdst ? 1 : 0)].offset;
  }
  static int isdst(uint32_t time);


};

extern TinyTimezone TinyTZ;

#endif
