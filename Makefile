run: server client countctl
#	./countctl /media/dim-ka/Archive1/altlibrary-linuxintro2.pdf 1
	./countctl ./1.c 10

countctl: countctl.c head.h
	gcc countctl.c -o countctl

server: server.c head.h
	gcc server.c -o server

client: client.c head.h
	gcc client.c -o client
