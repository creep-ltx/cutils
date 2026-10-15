# cterm - terminal: the Amiga Shell, telnet, ssh, serial, SFTP and FTP

## SYNOPSIS
    CTerm [SHOW] [COLS N] [ROWS N] [FROM SCRIPT] [SHELL COMMAND]
          [PC] [LATIN1]
    CTerm HOST NAME[:PORT] [PC] [LATIN1] [COLS N] [ROWS N]
    CTerm SSH USER@HOST[:PORT] [KEY FILE]
    CTerm SFTP USER@HOST[:PORT][/DRAWER]
    CTerm FTP [USER[:PASSWORD]@]HOST[:PORT][/DRAWER]
    CTerm BOOK NAME
    CTerm SERIAL [DEVICE NAME] [UNIT N] [BAUD N]
    CTerm LISTEN [PORT N]

## DESCRIPTION
CTerm is a terminal for AmigaOS 3.2 and up. It shows the Amiga Shell in
a window of its own, and it is also a terminal for other computers:
telnet, ssh, a serial line or a modem, BBSes, and programs on a PC. It
has a file browser for SFTP and FTP that works inside the tab.

CTerm reads VT100 and xterm codes, as terminals on Linux do. It shows
Amiga fonts and TrueType fonts, from 2 to 16 million colours, and the
characters of Latin-1, UTF-8 and the PC (CP437).

CTerm needs AmigaOS 3.2 and extra memory (fast RAM). On an Amiga with
only chip memory it says "CTerm needs an Amiga with extra memory (fast
RAM)" and does not start. ssh and SFTP also need AmiSSL 5.

Several terminals fit in one window: as **tabs** (one shown at a time),
as **panes** (the window split, several in sight), on nine
**workspaces**, and one floating **scratchpad**. The panes work the way
the i3 and Sway window managers do on Linux, with the Left Amiga key
where i3 has its Mod key.

CTerm keeps its features in files of their own, beside the program:

    CTermSSH      ssh and SFTP
    Protocols/    Zmodem, XYmodem, Hydra; Serial (lines, modems)
    Parts/        Settings, PhoneBook, ARexx, Call, Transfers,
                  SpeedTest, Files (the SFTP/FTP browser), Pictures
    Fonts/        the PC's VGA font (for PC BBSes)
    Rexx/         example ARexx scripts

A part is loaded only when it is used, and leaves memory when it is
done. A feature you never use costs no memory. Without a part, its
window or feature says it is missing. Keep these files with CTerm.

## OPTIONS
**CTerm** with no options opens the Amiga Shell in a CTerm window. If a
CTerm is already running but hidden, it comes back instead, and no
second one opens.

- **HOST** NAME[:PORT]
  call a telnet host. The port is 23 when none is given.
- **SSH** USER@HOST[:PORT]
  call an ssh host (port 22). CTerm starts CTermSSH for it.
- **KEY** FILE
  with SSH: log in with this private key file.
- **SFTP** USER@HOST[:PORT][/DRAWER]
  open the file browser on an ssh host.
- **FTP** [USER[:PASSWORD]@]HOST[:PORT][/DRAWER]
  open the file browser on an FTP host (port 21). With no user, CTerm
  logs in as anonymous.
- **BOOK** NAME
  call a place in the phone book. Use quotes when the name has spaces.
- **SERIAL** a serial terminal on the line set in Settings (Modem tab).
- **DEVICE** NAME
  with SERIAL: another serial device (giving DEVICE alone also means
  SERIAL).
- **UNIT** N
  with SERIAL: another unit of the device.
- **BAUD** N
  with SERIAL: another speed.
- **PC** a PC BBS: the PC's character set (CP437), the PC's colours and
  its own font (IBM VGA 8x16, from `Fonts/` beside CTerm).
- **LATIN1** Latin-1 instead of UTF-8 (for Amiga BBSes).
- **COLS** N
  the window's width in letters.
