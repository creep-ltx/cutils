# man - every manual page, for every program

CUtils/man is where every manual page lives: the CUtils commands, the
user's own programs and the third-party commands on the Amiga. The format
is in `docs/man.md`, section "The page format (.md)".

The rule: `man/<language>/<Program>/<page>`. Each program (or package) has
its own folder, so its pages are easy to find and easy to remove. Install
copies `man/<language>/` into `MAN:` as it is, so a page here at
`man/english/CShell/csh.md` is read by man as `MAN:english/CShell/csh.md`.

## The folders (english)

Our own programs:

| Folder | Pages |
|---|---|
| CUtils/ | the Unix-style commands (ls, grep, tar, man, ...) |
| CShell/ | csh (the shell), csh-builtins, cshell (csh as the user shell) |
| CTerm/ | cterm (the terminal) |
| Sumi/ | sumi (the editor) |
| Maki/ | maki (the pager) |
| Ahiru/ | ahiru (the Ranger-style file manager) |
| CFile/ | cfile (the file manager) and its cfile.config |
| Markdown/ | markdown (the Markdown viewer) |
| Wasabi/ | wasabid (the remote-control daemon on the Amiga) |
| AmigaTools/ | conbench (console speed benchmark) |
| exFAT/ | readbench (disk read benchmark, from exfat-aos3) |

Third-party commands (pages written from their own documents):

| Folder | Pages |
|---|---|
| Poseidon/ | the Poseidon USB stack's commands |
| SFS/ | the Smart File System's commands |
| Archivers/ | lha, unzip and the other archivers |
| Network/ | the TCP/IP stack's commands |
| System/ | AmigaOS commands without a page of their own elsewhere |

A new program gets a new folder; a new language gets `man/<language>/`
with the same folders inside.
