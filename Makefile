all: dfs dfc

dfc: dfclient.c
	gcc -W -o dfc dfclient.c

dfs: dfserver.c
	gcc -W -o dfs dfserver.c

clean:
	rm dfs; rm dfc

compile:
	make clean; make
