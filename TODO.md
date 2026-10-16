# CUtils - to do

Bugs and problems waiting for a fix, the most serious first. Every session working on
CUtils reads this and takes from the top; when one is fixed, delete its entry (git keeps
the history) and say how it was checked in the commit. New findings go in here as they
are found, with how to make them happen again.

How each entry was found: *verified* (made to happen, more than once where it says so),
*seen* (happened once, not yet made to happen again), *suspect* (looks wrong, not checked
against the right answer).

## Crashes

### 1. tar crashes the Amiga when it creates an archive on a 4 KB stack

*Verified 2026-10-16*, FS-UAE a1200-32-mui5 as a 68030 (`amiga-test --cpu 030`),
CUtils at 3e34669, csh 0.1b12, CTerm 0.3b50.

- `tar czf RAM:x.tgz SYS:T/demo/MyGame` (a drawer of 15 small files and 2 drawers)
  run from a script with `Stack 4096` first: the machine resets. 3 of 3 times in a
  CTerm tab, 1 of 1 from `Execute` with `Stack 4096` through `wasabi run`.
  `tar cjf` in a CTerm tab: **Software Failure, tar, #80000006**.
- With 16 KB (wasabi's `run` gives 16384) the same commands work.
- On 4 KB these work: `gzip -c`, `bzip2 -c`, `gunzip -c`, `tar cf` (no squeezing),
  `tar tvf`/`tar tvzf`, and `tar cjf` from `Execute` (it failed only in the CTerm tab).
- **4096 is what a Shell gets by default**, and a CTerm tab too (`Stack` there says 4096).
  So anyone typing `tar czf` in an ordinary Shell takes the machine down.
- The guru report wasabid kept once was `DEADEND ALERT #81000005 in task
  'SYS:T/CT/CTerm'`. It most likely came from tar overrunning its stack and damaging
  memory, not from CTerm itself (see CTerm's TODO.md).
- Where to look: tar's create path (walking the tree, writing headers) together with
  cz's packing. Measure with `-fstack-usage` (as for wasabid: at -O2 the static
  functions are inlined, so their locals add up in one frame). The rule in design.md
  is "big things on the heap"; tests/sed-pc/stack.sh is the model for a 4 KB stack test.
  A tar case on 4 KB belongs in tests/cases once it is fixed.

### 2. Closing CTerm while man shows a page crashes the Amiga

*Verified 2026-10-16* (2 of 2), the same machine and builds.

- In a CTerm tab: `man ls` (the viewer is open), then end CTerm with a break
  (`wasabi kill <CTerm's address>`, the same as Ctrl-C to CTerm): the machine resets.
- top and ctop, ended in exactly the same way, quit cleanly (3e34669's TE_END fix).
  man got the same fix in that commit, so something else in man's exit is at fault.
- Not yet tried: closing the tab alone, or CTerm's Quit menu, with man open.
- Could be on CTerm's side too; CTerm's TODO.md points here.

## Wrong results

### 3. `cut -d" "` and `cut -d ' '` say "-f, -c or -b, with a list"

*Verified 2026-10-16* under csh 0.1b12 (in a CTerm tab and through `wasabi run`).

- `cut -d" " -f1 file` and `cut -d ' ' -f1 file` both fail, rc 10, with
  `cut: -f, -c or -b, with a list`. `cut "-d " -f1 file` works.
- So the space either never reaches cut as an argument (csh), or cut takes `-d` with a
  lone space as the next word wrongly. Try under the ROM Shell (NOCSH=1 rig) to tell which;
  if csh is at fault, it goes into CShell's list instead.

### 4. wget's progress line repeats instead of redrawing, in a CTerm window

*Verified 2026-10-16*, once each way.

- CTerm in a Workbench window started with `COLS 80 ROWS 20`: every update of
  `wget http://127.0.0.1:8481/slow.bin` came out on a new line (17 lines for one file).
- On CTerm's own full screen (80 columns) the line redraws in place, as it should.
- Likely a width mismatch: the line fills exactly the width wget thinks the window
  has, and the console wraps before the `\r` comes back. Either use one column less, or
  find out why the width is wrong there ($COLUMNS, or the console's answer).

### 5. tar's listing shows file times in UTC

*Suspect 2026-10-16.*

- `tar tvzf MyGame.tgz` (made with GNU tar on the PC, files dated 11:34 local time,
  CEST) shows 09:34. GNU tar shows local time. The Amiga's clock was on local time.
- Check what the unpacked files get, too, and use the Locale's offset from GMT both ways.

### 6. tar's listing of an archive made on the Amiga: rwx for all, a drawer missing

*Suspect 2026-10-16.*

- `tar czf RAM:MyGame.tgz SYS:T/demo/MyGame` (16 KB stack), then `tar tvf`: every file is
  `-rwxrwxrwx`, also the script with the s bit and the plain text files; `src/` has an
  entry, but `gfx/` and `MyGame/` itself have none.
- Compare with GNU tar's listing of the same archive on the PC before calling it wrong:
  the Amiga's bits may be mapped that way on purpose (pax AMIGA.protect keeps the real ones).

## Docs

- README.md and docs/guide/ were regrouped on 2026-10-16. The screenshots in
  docs/images/ are of FS-UAE (topaz 8, CTerm's own screen, the Nord theme): retake them
  when a command's output changes. How they were taken is in docs/guide/README.md.
