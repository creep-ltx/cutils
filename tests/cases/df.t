$ df RAM: | head -n 1
      Size       Used       Free Use% Volume
? 0
$ df -h RAM: | head -n 1
  Size   Used   Free Use% Volume
$ df RAM: | wc -l
2
$ df RAM: RAM:cutb | wc -l
3
$ df nothere:
      Size       Used       Free Use% Volume
df: nothere:: device (or volume) is not mounted
? 5
$ df -x
df: -x: unknown option
? 10
$ df ?
df [-h] [path ...]
? 0
