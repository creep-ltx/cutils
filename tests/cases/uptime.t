$ uptime | grep -c " up .**shell"
1
? 0
$ uptime -p | cut -c1-3
up 
$ uptime -s | cut -c1-2
20
$ uptime -x
uptime: -x: unknown option
? 10
