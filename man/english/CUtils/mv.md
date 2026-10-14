# mv - move or rename files and drawers, never overwriting by default

## SYNOPSIS
    mv [-fb] from ... to

## DESCRIPTION
Moves files and drawers to another place, or gives them a new name. The
last name is where they go (TO); the names before it are what is moved
(names or patterns). With one plain source, TO is its new name, or the
drawer it moves into when TO is a drawer. With several sources or a
pattern, TO must be an existing drawer.

On one volume a move is just a rename: instant, for a file or a whole
drawer. Between volumes a file is copied (keeping its protection bits,
date and filenote) and the original deleted. A drawer is moved whole or
not at all: the whole tree is copied first, and the original is deleted
only when every file copied cleanly. If anything fails, the partial
copy is deleted and the original is left untouched.

mv is careful: a target that already exists is *not* replaced. It is
skipped, and all skipped names are listed under "not moved:" at the
end. Use **-f** or **-b** to replace it.

## OPTIONS
- **-f** delete an existing target and put the source in its place.
- **-b** keep a backup: rename the existing target to NAME.mvbak first.
  If that name is taken too, the source is not moved (and this is said).
  With **-bf** the old .mvbak is replaced: **-f** always allows
  destroying exactly one thing.

## EXAMPLES
    mv RAM:letter.txt Work:Docs
    mv Work:Docs/letter.txt Work:Docs/letter-old.txt
    mv Work:Downloads/#?.lha Work:Archives
    mv -b RAM:new-startup S:User-Startup
    mv readme ReadMe
    mv Work:Projects/Game DH1:Old

## RETURN CODES
0 fine, 5 something was skipped, or a moved source could not be deleted
(said), 10 something failed, 20 stopped with Ctrl-C.

## NOTES
Neither **-f** nor **-b** touches anything when the source and the
target are the same file, or when the target is a drawer. A volume
itself (`Work:`) is never moved. Changing only upper and lower case in a
name (`mv readme ReadMe`) works.

Between volumes, a drawer cannot be moved onto a drawer that exists
already (merging is cp's job), and a tree with a link in it is refused:
a link cannot be made again faithfully on another volume. A single link
is not moved between volumes either.

Ctrl-C during a copy between volumes leaves the partial copy at TO and
the original untouched, and says so.

Patterns may be written `#?` or `*`; upper and lower case are the same.

`mv ?` shows the usage line.

## SEE ALSO
`cp`, `rm`, `ln`, `mkdir`, Rename, Copy
