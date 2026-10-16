# date - show or set the date and time (replaces C:Date)

## SYNOPSIS
    date [DAY] [DATE] [TIME] [TO|VER file]
    date [-u] [-r file | -d date] [+FORMAT]

## DESCRIPTION
Shows the date and time, or sets the clock. It takes the place of the
Amiga's own C:Date, so everything you type for Date still works, and it
also understands the Unix form with a `+FORMAT` for scripts.

With nothing after it, date prints the date as the Amiga's Date does, in
the words of your Locale settings:

    Wednesday 14-Oct-26 18:36:41

When the first word starts with `-` or `+`, it is the Unix form;
otherwise it is the Amiga form.

## OPTIONS
These belong to the Unix form.
- **+FORMAT**
  print the date in this shape; see FORMAT below. Put the whole thing in
  quotes when it has spaces in it. Without it, the Amiga's form is used.
- **-r** FILE
  show the date of FILE instead of now.
- **-d** DATE
  show this date instead of now. Accepts `2026-10-14`,
  `"2026-10-14 13:45"`, `2026-10-14T13:45:10`, `14-Oct-26`, `13:45`
  (today), Today, Yesterday or a weekday, as touch **-d** does.
- **-u** accepted and ignored: date shows the Amiga's own (local) clock.

## AMIGA FORM
The Amiga template is `DAY,DATE,TIME,TO=VER/K`. Any of DAY, DATE and
TIME, in any order, *sets* the system clock and prints nothing:
- **DATE** `14-Oct-26`, `14-10-26`, `14-Oct-2026` or `2026-10-14`: that
  day, keeping the time of day. Two-digit years 78 to 99 are 19xx, the
  others 20xx.
- **TIME** `18:30` or `18:30:15`: that time, keeping the day.
- **DAY** Today, Tomorrow, Yesterday, or a weekday name, meaning the next
  one (on a Wednesday, Wednesday is a week ahead).
- **TO** file
  (also `VER file`, `TO=file`, `VER=file`): write the date into the file
  instead of the window. After setting the clock, the new date is
  written.

Setting changes the system clock only. To keep the new time after
switching off, save it to the battery clock with `SetClock SAVE`.

## FORMAT
Inside a `+FORMAT` these are replaced; anything else is printed as it is.
Names are in English.

    %Y  year, 2026          %y  year, 26          %C  century, 20
    %m  month, 01-12        %d  day, 01-31        %e  day, space-padded
    %b  month, Oct (also %h)                     %B  month, October
    %a  weekday, Wed        %A  weekday, Wednesday
    %H  hour, 00-23         %k  hour, space-padded
    %I  hour, 01-12         %l  hour 1-12, space-padded   %p  AM or PM
    %M  minute              %S  second            %j  day of year
    %u  weekday 1-7 (Monday 1)    %w  weekday 0-6 (Sunday 0)
    %F  2026-10-14          %T  18:36:41          %R  18:36
    %D  10/14/26            %s  seconds since 1970
    %Z  nothing (no zones)  %n  new line   %t  tab   %%  a %

## EXAMPLES
    date
    date 14-Oct-26 18:30
    date 12:00
    date TO RAM:now.txt
    date +%F
    date "+%Y-%m-%d %H:%M" -r S:Startup-Sequence

## RETURN CODES
0 fine; 10 a bad date or argument, a clock that could not be set, or a
file that could not be read or written. (C:Date returns 20 for a bad
date; this date returns 10.)

## NOTES
`%s` is seconds since 1970 UTC, the Amiga's local clock turned into UTC.
The Amiga's clock is local time, so the zone comes from **ENV:TZ**
when it is set, written as Unix does (`SetEnv SAVE TZ CET-1CEST,M3.5.0,M10.5.0/3`:
Central Europe with its summer time; `UTC0` for none), else from the
Locale preferences' time zone (which knows no summer time).

`date ?` shows the usage line.

## SEE ALSO
`touch`, `stat`, `ls`, Date, SetClock
