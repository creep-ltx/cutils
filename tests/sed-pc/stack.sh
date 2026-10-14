#!/usr/bin/env bash
# stack.sh - bin/sed under vamos with the Shell's usual 4 KB stack, on
# patterns that nest deep (regex.c compiles recursively, its matcher's add()
# recurses along SPLIT chains). Each must give the same answer as with 64 KB.
T=$(cd "$(dirname "$0")" && pwd)
mkdir -p "$T/w/stack" && cd "$T/w/stack" || exit 2
cp "$T/../../bin/sed" sed.am
bad=0
check() {
  local input=$1; shift
  big=$(printf '%b' "$input" | vamos -q --cpu 68020 -s 64 -- ./sed.am "$@" 2>&1)
  small=$(printf '%b' "$input" | vamos -q --cpu 68020 -s 4 -- ./sed.am "$@" 2>&1)
  if [ "$big" != "$small" ]; then echo "FAIL (4 KB stack) sed $*: '$small' (64 KB: '$big')"; bad=1; else echo "ok   sed $*  ->  $small"; fi
}
check 'aaab\n' -E 's/((((a)))){2}/X/'
check 'aaab\n' 's/\(\(\(\(a\)\)\)\)\{1,2\}/X/'
check 'aaaaaaaaaab\n' -E 's/a{0,100}b/X/'
check 'abcabc\n' -E 's/(a|b|c|d|e|f|g|h|i|j|k|l|m|n|o|p)+/X/'
check 'x\n' -E 's/(((((((((x)))))))))/[\9]/'
check 'ab\n' -i.bak 's/a/b/' nosuch
check 'aaa\n' -n '/a\{50\}/!p'
check 'ababababc\n' -E 's/(a|b){0,40}c/X/'
check 'aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n' -E 's/a?a{0,90}b?/X/'
check 'xyz\n' -E 's/((((((((((x|y|z)+))))))))){1,3}/[\1]/'
exit $bad
