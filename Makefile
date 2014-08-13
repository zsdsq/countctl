run: server client countctl
	./countctl /home/dim-ka/main/step1/task.txt 1
#	./countctl ./1.c 10

countctl: countctl.c head.h
	gcc countctl.c -o countctl

server: server.c head.h
	gcc server.c -o server

client: client.c head.h
	gcc client.c -o client
