# TinyTZ
## Small Arduino Timezone library with POSIX TZ String support

**TinyTZ** is a small Arduino library for handling timezones.
It supports specifying timezone using POSIX TZ (IEEE 1003 String) format,
thus making it easy to use DHCP to obtain Timezone and NTP to display
correct local time...

TinyTZ sets calls Arduingo (AVR) *set_zone()* and *set_dst()* functions
to configure timezone (including Daylight Savings Time (DST))
to work with built-in functions from *time.h*.

License: GPL3


## Library Installation

1. Download the ZIP file (rename it to "WireScan.zip" if needed)
2. From the Arduino IDE select: Scketch -> Include Library... -> Add .ZIP Library...
3. Restart the IDE to see new library "TinyTZ" and examples for it


## Usage Examples

Typical usage for this library is to be able to get time from NTP server (or RTC clock)
that is using UTC (GMT) time and convert it to local time.


```
#include <time.h>
#include <TinyTZ.h>

...

void setup(void)
{
  Serial.begin(9600);

  TinyTZ.setTZ("PST8PDT7,M3.2.0/02:00,M11.1.0/02:00");         

  ...
}


void main(void)
{
  struct tm *tm;
  
  // get UTC time from NTP server (or from RTC...)
  time_t unixtime = get_time_from_somewhere()

  // Unix time and Arduing (AVR) time start differs exactly UNIX_OFFSET seconds...
  time_t arduinotime = unixtime - UNIX_OFFSET;

  gmtime_r(&arduinotime, &tm);
  Serial.print("UTC time is ");
  Serial.println(isotime(&tm);

  localtime_r(&arduinotime, &tm);
  Serial.print("Local time is ")
  Serial.println(isotime(&tm));

  ...

  int isdst = TinyTZ(unixtime);

  Serial.print("Timezone name is ");
  Serial.println(TinyTZ.timezone(isdst));
  Serial.print("Timezone offset is ");
  Serial.println(TinyTZ.offset(isdst));

  ...
}


```


## Parser Options

This library includes TZ string parser from GNU C Library (glibc) as well as new small footprint parser that uses some 2.5kB less flash (and some 40 bytes less SRAM)

To select which parser to use edit *TINY_PARSER* definition in *TinyTZ.h*.  By default the new small parser is used/


