/* tzset.cpp -- timezone functions based on GNU glibc
   Copyright (C) 2017 Timo Kokkonen <tjko@iki.fi>

   This file is based on time/tzset.c from GNU C Library:

   Copyright (C) 1991-2012 Free Software Foundation, Inc.


   This file is part of the TinyTZ Library.

   TinyTZ is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   TinyTZ is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

#include <Arduino.h>
#include "TinyTZ.h"

#define min(a, b)    ((a) < (b) ? (a) : (b))
#define max(a, b)    ((a) > (b) ? (a) : (b))
#define sign(x)      ((x) < 0 ? -1 : 1)

#define SECSPERMIN  60
#define MINSPERHOUR 60
#define HOURSPERDAY 24
#define SECSPERHOUR  (SECSPERMIN * MINSPERHOUR)
#define SECSPERDAY  ((long) SECSPERHOUR * HOURSPERDAY)


/* Nonzero if YEAR is a leap year (every 4 years,
   except every 100th isn't, and every 400th is).  */
# define __isleap(year)\
  ((year) % 4 == 0 && ((year) % 100 != 0 || (year) % 400 == 0))


typedef uint32_t time_t;


/* tz_rules[0] is standard, tz_rules[1] is daylight.  */
//static tz_rule tz_rules[2];
#define tz_rules TinyTimezone::tz_rules

/* How many days come before each month (0-12).  */
const unsigned int __mon_yday[2][13] =
  {
    /* Normal years.  */
    { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 },
    /* Leap years.  */
    { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 }
  };



static uint32_t compute_offset (uint32_t ss, uint32_t mm, uint32_t hh)
{
  return min (ss, 59) + min (mm, 59) * 60 + min (hh, 24) * 60 * 60;
}


