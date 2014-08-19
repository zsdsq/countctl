#include "head.h"

struct cli_connection{
	int sfd;
	struct msg_head last_hdr;
};

/////////////////////////////////////////
int ask_srv_for_blob(struct cli_connection* conn);
int reset_hdr(struct cli_connection* conn);
////////////////////////////////////////

//
// main
//
int init_connection(struct cli_connection* conn);
int work_with_srv(struct cli_connection* conn);
int close_connection(struct cli_connection *conn);

int main(){

	struct cli_connection conn;

	init_connection(&conn);

	work_with_srv(&conn);

	close_connection(&conn);

	return 0;
}

//
// init connection
//

int connect_to_srv(struct cli_connection* conn);

int init_connection(struct cli_connection* conn){

	if ((conn->sfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1){
		printf("soket");
		return 0;
	}

	printf("Try to connect\n");

	connect_to_srv(conn);

	printf("Connected %d\n", getpid());

	reset_hdr(conn);

	return conn->sfd;
}

// connect to server

int connect_to_srv(struct cli_connection* conn){
	struct sockaddr_un addr;
	int len, err;

	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, SERV_PATH);
	len = strlen(addr.sun_path) + sizeof(addr.sun_family);

	do {  // listener queue is full or stopped by signal, try to reconnect
		err = connect(conn->sfd, (struct sockaddr *)&addr, len);
	} while (err == -1 && (errno = ECONNREFUSED || EINTR)); 
																												 
	if ( err == -1) {
		printf("connect");
		return 1;	
	}

	return 0;
}

//
// work with server
//

int read_msg(struct cli_connection* conn, char* buff);
int process_msg(struct cli_connection* conn, char* buff, int size);

int work_with_srv(struct cli_connection* conn){
	char buff[BUFFSZ];
	int rc;

	ask_srv_for_blob(conn);

	while(1){

		rc = read_msg(conn, buff);

		rc = process_msg(conn, buff, rc);

		if (rc) break;
	}
}

// read message

int read_msg(struct cli_connection* conn, char* buff){
	int msg_sz;

	if (conn->last_hdr.code==MSG_BLOB)
		msg_sz = conn->last_hdr.blob_size;
	else msg_sz = HEADSZ;

	return read(conn->sfd, buff, msg_sz);
}

// process message

int process_blob(struct cli_connection* conn, char* buff);
int process_command(struct cli_connection* conn, char *buff);

int process_msg(struct cli_connection* conn, char* buff, int size){
	int res;

	switch(conn->last_hdr.code){
	case MSG_BLOB:
		res = process_blob(conn, buff);
		break;
	default:
		res = process_command(conn, buff);
	}
	return res;
}

// process blob

int send_result(struct cli_connection* conn, int *count_a);
int count(int* count_a, const unsigned char* blob, int sz);

int process_blob(struct cli_connection* conn, char* buff){
	int count_a[CHAR_CT];

	count(count_a, buff, conn->last_hdr.blob_size);

	send_result(conn, count_a);

	reset_hdr(conn);
	
	ask_srv_for_blob(conn);

	return 0;
}

int send_result(struct cli_connection* conn, int *count_a){
	int res;
	struct msg_head m_msg;

	m_msg.code = MSG_BLOB;
	m_msg.blob_size = sizeof(int)*CHAR_CT;

	res = write(conn->sfd, &m_msg, HEADSZ);

	res = write(conn->sfd, count_a, m_msg.blob_size);

	return res;
}

int count(int* count_a, const unsigned char* blob, int sz){
	int i;

	for (i=0; i<CHAR_CT; i++){
		count_a[i]=0;
	}	

	for (i=0; i<sz; i++){
		(count_a[blob[i]])++;
	}

	return 0;
}

// process command

int process_command(struct cli_connection* conn, char *buff){
	conn->last_hdr = *((struct msg_head*) buff);
	if (conn->last_hdr.code==TERM)
		return 1;
	return 0;
}

//
// close connection
//

int close_connection(struct cli_connection *conn){
	close(conn->sfd);
	return 0;
}

///////////////////////////////////////////////////////////////

int reset_hdr(struct cli_connection* conn){
		conn->last_hdr.code = MSG_NULL;
		conn->last_hdr.blob_size = HEADSZ;
		return 0;
}

int ask_srv_for_blob(struct cli_connection* conn){
	struct msg_head m_msg;

	m_msg.code = CLI_MSG_WAIT;
	m_msg.blob_size = HEADSZ;

	return write(conn->sfd, &m_msg, HEADSZ);
}

