# CUtils - to do

Bugs and problems waiting for a fix, the most serious first. Every session working on
CUtils reads this and takes from the top; when one is fixed, delete its entry (git keeps
the history) and say how it was checked in the commit. New findings go in here as they
are found, with how to make them happen again.

How each entry was found: *verified* (made to happen, more than once where it says so),
*seen* (happened once, not yet made to happen again), *suspect* (looks wrong, not checked
against the right answer).

## Decisions waiting

### 1. tar's times are the Amiga clock taken as UTC: a decision to make

*Read 2026-10-16* (man tar, "Dates"): on purpose, "as Amiga tools have always done".
**The user's call** whether to change it.

- `tar tvzf MyGame.tgz` (made with GNU tar on the PC, files dated 11:34 local time,
  CEST) shows 09:34. GNU tar shows local time. The Amiga's clock was on local time.
- Changing it means an offset from GMT both ways (listing, unpacking, creating). The catch
  (Knowledge amiga/ftp-client.md): locale.library's `loc_GMTOffset` has no summer time and a
  fresh 3.2.1 says UTC+5; a Unix-style `ENV:TZ` (`CET-1CEST,M3.5.0,M10.5.0/3`) has both.
  CTerm's FTP client went to local times this way in 0.3b44.

## Docs

- README.md and docs/guide/ were regrouped on 2026-10-16. The screenshots in
  docs/images/ are of FS-UAE (topaz 8, CTerm's own screen, the Nord theme): retake them
  when a command's output changes. How they were taken is in docs/guide/README.md.
