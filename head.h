#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>

struct msg_head{
	int code;
	int size;   // in case simple command store size of struct, else size of blob
};

#define SERV_PATH "/tmp/superserv"

#define BUFFSZ 4096
#define CHAR_CT 256
#define HEADSZ sizeof(struct msg_head)

#define CLI_MSG_WAIT 1
#define MSG_NULL 0

#define MSG_BLOB 2

#define TERM -1
// server and client send message as int
// server msg:
// > 0 - size of blob of text
// 0 - end
// client msg:
// > 0 - size 
// 0 - wait of next blob
