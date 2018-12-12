## Comments
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; Instead of hashing the file I just chose a random number and modded that by 4. The point of hashing is randomness so I believe this is more efficient and produces the same result. Also for the client I didn't have the '/' before the folder name. I also made a small change to the conf files to make parsing easier.

## Compile Client and Server
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; make compile;

## Run Client
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; ./dfc bob.conf

## Run Servers
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; make serverRun;  // this will start all servers.

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp; ./dfs DFS1 10001 & or make server1  // start individual server