/* Parse the POSIX TZ-style string.  */
void __tzset_parse_tz (const char *tz)
{
  unsigned short int hh, mm, ss;

  /* Clear out old state and reset to unnamed UTC.  */
  memset (tz_rules, '\0', sizeof tz_rules);
  //tz_rules[0].name = tz_rules[1].name = "";

  /* Get the standard timezone name.  */
  char *tzbuf = strdup (tz);

  int consumed;
  if (sscanf (tz, "%[A-Za-z]%n", tzbuf, &consumed) != 1)
    {
      /* Check for the quoted version.  */
      char *wp = tzbuf;
      if (__builtin_expect (*tz++ != '<', 0))
	goto out;

      while (isalnum (*tz) || *tz == '+' || *tz == '-')
	*wp++ = *tz++;
      if (__builtin_expect (*tz++ != '>' || wp - tzbuf < 3, 0))
	goto out;
      *wp = '\0';
    }
  else if (__builtin_expect (consumed < 3, 0))
    goto out;
  else
    tz += consumed;

  memcpy(tz_rules[0].name, tzbuf, min(consumed,TZ_NAME_MAX_LEN));
  tz_rules[0].name[min(consumed,TZ_NAME_MAX_LEN)]=0;

  /* Figure out the standard offset from UTC.  */
  if (*tz == '\0' || (*tz != '+' && *tz != '-' && !isdigit (*tz)))
    goto out;

  if (*tz == '-' || *tz == '+')
    tz_rules[0].offset = *tz++ == '-' ? 1L : -1L;
  else
    tz_rules[0].offset = -1L;
  switch (sscanf (tz, "%hu%n:%hu%n:%hu%n",
		  &hh, &consumed, &mm, &consumed, &ss, &consumed))
    {
    default:
      tz_rules[0].offset = 0;
      goto out;
    case 1:
      mm = 0;
    case 2:
      ss = 0;
    case 3:
      break;
    }
  tz_rules[0].offset *= compute_offset (ss, mm, hh);
  tz += consumed;

  /* Get the DST timezone name (if any).  */
  if (*tz != '\0')
    {
      if (sscanf (tz, "%[A-Za-z]%n", tzbuf, &consumed) != 1)
	{
	  /* Check for the quoted version.  */
	  char *wp = tzbuf;
	  const char *rp = tz;
	  if (__builtin_expect (*rp++ != '<', 0))
	    /* Punt on name, set up the offsets.  */
	    goto done_names;

	  while (isalnum (*rp) || *rp == '+' || *rp == '-')
	    *wp++ = *rp++;
	  if (__builtin_expect (*rp++ != '>' || wp - tzbuf < 3, 0))
	    /* Punt on name, set up the offsets.  */
	    goto done_names;
	  *wp = '\0';
	  tz = rp;
	}
      else if (__builtin_expect (consumed < 3, 0))
	/* Punt on name, set up the offsets.  */
	goto done_names;
      else
	tz += consumed;

      //tz_rules[1].name = __tzstring (tzbuf);
      memcpy(tz_rules[1].name, tzbuf, min(consumed,TZ_NAME_MAX_LEN));
      tz_rules[1].name[min(consumed,TZ_NAME_MAX_LEN)]=0;

      /* Figure out the DST offset from GMT.  */
      if (*tz == '-' || *tz == '+')
	tz_rules[1].offset = *tz++ == '-' ? 1L : -1L;
      else
	tz_rules[1].offset = -1L;

      switch (sscanf (tz, "%hu%n:%hu%n:%hu%n",
		      &hh, &consumed, &mm, &consumed, &ss, &consumed))
	{
	default:
	  /* Default to one hour later than standard time.  */
	  tz_rules[1].offset = tz_rules[0].offset + (60 * 60);
	  break;

	case 1:
	  mm = 0;
	case 2:
	  ss = 0;
	case 3:
	  tz_rules[1].offset *= compute_offset (ss, mm, hh);
	  tz += consumed;
	  break;
	}
#if 0
      if (*tz == '\0' || (tz[0] == ',' && tz[1] == '\0'))
	{
	  /* There is no rule.  See if there is a default rule file.  */
	  __tzfile_default (tz_rules[0].name, tz_rules[1].name,
			    tz_rules[0].offset, tz_rules[1].offset);
	  if (__use_tzfile)
	    {
	      free (old_tz);
	      old_tz = NULL;
	      return;
	    }
	}
#endif      
    }
  else
    {
      /* There is no DST.  */
      //tz_rules[1].name = tz_rules[0].name;
      strncpy(tz_rules[1].name, tz_rules[0].name, TZ_NAME_MAX_LEN);
      tz_rules[1].offset = tz_rules[0].offset;
      goto out;
    }

 done_names:
  /* Figure out the standard <-> DST rules.  */
  for (unsigned int whichrule = 0; whichrule < 2; ++whichrule)
    {
      register tz_rule *tzr = &tz_rules[whichrule];

      /* Ignore comma to support string following the incorrect
	 specification in early POSIX.1 printings.  */
      tz += *tz == ',';

      /* Get the date of the change.  */
      if (*tz == 'J' || isdigit (*tz))
	{
	  char *end;
	  tzr->type = *tz == 'J' ? J1 : J0;
	  if (tzr->type == J1 && !isdigit (*++tz))
	    goto out;
	  unsigned long int d = strtoul (tz, &end, 10);
	  if (end == tz || d > 365)
	    goto out;
	  if (tzr->type == J1 && d == 0)
	    goto out;
	  tzr->d = d;
	  tz = end;
	}
      else if (*tz == 'M')
	{
	  tzr->type = M;
	  if (sscanf (tz, "M%hu.%hu.%hu%n",
		      &tzr->m, &tzr->n, &tzr->d, &consumed) != 3
	      || tzr->m < 1 || tzr->m > 12
	      || tzr->n < 1 || tzr->n > 5 || tzr->d > 6)
	    goto out;
	  tz += consumed;
	}
      else if (*tz == '\0')
	{
         /* Daylight time rules in the U.S. are defined in the
            U.S. Code, Title 15, Chapter 6, Subchapter IX - Standard
            Time.  These dates were established by Congress in the
            Energy Policy Act of 2005 [Pub. L. no. 109-58, 119 Stat 594
            (2005)].
	    Below is the equivalent of "M3.2.0,M11.1.0" [/2 not needed
	    since 2:00AM is the default].  */
	  tzr->type = M;
	  if (tzr == &tz_rules[0])
	    {
	      tzr->m = 3;
	      tzr->n = 2;
	      tzr->d = 0;
	    }
	  else
	    {
	      tzr->m = 11;
	      tzr->n = 1;
	      tzr->d = 0;
	    }
	}
      else
	goto out;

      if (*tz != '\0' && *tz != '/' && *tz != ',')
	goto out;
      else if (*tz == '/')
	{
	  /* Get the time of day of the change.  */
	  ++tz;
	  if (*tz == '\0')
	    goto out;
	  consumed = 0;
	  switch (sscanf (tz, "%hu%n:%hu%n:%hu%n",
			  &hh, &consumed, &mm, &consumed, &ss, &consumed))
	    {
	    default:
	      hh = 2;		/* Default to 2:00 AM.  */
	    case 1:
	      mm = 0;
	    case 2:
	      ss = 0;
	    case 3:
	      break;
	    }
	  tz += consumed;
	  tzr->secs = (hh * 60 * 60) + (mm * 60) + ss;
	}
      else
	/* Default to 2:00 AM.  */
	tzr->secs = 2 * 60 * 60;

      tzr->computed_for = -1;
    }

 out:
  free(tzbuf);
  //update_vars ();
}


