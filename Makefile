all: dfs dfc

dfc: dfclient.c
	gcc -Wall -o dfc dfclient.c

dfs: dfserver.c
	gcc -Wall -o dfs dfserver.c

clean:
	rm dfs; rm dfc

compile:
	make clean; make

server1:
	./dfs DFS1 10001 &

server2:
	./dfs DFS2 10002 &

server3:
	./dfs DFS3 10003 &

server4:
	./dfs DFS4 10004 &

serverRun:
	  make server1; make server2; make server3; make server4