- **ROWS** N
  the window's height in lines.
- **FROM** SCRIPT
  the Shell runs this script first.
- **SHELL** COMMAND
  every new Shell tab of this run starts COMMAND after
  S:Shell-Startup, for example csh. It is not saved. To keep it, use
  the Settings' Files tab ("A new Shell tab ... Then runs").
- **LISTEN** wait for a program on a PC to connect (the relay,
  pc/cterm-relay.py in CTerm's sources).
- **PORT** N
  with LISTEN: the network port to wait on (7478 when none is given).
- **SHOW** bring a CTerm that is already running to the front, hidden or
  not, instead of opening another.

A start with options that ask for something new (HOST, BOOK, SERIAL,
FROM, COLS and so on) opens a CTerm of its own, even when one is hidden.
To call from a running CTerm, use the Call menu or ARexx's CALL.

LISTEN lets in only the PC named in `ENVARC:CTerm/relay_from`. The
first time, CTerm asks whether to trust the PC that calls, and
remembers it. Delete that file to choose another PC.

## MENUS
"RA" is the Right Amiga key. Every menu shortcut can be changed (see
KEYS), and items without one can be given one.

**CTerm**
- **New Shell** a new Shell, in a pane or a tab (as the Panes menu
  says). Left Amiga+Return.
- **Close Shell** closes it again. Left Amiga+Q.
- **New Shell tab** RA+T. **Close tab** RA+W: ends the shown tab's
  Shell, or hangs up its call.
- **Run ARexx script...** chooses a script in REXX: and runs it.
- **Speed test** measures how fast CTerm runs on this Amiga.
- **About** the version, and the name of the ARexx port.
- **Hide window** puts CTerm away (see HIDING THE WINDOW).
- **Quit** RA+Q.

**Call**
- **Call...** RA+K: asks for a host, and lists the phone book's places
  and the last eight calls. Write `ssh://`, `sftp://` or `ftp://` in
  front for those (`ftp://user:password@host/drawer`). A call opens in
  a new tab, or in a window of its own if that box is unticked. When a
  call ends, Return calls again.
- **Phone book...** RA+B (see PHONE BOOK).
- **Serial terminal** opens the serial line in a tab.
- **Answer call (ATA)** the modem picks up.
- **Send user name** RA+U, **Send password** RA+P: type the phone book
  place's user name or password, with Return, at the host's prompt.

**Edit**
- **Copy** RA+C, **Paste** RA+V.

**Files**
- **Open SFTP browser** in an ssh tab: the file browser on the same
  host, with the same login, in a tab of its own.
- **Send file** with Ymodem, Xmodem or Hydra.
- **Receive file** with Ymodem or Xmodem.
- **Show transfers...** a window that lists every transfer, with a Stop
  button for each.
- **Start log...** RA+L, **Stop log** (see LOGGING).

**Panes**
- **New Shell pane**, **Close pane and tab**.
- **Focus** and **Move** (Left, Right, Up, Down): go to the pane on
  that side, or change places with it.
- **Fill the window** the pane fills the window; again: back.
- **New panes side by side**, **New panes one below other**,
  **Automatic split (dwindle)**: which way the next pane goes.
- **New Shells in panes**, **New Shells in tabs**: where New Shell and
  Left Amiga+Return put a new Shell.
- **Save layout...**, **Open layout...**, **Open this layout at
  start** (see PANES).
- **All panes to tabs**, **All tabs to panes**.

**View**
- **Bigger text** RA++, **Smaller text** RA+-.
- **Screen**: a Workbench window; CTerm's own screen with 16 or 256
  colours; or its own screen like Workbench.
- **Full screen** RA+F. **Scroll bar** on or off.
- **Characters**: UTF-8, PC BBS (CP437). These change only the shown
  tab.

**Settings**
- **Settings...** RA+S (see SETTINGS).
- **Amiga font...**, **TrueType font...**

## TABS
Up to 16 terminals can run in one window. A row of tab names shows
above the text: click one, or press RA+1 to RA+9 for the first nine.
Ctrl+Tab goes to the next tab, Ctrl+Shift+Tab to the one before. Tabs
you are not looking at keep running.

## PANES
Panes split the window, as tmux and i3 do. Every pane shows a tab, and
each pane keeps at least 10 letters by 3 lines. There can be up to 16.

Click in a pane to type there; it gets a frame and the solid cursor.
Pull the line between two panes with the mouse to give one more room.
Each Shell or host is told its own pane's size, and each pane keeps its
own history (and its own scroll bar, when the Scroll bar is on).

The keys, as on i3 (L-Amiga is the Left Amiga key):

    L-Amiga+Return       a new Shell: a new pane (or a new tab,
                         with Panes/New Shells in tabs)
    L-Amiga+Q            close the pane and its tab (asks first
                         while a file transfer runs there)
    L-Amiga+arrows       go to the pane on that side
    L-Amiga+Shift+arrows change places with the pane on that side
    L-Amiga+H            new panes from this one side by side
    L-Amiga+V            new panes from this one one below other
    L-Amiga+D            dwindle on or off
    L-Amiga+F            fill the window; again: back
    L-Amiga+T            all panes become tabs
    L-Amiga+P            all tabs become panes
    L-Amiga+1 to 9       workspace 1 to 9
    L-Amiga+Shift+1 to 9 move the tab you are in to that workspace
    L-Amiga+Ctrl+1 to 9  tab 1 to 9 of this workspace into this pane
    L-Amiga+R            resize mode (Return or Esc ends it)
    L-Amiga+-            show, use or hide the scratchpad
    L-Amiga+Shift+-      the tab you are in becomes the scratchpad

**Dwindle** (as Hyprland's layout): a new pane splits the one you are
in along its longer side. On a wide screen the panes spiral inwards.
With dwindle on, L-Amiga+H or V chooses the way for the next pane only.

**Workspaces**: nine, each with its own panes and tabs. A bar at the
top shows the ones in use; click one to go there. An empty workspace
opens with a new Shell. Every tab keeps running, whichever workspace is
shown.

**The scratchpad**: one Shell that floats over the panes. L-Amiga+-
shows it; again gives it the keys; again hides it. In resize mode the
arrows size it and Shift+arrows move it. For a tiled pane, the arrows
in resize mode move its edge.

**All tabs to panes** lays the tabs out by themselves: 2 side by side,
4 as a 2 x 2 grid, and so on. With an odd number, the tab you are in
gets twice the room. Tabs that do not fit stay tabs.

**Saved layouts** (Panes/Save layout..., Open layout...) keep every
workspace's panes, their tabs and the scratchpad, in
`ENVARC:CTerm/Layouts`. A Shell comes back in its drawer with the name
of the program that ran in it; a host is called again. "Open this
layout at start" opens it each time CTerm starts.

While the window is split, every pane uses the window's own font. A
tab's own font (a PC BBS, a phone book place) comes back when it is
alone in the window.

L-Amiga+N and L-Amiga+M stay the Amiga's (screens to front and back).
L-Amiga with the arrows normally moves the mouse pointer; in CTerm's
window CTerm takes them, but only while that window is active.

## KEYS
CTerm's own keys make CTerm do something. All other keys go to the
program in the tab, as on any terminal: arrows, function keys and Alt
as Meta are sent as VT100 and xterm (or, in the Amiga Shell, the Amiga
console) say. A key given to CTerm no longer reaches the program.

Every one of CTerm's keys can be changed, in Settings (the Keys tab) or
in the file `ENVARC:CTerm/CTerm.keys`. CTerm writes this file with the
keys below the first time it starts. A change in the file counts the
next time CTerm starts.

The file has six sections: `[window]` (in every tab), `[panes]`,
`[call]`, `[shell]`, `[files]` and `[filesline]`. Each section has its
own keys, so one key can do one thing in the Shell's line and another
in the file browser. One key a line, the key first, then the action:

    <A-t>       newtab
    <L-Return>  open
    none        about
    <F9>        none

A key is a character, or a name in angle brackets: `<Up>` `<Down>`
`<Left>` `<Right>` `<Return>` `<Esc>` `<Tab>` `<Space>` `<Del>` `<BS>`
`<Help>` `<F1>` to `<F12>` `<PgUp>` `<PgDn>` `<Home>` `<End>` `<Ins>`.
In front may come `A-` (Right Amiga), `L-` (Left Amiga), `C-` (Ctrl),
`M-` (Alt) and `S-` (Shift, with the names only): `<C-S-Tab>`,
`<L-S-Left>`. `none ACTION` gives an action no key; `KEY none` takes a
key away from CTerm. Lines starting with `;` or `#` are notes.

In Settings, choose a line in the Keys tab, then Press a key and the
key you want, or type the keys in the field (`<A-n> <F9>`) and press
Return. None takes the keys away; Defaults puts every key back. "Press
a key" cannot catch Left Amiga with an arrow (the pointer moves): type
it as `<L-Left>`.

The keys CTerm comes with (RA is Right Amiga, LA Left Amiga):

    [window] - in every tab
    RA+T            newtab        New Shell tab
    RA+K            call          Call...
    RA+B            phonebook     Phone book...
    RA+W            closetab      Close tab
    RA+L            log           Start log...
    RA+U, RA+P      senduser, sendpass
    RA+Q            quit          Quit
    RA+C, RA+V      copy, paste
    RA+S            settings      Settings...
    RA++, RA+-      bigger, smaller
    RA+F            fullscreen    Full screen
    RA+1 to 9       tab1 .. tab9  that tab
    Ctrl+Tab        nexttab       the next tab
    Ctrl+Shift+Tab  prevtab       the tab before
    Shift+Up/Down   pageup, pagedown   the history a page up, down
    (no key)        serial answer sftp logstop rexx hide about
                    speedtest yrecv xrecv ysend xsend hsend
                    transfers splitside splitunder nextpane
                    closepane font ttfont workbench screen16
                    screen256 screendeep scrollbar utf8 pcbbs
                    lineup linedown historytop historyend

    [panes] - the i3 keys, in every tab
    LA+Return       open          a new Shell (pane or tab)
    LA+Q            kill          close the pane and its tab
    LA+arrows       focusleft focusright focusup focusdown
    LA+Shift+arrows moveleft moveright moveup movedown
    LA+H, LA+V      splith, splitv
    LA+F            full          fill the window
    LA+D            dwindle       split by shape, on or off
    LA+T, LA+P      tabs, panes   all to tabs, all to panes
    LA+1 to 9       ws1 .. ws9    that workspace
    LA+Shift+1 to 9 movews1 .. movews9  the tab to that workspace
    LA+Ctrl+1 to 9  show1 .. show9      that tab in this pane
    LA+R            resize        resize mode
    LA+-            scratch       the scratchpad
    LA+Shift+-      toscratch     this tab becomes the scratchpad

    [call] - calls and transfers
    Esc             stop          stop a transfer, give up a call
    Return          redial        call again after a call ended

    [shell] - the Amiga Shell's line, as you type it
    Left, Right     left, right   a letter left, right
    Shift+Left, Ctrl+A   start    to the start
    Shift+Right, Ctrl+E  end      to the end
    Ctrl+Left/Right wordleft, wordright
    Backspace, Del  back, delete  the letter before, under
    Ctrl+W          wordback      delete the word before
    Ctrl+U, Ctrl+K  killstart, killend  delete to start, to end
    Ctrl+X          clear         clear the line
    Up, Down        older, newer  the lines typed before
    Ctrl+R          search        find an earlier line
    Tab             complete      complete a file name
    Return          accept        enter the line
    Ctrl+\          eof           end of input (empty line)

    [files] - the file browser's panels
    Up, Down        up, down
    Shift+Up, PgUp  pageup        a page up
    Shift+Down, PgDn pagedown     a page down
    Ctrl+Up, Home   top           the first line
    Ctrl+Down, End  bottom        the last line
    Left, Right     left, right   the Amiga's panel, the host's
    Tab             switch        the other panel
    Return          open          into the drawer under the bar
    Backspace       parent        the drawer above
    Space, Ins      mark          mark or unmark, then down
    *               invert        turn all the files' marks over
    F1, Help        help          the keys and the commands
    F2, Ctrl+R      refresh       read both drawers again
    F5              copy          copy to the other side
    F6              rename        rename
    F7              mkdir         a new drawer
    F8, Del         delete        delete (asks first)
    F9              log           the messages
    F10             quit          hang up, close the tab (asks)
    Esc             stop          stop a transfer or a listing
    :               command       the command line
    Ctrl+L          redraw        draw everything again

    [filesline] - a line typed in the file browser
    Left, Right     left, right
    Shift+Left, Ctrl+A, Home   start
    Shift+Right, Ctrl+E, End   end
    Backspace, Del  back, delete
    Shift+Backspace, Ctrl+U    kill (delete to the start)
    Return, Esc     accept, cancel
    Tab             complete      complete a name or a command
    Up, Down        older, newer  the command lines before, after

LA+Shift+1 to 9 and LA+Shift+- follow your keymap: on some keymaps
Shift+3 is a pound sign, and the file shows that character.

The Shell's line keys count while the Shell (or a program) reads a
line. A program that takes the keys itself (Ed, More) gets them as they
are. Ctrl+C to Ctrl+F stay the Amiga's break keys. In questions, yes
and no are always y and n.

## PHONE BOOK
Call/Phone book... (RA+B) opens the phone book window: the places on
the left (New, Copy, Delete), the one chosen on the right. What you
type is saved when the window closes. Call, or a double-click on a
place, calls it in a new tab. Each place has:

- a name, the address (a host name, an IP number, or a `telnet://`,
  `ssh://`, `sftp://` or `ftp://` link) and the port;
- the kind: telnet, ssh, serial, or sftp and ftp (the file browser).
  For sftp and ftp, More... sets the drawer the browser starts in;
- your user name and password. An ssh login is given the saved
  password by itself, once;
- the letters (CTerm's setting, an Amiga font, TrueType, or the PC's
  VGA font) and the characters (UTF-8, Latin-1, PC BBS);
- its own download and upload drawers (empty: the Settings' ones);
- a script: an ARexx script run when the call is up, to log in by
  itself.

For a serial place, Phone is the number the modem dials; with no
number, the line itself is the connection (a null modem). Line... sets
the device, unit, speed, data bits, parity, stop bits, flow control,
buffer, whether the speed is locked, and the place's own init string
and dial prefix.

The book is kept in `ENVARC:CTerm/phonebook` as plain text, one
`key=value` a line, an empty line between places. Passwords are kept
there as they are, as the BBS dialers of old did.

## SERIAL LINES AND MODEMS
CTerm works with any device that works as serial.device does, any unit,
at any speed the device takes (300 to 460800 baud and others).

In a serial terminal you type the AT commands yourself. CTerm notices
CONNECT and NO CARRIER. A phone book place on a serial line dials by
itself: the modem's init strings (Settings, Modem tab), the place's
own, the dial prefix, the number. BUSY and NO ANSWER call again if
Settings says so. Esc while dialling stops and hangs up.

Flow control is RTS/CTS (the default), XON/XOFF or none. "Baud locked"
(on by default) keeps the line's speed; off, it follows the speed in
the CONNECT message.

Closing the tab hangs up: "+++", then ATH0 (or your own command), then
the line closes, which drops DTR. Call/Answer call (ATA) picks up a
call; with "Answer when it rings" on, CTerm answers by itself.

## FILE TRANSFERS
- **Zmodem** starts by itself when the host begins one (a BBS
  download, or `sz file` on Linux). When the host waits for files (a
  BBS upload, or `rz`), a requester asks which; several may be chosen.
- **Hydra** starts by itself too, and goes both ways at once: files you
  queued with Files/Send file/With Hydra... go out while the host's
  come in.
- **Ymodem** and **Xmodem** you start yourself, after starting the
  other side: Files/Receive file and Files/Send file. Xmodem asks for
  the name when receiving.

Files arrive in the phone book place's download drawer, else the
Settings' (Files tab), else `PROGDIR:Downloads` (made when missing). A
file that is there already is never written over: a number is added.

CTerm goes on while files move: switch tabs, use a Shell, call another
board. A panel over the tab's last lines shows the file, bars for the
file and the batch, the speed and the time left, and a Stop button (Esc
stops too). The tab's name shows the per cent. Transfers work over a
serial line as over telnet.

## SFTP AND FTP BROWSER
The file browser copies files to and from other computers with SFTP
(files over ssh, as safe as ssh) or FTP (the password goes over the
network as it is: use it only on your own network, or where that does
not matter). It shows in the tab itself, in two panels, the way
Midnight Commander does on Linux.

Open it with a phone book place of the kind sftp or ftp, an `sftp://`
or `ftp://` link in the Call window, `CTerm SFTP ...` or `CTerm FTP
...`, ARexx's SFTP and FTP, or Files/Open SFTP browser in an ssh tab.

The left panel is the Amiga (it starts in the download drawer), the
right one the other computer. Each lists `..` (the drawer above), then
drawers, then files, with sizes and dates. Above the Amiga's volumes,
`..` lists the volumes. Under the panels is the message line, and at
the bottom the function keys. The keys are in the KEYS table above
(`[files]`); the mouse works too: a click chooses a line, a
double-click goes into a drawer, the wheel scrolls, a click on a
function key's name does what it says.

The command line (**:**) takes the commands of Linux's ftp and sftp.
Names with a space go in quotes; `#?` and `*` match any letters, `?`
one; Tab completes a name or a command:

    ls [pattern]     lls [pattern]   list there, list on the Amiga
    cd drawer        lcd drawer      change drawer there, on the Amiga
    pwd              lpwd            where each side is
    get name...      mget pattern... copy from there to the Amiga
    put name...      mput pattern... copy from the Amiga to there
    mkdir name                       a new drawer there
    rm name...       rmdir name      delete there (asks first)
    rename old new                   rename there
    bye                              hang up and close the tab
    help                             the keys and the commands

A file that is on the Amiga already is never written over: a number is
added (`name.1`). A name the Amiga cannot take gets `_` in place of
`:` or `/`. Sending a file that is on the other computer already asks
first. Drawers are not copied: mark their files.

A stopped download keeps `name.part`; copying the same file again goes
on from where it stopped. One transfer at a time runs in all of CTerm.

SFTP checks the host's key and logs in as ssh does. FTP uses the phone
book's user and password, asks in the tab for a password it has not
got, or logs in as anonymous with no user. Only passive FTP works; FTP
with TLS (FTPS) and copying whole drawers are not there yet.

## SSH
ssh is in a second program, CTermSSH, kept beside CTerm. CTerm starts
it when ssh is asked for, so it is not loaded otherwise. It needs AmiSSL
5.

A key file is used if there is one: `ENVARC:CTerm/id_ed25519`,
`id_ecdsa` or `id_rsa` (made with ssh-keygen, no passphrase; put the
.pub file in the host's `~/.ssh/authorized_keys`). Otherwise CTerm asks
for the password in the window.

A host met for the first time shows its key's fingerprint and asks.
The answer is kept in `ENVARC:CTerm/known_hosts`. If a host's key
changes, CTerm warns in red and does not connect unless told to.

## FOR PROGRAMS ON OTHER COMPUTERS
In a telnet, ssh or relay tab CTerm behaves like a modern xterm. Alt is
Meta (Alt+B and Alt+F in bash). Programs can use the mouse (vim, htop,
mc); Shift and the mouse still mark text. When the window changes
width, the text and the history flow again to the new width.

Programs can show pictures inside the text: sixel (img2sixel, chafa)
and iTerm2's picture files (imgcat). The setting is in Settings, Text
tab. PNG and JPEG files, and pictures on screens of 256 colours or
fewer, need `Parts/Pictures`. `ShowPic FILE [WIDTH W] [HEIGHT H]`, in
CTerm's Pictures drawer, shows a picture file in the Amiga Shell.

## COPY AND PASTE
Mark text with the left mouse button: drag, double-click a word (a file
name or a path is one word) or triple-click a line. RA+C copies it,
RA+V types the clipboard in. Programs on the other computer can copy
to the Amiga's clipboard too (vim, tmux: the OSC 52 code), but cannot
read it.

## LOGGING
Files/Start log... (RA+L) saves everything that comes into the tab to a
file, added to its end if the file is there. The title says "(logging)"
until Files/Stop log or the tab closes. The bytes are kept as they
came, colour codes and all, so a BBS screen plays back from the file.

## HIDING THE WINDOW
CTerm/Hide window, or the window's iconify gadget, closes the window
(and CTerm's own screen), but everything goes on: Shells, calls,
transfers, logs and ARexx scripts. A CTerm icon shows on Workbench. To
bring it back: double-click the icon; start CTerm again with no options
(or with SHOW); use Exchange (CTerm, Show Interface); or ARexx's
`WINDOW SHOW`. It comes back as it was. While a program has the window
to itself (Ed, More) it cannot hide; you hear a beep.

## SETTINGS
Settings/Settings... (RA+S) opens one window with six tabs:

- **Screen** a Workbench window or CTerm's own screen (any mode and
  number of colours), full screen, scroll bar, the gap between panes
  (0 to 32 pixels), outer gaps, the pane border.
- **Text** an Amiga font or TrueType, the size, line spacing, the
  characters (UTF-8, Latin-1, PC BBS), pictures in the terminal.
- **Colours** a theme (Nord, Black, Workbench, Amber, Green, Light),
  your own colours, the pane borders' colours, and a picture behind the
  text (it needs a screen of 16 bits of colour or more).
- **Keys** what the Alt key does, and every one of CTerm's keys.
- **Files** where downloads go, where the upload requester opens, and a
  program every new Shell tab runs after S:Shell-Startup (for example
  csh; empty: the Shell alone).
- **Modem** the modem for dial-up places (init strings, dialling,
  hanging up, redialling, answering) and the serial terminal's line.

Save keeps the settings, Use keeps them until the next reboot, Cancel
puts everything back. Letters and colours change at once; the screen
changes on Use or Save.

## ARexx PORT
The first CTerm's port is **CTERM**, a second one's **CTERM.2**, and so
on (CTerm/About says which). RexxMast must be running. Scripts run from
CTerm/Run ARexx script..., as a phone book place's login script, or
from the Shell with `rx` and `ADDRESS CTERM`.

Commands may be in any case. Put `OPTIONS RESULTS` in the script to get
answers in RESULT. RC is 0 when all went well, 5 for a warning (such as
a time limit), 10 for an error, 20 for a failure. Commands go to the
tab the script last called, opened or chose, else to the shown tab.

    CALL name [NOSCRIPT]     call a place, host:port, or SERIAL
    HANGUP                   hang up the tab's call
    SEND [RAW] "text"        type text (\r Return, \n new line,
                             \e Esc, \t Tab, \xHH any byte)
    WAITFOR "text"... [TIMEOUT secs]   wait for one of the texts
    GETLINE [row]            the text of a row (the cursor's)
    GETSCREEN                the whole screen
    CURSOR                   where the cursor is: "row column"
    TAB NEW|SELECT n|CLOSE [n]|COUNT|CURRENT|NAME [n]
    PANE SIDE|UNDER|NEXT|SELECT n|CLOSE|COUNT|CURRENT
    PANE OPEN|KILL|SPLIT H|SPLIT V|FULL|SHOW n
    PANE FOCUS dir|MOVE dir  dir: LEFT RIGHT UP DOWN
    PANE MODE TABS|PANES
    DOWNLOAD [file] [PROTOCOL p] [TIMEOUT secs]
    UPLOAD file... [PROTOCOL p]  p: zmodem ymodem xmodem hydra
    LOG file | LOG OFF       log the tab to a file, or stop
    SCREEN TOFRONT|TOBACK
    WINDOW TOFRONT|ACTIVATE|TOBACK|HIDE|SHOW|STATE
    PHONEBOOK                the places' names, each in quotes
    SFTP place, FTP place    open the file browser in a new tab
    LIST                     the browser's files (drawers end in /)
    CD drawer                go to a drawer there
    GET name... [TO drawer]  download from the browser's drawer
    PUT file...              upload into the browser's drawer
    MKDIR name, DELETE name, RENAME name TO newname
    VERSION, HELP [command], QUIT

A known bug: the QUIT command does not close CTerm while a Shell tab
is open; close the Shell tabs first (`endcli` in each).

In a script, put each command in quotes as a whole, with the text in
double quotes inside: `'SEND "guest\r"'`. ARexx turns words outside
quotes into capitals.

The Rexx drawer beside CTerm has examples: `bbs-login.rexx` (a board's
login, for a phone book place's script) and `info.rexx` (what CTerm
says about itself).

## FILES
    ENV:CTerm.prefs           the settings (Use)
    ENVARC:CTerm.prefs        the settings (Save)
    ENVARC:CTerm/CTerm.keys   CTerm's keys
    ENVARC:CTerm/phonebook    the phone book
    ENVARC:CTerm/known_hosts  ssh hosts' keys met before
    ENVARC:CTerm/id_ed25519   your ssh key (or id_ecdsa, id_rsa)
    ENVARC:CTerm/relay_from   the PC LISTEN lets in
    ENVARC:CTerm/Layouts/     saved pane layouts
    PROGDIR:Downloads         downloads, when no drawer is set

## EXAMPLES
    CTerm
    CTerm SHOW
    CTerm HOST bbs.example.org
    CTerm HOST bbs.example.org:2323 PC
    CTerm HOST amiga.example.org LATIN1
    CTerm SSH me@192.168.1.10
    CTerm SSH me@host.example.org:2222 KEY ENVARC:CTerm/id_ed25519
    CTerm SFTP me@192.168.1.10/home/me
    CTerm FTP ftp.aminet.net
    CTerm BOOK "AmigaNet BBS"
    CTerm SERIAL BAUD 19200
    CTerm SERIAL DEVICE serial.device UNIT 1
    CTerm COLS 100 ROWS 40 SHELL csh

A login script for a phone book place:

    /* login.rexx */
    OPTIONS RESULTS
    'WAITFOR "name:" TIMEOUT 30'
    IF RC ~= 0 THEN EXIT 5
    'SEND "guest\r"'

## RETURN CODES
0 fine. 20 when CTerm could not start: bad options, no fast RAM, a
phone book name that is not there, no TCP/IP stack for a call, or no
window.

## NOTES
CTerm needs AmigaOS 3.2 or later and fast RAM. ssh and SFTP need AmiSSL
5 (amisslmaster.library and LIBS:AmiSSL/). A call needs a TCP/IP stack
(bsdsocket.library); the Amiga Shell and serial lines do not.

The phone book keeps passwords as plain text.

`CTerm ?` shows the options' template.

## SEE ALSO
`csh`, `sumi`, `maki`, `ahiru`, `man`
