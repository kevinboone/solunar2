# Solunar 2.0

Version 2.0b

`solunar` is a command-line utility for Linux and similar platforms, 
that displays sunrise and sunset times and related data at a specified
location on a specified day.  You can specify the location as a city or as
geographical coordinates.  The timezone can also be specified in terms 
of a city, or as an offset from UTC. By default, `solunar` uses the local
time of the specified city. `solunar` can display a summary of
solar and lunar events for a selected year. 

## Differences between version 2.0 and previous releases 

Internally, version 2.0 is completely different from earlier releases --
it is a complete reimplementation. In this version I have separated
the astronomical computations completely from the input and output,
into an independent library.
Not only should this make it possible to use the astronomical library
in other programs, `solunar` should be easier to maintain. The
main problem with the previous version of `solunar` was that it had 
grown up incrementally over a period of twenty years, and had become
impossible to modify without breaking existing functionality. 
Although this new version does not yet have all the same functionality
of the old one, it should be much easier to extend. 

Despite the internal reorganization, there should be relatively
few differences from a user perspective.

* The display timezone can be set explicitly using the `--tz` option.
  You can set it to a city or to a UTC offset (see notes below on how
  the offset is interpreted). 

* `--tz` must now be specified explicitly 
  when the location is given in
  lat/long coordinates, rather than a city.
  The program no longer defaults to using the host
  system's timezone. The special timezone "sys" can be used to indicate
  the host system timezone, for compatibility with previous versions. However,
  the program will display a warning when this is done.

* Default values can now be set in a configuration file, either at the
  user or the system level. This is particularly useful for setting
  a home city.

* The display format has changed. I'm hoping that it is both clearer to
  read, and also easier for other programs to parse.

* The program displays the dates of changes to daylight savings time, to
  the extent that the host system's timezone database has the correct information.

* There is a JSON output mode, for use in scripts. JSON is easy to parse
  in most scripting languages. I have not settled on the exact format of
  the JSON output yet, and I'm open to suggestions.

* All internal calculation now uses Unix time\_t objects, rather than the
  mixture of Unix times, julian days, and custom date representations the
  old version had. This isn't a change that users will notice, but I hope
  it will make timezone handling more consistent.

Although it gives me no pleasure to say this, I should point out that,
as a completely new implementation, I will probably have to fix many
of the bugs again, that I had to fix in the previous version. Most of
these bugs concerned timezone handling, particular in the southern
hemisphere.

## Building `solunar`

`solunar` (version 1.0) is already in the repositories of several Linux
distributions, but this new version will probably not be, so you'll have
to build it from source. This should be trivially easy on Linux, so
long as you have `gcc` and `make` available:

    $ make
    $ sudo make install

`solunar` will build on Linux, and is reported to build on OS/X. It
will build for Android (for use in a terminal) with the Google 
native development tools. For Android you'll need to add position-independent
code flags to both the build and link:

    $ EXTRA_LDFLAGS=-pie EXTRA_CFLAGS=-fPIC -fPIE make 

It builds on Windows under Cygwin, and I would expect it to build with the
Windows Linux subsystem (WSL) but I haven't tried it. It won't run on Windows
under MinGW, even if it builds, because there is no timezone database. Problems
may also be encountered on very minimal Linux systems like Alpine, for the same
reason. However, you can install the timezone database on Alpine using `apk add
tzdata`.

`solunar` stores timezone information for cities in `libsolunar/src/cityinfo.h`.
Timezone information changes occasionally. If you're building on a system with 
an up-to-date `/usr/share/zoneinfo/zone.tab` you can optionally run
`parse_zoneinfo.pl` to generate a new `cityinfo.h`. All the recent changes of
which I'm aware have been in naming -- Kyev to Kiev, etc.

## Command-line options

*-a,--ampm*

Show 12-hour AM/PM times rather than 24-hour clock times.

*-c,--city={name}*

Specify a full or partial city name. Full names are of the form
"Europe/London", but it is usually easier to give a partial name:
`--city=paris`.  Matching is case-insensitive, and the program will warn if
there is more than one match. Use `--city-list` to get a (long) list of known
cities.