/* Figure out the exact time (as a time_t) in YEAR
   when the change described by RULE will occur and
   put it in RULE->change, saving YEAR in RULE->computed_for.  */
void __tzset_compute_change (tz_rule *rule, int year)
{
  register time_t t;

  if (year != -1 && rule->computed_for == year)
    /* Operations on times in 2 BC will be slower.  Oh well.  */
    return;

  /* First set T to January 1st, 0:00:00 GMT in YEAR.  */
  if (year > 1970)
    t = ((year - 1970) * 365
	 + /* Compute the number of leapdays between 1970 and YEAR
	      (exclusive).  There is a leapday every 4th year ...  */
	 + ((year - 1) / 4 - 1970 / 4)
	 /* ... except every 100th year ... */
	 - ((year - 1) / 100 - 1970 / 100)
	 /* ... but still every 400th year.  */
	 + ((year - 1) / 400 - 1970 / 400)) * SECSPERDAY;
  else
    t = 0;

  switch (rule->type)
    {
    case J1:
      /* Jn - Julian day, 1 == January 1, 60 == March 1 even in leap years.
	 In non-leap years, or if the day number is 59 or less, just
	 add SECSPERDAY times the day number-1 to the time of
	 January 1, midnight, to get the day.  */
      t += (rule->d - 1) * SECSPERDAY;
      if (rule->d >= 60 && __isleap (year))
	t += SECSPERDAY;
      break;

    case J0:
      /* n - Day of year.
	 Just add SECSPERDAY times the day number to the time of Jan 1st.  */
      t += rule->d * SECSPERDAY;
      break;

    case M:
      /* Mm.n.d - Nth "Dth day" of month M.  */
      {
	unsigned int i;
	int d, m1, yy0, yy1, yy2, dow;
	const unsigned short int *myday =
	  &__mon_yday[__isleap (year)][rule->m];

	/* First add SECSPERDAY for each day in months before M.  */
	t += myday[-1] * SECSPERDAY;

	/* Use Zeller's Congruence to get day-of-week of first day of month. */
	m1 = (rule->m + 9) % 12 + 1;
	yy0 = (rule->m <= 2) ? (year - 1) : year;
	yy1 = yy0 / 100;
	yy2 = yy0 % 100;
	dow = ((26 * m1 - 2) / 10 + 1 + yy2 + yy2 / 4 + yy1 / 4 - 2 * yy1) % 7;
	if (dow < 0)
	  dow += 7;

	/* DOW is the day-of-week of the first day of the month.  Get the
	   day-of-month (zero-origin) of the first DOW day of the month.  */
	d = rule->d - dow;
	if (d < 0)
	  d += 7;
	for (i = 1; i < rule->n; ++i)
	  {
	    if (d + 7 >= (int) myday[0] - myday[-1])
	      break;
	    d += 7;
	  }

	/* D is the day-of-month (zero-origin) of the day we want.  */
	t += d * SECSPERDAY;
      }
      break;
    }

  /* T is now the Epoch-relative time of 0:00:00 GMT on the day we want.
     Just add the time of day and local offset from GMT, and we're done.  */

  rule->change = t - rule->offset + rule->secs;
  rule->computed_for = year;
}


