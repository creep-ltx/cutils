$ file ?
file [-b] file | pattern ...
$ file fx#?
fx.8svx: IFF 8SVX sound, 8363 Hz
fx.adf: Amiga bootblock, FFS
fx.anim: IFF ANIM animation, 640 x 512, 4 planes (16 colours)
fx.exe: AmigaOS executable
fx.gif: GIF image, 32 x 16
fx.guide: AmigaGuide document, ASCII text
fx.gz: gzip compressed data
fx.ilbm: IFF ILBM picture, 320 x 256, 5 planes (32 colours)
fx.info: Workbench icon (tool)
fx.jpg: JPEG image, 300 x 200
fx.lha: LhA archive (-lh5-)
fx.library: AmigaOS library
fx.mod: ProTracker module (M.K.), 4 channels, "test song"
fx.pdf: PDF document, version 1.4
fx.png: PNG image, 640 x 480, 8-bit RGBA
fx.zip: Zip archive
fx_ham.iff: IFF ILBM picture, 320 x 200, HAM6
fx_latin1: ISO-8859-1 text
fx_utf8: UTF-8 text
? 0
$ file a.txt crlf.txt empty.txt dbin diff1
a.txt: ASCII text
crlf.txt: ASCII text, with CRLF line ends
empty.txt: empty
dbin: data
diff1: ASCII text
$ file -b fx.png
PNG image, 640 x 480, 8-bit RGBA
$ chmod +p fx.exe
$ file fx.exe
fx.exe: AmigaOS executable, pure
$ chmod -p fx.exe
$ rm -rf fzd
$ MakeDir fzd
$ Echo >fzd/s "echo hi"
$ Protect fzd/s +s
$ Echo >fzd/t.rexx "/** test **/"
$ Echo >fzd/sh "#!/bin/sh"
$ Echo >fzd/esc "*e[1mbold*e[0m"
$ ln -s a.txt fzd/lnk
$ file fzd fzd/#?
fzd: drawer
fzd/esc: ASCII text, with escape sequences
fzd/lnk: soft link to a.txt
fzd/s: AmigaDOS script, ASCII text
fzd/sh: script for /bin/sh, ASCII text
fzd/t.rexx: ARexx script, ASCII text
$ file nofile
file: nofile: object not found
? 5
$ file
file: no file named (file ? for usage)
? 10
$ rm -r fzd