Specifying a city sets the geographical location (latitude and
longitude) and also the timezone for local time display. The
latitude and longitude can be overridden whilst still keeping the
city's name and timezone. Alternative, the timezone can be
overridden, while keeping the same location. 

*-d,--date={date}*

The date can be specified in any of the following formats:

* Jan 21
* 21 Jan
* Jan 21 2020
* Jan 21 20
* 21 Jan 2020
* 21 Jan 20
* 2020-01-21 

Note that `solunar` will not accept dates in formats where the separator is a
forward-slash character, because there is too much opportunity for confusion
between the US and European varieties. 

If no year is specified in the date, the current year is assumed.  If no date
is specified at all, the results will be for the current day
(relative to the specified timezone).

*-f,--full*

Display full, rather than summary, results. Not all functions display
more data in 'full' mode.

*-j,--json*

Outputs all data in JSON format, for parsing by other programs.

*--list-cities*

Print to standard out the full list of cities.

*--log-level={0..4}*

For debugging purposes, set the logging level. The default level is
1. Levels greater then 3 will probably only make sense alongside the 
source code.

*-l,--latitude={-90..90}*

Specifies the latitude in degrees. Positive latitudes are north
of the equator, negative south. The latitude can be a decimal number.

*-o,--longitude={-180..180}*

Specifies the longitude in degrees. Positive longitudes are east of 
of the meridian, negative south. The longitude can be a decimal number.

Note that latitude and longitude can be specified along with a city
name, to use the city's timezone information. Without a city name, 
the latitude and longitude must be used with the `--tz` option
to set a timezone.

*-t,--tz={timezone}*

Sets the timezone in which results will be displayed. This option also
affects the interpretation of input dates, in a rather subtle way. 
In general `solunar` displays information for a 24-hour period
beginning and ending at midnight. When a date is entered on the
command-line, the 24-hour period is assumed to be on that date
in the specified timezone. Otherwise, the 24-hour period is
relative to the timezone of the specified city.

In general, any name in the city list can also be used as a timezone,
except that the `--tz` switch will not accept abbreviations. 
On most Linux systems, the timezone can also be one of the conventional
strings: GMT, UTC, EST, MST, WET, etc. These can be used with 
offsets, of which the most useful are probably `UTC+N` and `UTC-N`. 
N is a number of hours offset from UTC (GMT), which can be fractional.

Be aware that the UTC offset is for the selected location, not for the
user's system -- the meanings of the + and - signs might not be 
intuitive, and should be tested.

A special form of the `tz` option is the string `sys`. This overrides
the timezone in a selected city with the system's timezone. That is,
all times are displayed correct for the user's system, regardless of
the selected location. In some circumstances, the program will
display a warning when this option is used, as it is likely to
be inappropriate.

Note that `solunar` does not, indeed _can_ not, warn about an
incorrect timezone name -- the C function used to manipulate the
timezone does not report any errors.

*-y,--year={year}*

Print a year summary of events with astronomical significance, such
as equinoxes, and festival days derived from a lunar calendar, such
as Easter.

This option can optionally be used with a timezone, city, latitude,
and longitude, in any combination.
The effects are subtle and, if nothing else is specified, the location
is London, with the system timezone, for the current year.

The location and timezone affect the display of events that have
notional exact times, rather than just particular days, like the solstices. 
The latitude, however specified, affects the ordering of the solstices
-- the winter solstice is in June in the southern hemisphere. 
There is no general agreement about whether the term "vernal"
(spring) equinox should be used in the southern hemisphere, where
it usually occurs in autumn. `solunar` assumes the vernal equinox is
in March, whatever the location.

`solunar` attempts to work out the start and end of daylight
savings time -- not from astronomical calculations (because there
aren't any) but from the system's timezone database. These calculations
are not particularly exact, and are subject to the vagaries of
politics.

## RC (configuration) files 

`solunar` reads the configuration files `/etc/solunar.rc` and
`$HOME/.solunar.rc`. Any of the long-form command line options
can be specified, in the form `option=value`. The command-line
arguments override these settings. So, for example, to set the 
default city to London, add the line `city=London` to the RC file.


## Notes

The calculations are all based
on published algorithms that are in the public domain, and I have checked
them against reliable data sources so far as possible. There are
potential sources of error that the program can't control, and some
issues of interpretation.

The most troublesome potential source of error is the system's timezone
database. By default, results are worked out in UTC and then converted to
the local time of the specified city. This relies on the system's own
time and date being correct, and having a proper understanding of daylight
savings changes. In general, these issues seems to be handled reasonably well
in modern Linux distributions, but there's not much `solunar` can
do if the platform data is incorrect. I've recently discovered that some
Linux distributions require timezone data to be installed separately.

Issues of interpretation include uncertainty about exactly
what positions of the sun in the sky constitute sunset and
sunrise (and similar
considerations for the moon.)
The sun is not a uniform disc, so there has to be a convention for the
angle of zenith that we take as sunset. Most publications that give sunset
times take the Zenith angle as 90 degrees and 50 minutes, so
`solunar` does the same. However, the `--full` switch
will display sunset according to other popular zeniths.
In particular, civil twilight usually has 
a zenith 6 degrees below conventional
sunset, and denotes the time during which outdoor activities are reasonably
practicable. Nautical and astronomical twilight have zeniths 12 and 18 degrees
below conventional sunset respectively. In practice, many 
parts of the world will experience
no astronomical sunset for at least part of the year. Some, of course,
experience no sunset at all for part of the year.

Although there will always be at most one sunset on a given day, and
one sunrise, there can be zero, one, or two moonrises and sets. So to
capture all these events we have to consider the position of the moon
at a series of time intervals, and then determine the horizon-crossing
points, interpolating if necessary (at least, I have not been able to
find a better way to do this). This means that there is even more scope
for disagreement in lunar event times than solar events. Published
sources seem to vary by +/- ten minutes or so.

For the purposes of display and computation, the moon's perigee is taken
to be a distance less than 370000 km from the earth. By that definition, 
there are 5-6 perigee days in a lunar month. When the moon is full at perigee, this
configuration is commonly known as a "supermoon". `solunar` displays
the dates of supermoons, but be aware that there is no clear definition of 
what 'full' amounts to in this context.


## Revision history

_Version 0.1.0, May 2012_<br/>
* First release, based on the original Java implementation of 2005-2011.

_Version 0.1.1, June 2013_<br/>
* Fixed incorrect text label on moonset.
* Fixed a problem with TZ environment handling, that led to a hugely incorrect
  timezone compensation on some platforms.
* Added 12-hour time display.
* Added solunar scoring feature.
* Fixed some minor memory leaks.
* Corrected some errors in the documentation.

_Version 0.1.2, June 2014_<br/>
* Fixed a problem with timezone handling that caused the wrong day to be
  used between midnight and 1AM during daylight savings time.

_Version 0.1.2a, March 2015_<br/>
* Fixed bug in name parsing in parse\_zoneinfo.pl (thanks to Cezary Kruk)

_Version 0.1.3, January 2016_<br/>
* Added `--syslocal` switch; various bug fixes.

_Version 0.1.3a, January 2016_<br/>
* Fixed a bug in working out when a day started and ended, that affected
  mostly pacific timezones. 
* Tidied up the display of the "Today" 
  date. 
* Made system local the default timezone for situations where
  a location (lat/long) is given, but no city -- this seems to be
  what users prefer.

_Version 0.1.3b, May 2017_<br/>
* Tidied up Makefile, so that it respected CFLAGS and LDFLAGS environment
  variables. This is to make it easier to cross-compile.

_Version 0.1.3c, December 2018_<br/>
* Fixed summer/winter naming to depend on hemisphere of selected location.

_Version 2.0a, July 2020_<br/>
* Complete reimplementation. Astronomical calculations are now separated
  completely from input and output, into an independent library.

_Version 2.0b, November 2022_<br/>
* Removed some unnecessary code; added .gitignore; tidied up comments

_Version 2.0c, June 2024_<br/>
* Updated city information; small changes to Makefile; small documentation
  changes; added LICENCE file

## Bugs

Please report bugs through GitHub. Please provide enough information for
me to reproduce the problem including, in particular, your home timezone.

## Author

`solunar` is maintained by Kevin Boone, and released for general use under
the terms of the GNU Public Licence, v3.0. 



