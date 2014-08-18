#include "head.h"

#define LOG_FILE_NAME "log.txt"

#define MAX_BLOBSZ BUFFSZ-HEADSZ

struct client{
	int sfd;
	struct msg_head last_hdr;
};

struct srv_connection{
	int srv_sfd;
	
	int cli_ct;
	int active_ct;
	struct client *cli;
};

struct select{
	fd_set readfds;
	int max_d;
	struct timeval t;
}

//
// main
//

int init_connection(struct srv_connection *conn, int cli_ct);
int make_daemon(char* logfile);
int create_workers(int n);
int process_file(FILE* file, struct srv_connection *conn);
int close_connection(struct srv_connection *conn);

int main(int argc, char *argv[]){

	struct srv_connection conn;

	FILE *fin;

	if (argc!=3) {
		printf("Bad param\n");
		return 1;
	}

	cli_ct = atoi(argv[2]);

	fin = fopen(argv[1], "r");
	if (fin == NULL) {
		printf("err file");
		return 1;
	}

	make_daemon(argv[1]);

	create_workers(cli_ct);
	
	init_connection(&conn);

	process_file(fin, &conn);

	fclose(fin);

	close_connection(&conn);

	return 0;
}

//
// init connection
//

int get_srv_socket();
int connect_clients(struct srv_connection *conn);

int init_connection(struct srv_connection *conn, int cli_ct){

	conn->cli_ct = cli_ct;

	conn->cli = (struct client*) malloc(conn->cli_ct*sizeof(struct client));

	conn->srv_sfd = get_srv_socket();
  
	connect_clients(conn);

	conn->active_ct = conn->cli_ct;

	for(i=0; i<conn->cli_ct; i++) {
		conn->cli[i].last_hdr.code=MSG_NULL;
	} 
}

// get server socket

int get_srv_socket(){
	int len;
	struct sockaddr_un addr;


	if ((sfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
	  printf("s_open_err");
	  return 0;
	}

	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, SERV_PATH);
	unlink(addr.sun_path);
	len = sizeof(addr.sun_family) + strlen(srv.sun_path);

	if (bind(sfd, (struct sockaddr *)&addr, len) == -1) {
	  printf("bind err");
	  return 0;
	}

	if (listen(sfd, 5) == -1) {
	  printf("listen err");
	  return 0;
	}

	return sfd;
}

// connect clients

int connect_clients(struct srv_connection *conn){
	int len;
	struct sockaddr_un addr;

	len = sizeof(addr);

  for(i=0; i<conn->cli_ct; i++){

    if ((conn->cli[i].sfd = accept(conn->srv_sfd, (struct sockaddr *)&addr, &len)) == -1) {
      printf("accept err");
      return 0;
    }

    printf("Connected: %d\n", i);
  }

	printf("\nall connected\n");

	return 0;
}

// 
// make daemon
// 

int make_daemon(char* logfile){
	int logfd;

	if ((logfd = open(LOG_FILE_NAME, O_CREAT|O_WRONLY)) != -1) {
		close(0);
		close(1);
		close(2);
		open("/dev/null", O_RDONLY); // block in		

		dup2(logfd, 1);
		dup2(logfd, 2);
		close(logfd);

		pid = setsid();   // change session

		//chdir("/");
	} else {
		printf("Error, cannot open log, process not will start\n");
	}

	return 0;
}

//
// create workers
//

int create_workers(int n){
	int i;

	for (i=0; i<n; i++){
		if (fork() == 0) {
			execlp("./client", "./client", (char *)NULL);
			// will be executed only if error
			printf("CLIENT BIG ERROR\n");
		}
	}		
}

//
// process file
//

int prepare_select(struct srv_connection *conn, struct select_struct* sel);
int process_select(struct srv_connection *conn, struct select s_val, FILE* file, int* count);
int process_msg(struct msg_head* prev_msg, struct msg_head* prev_msg);
int print_result(int arr*, int ct);

int process_file(FILE* file, struct srv_connection *conn){
	struct select s_val;
	int count[CHAR_CT];
	int res;

	while(1){
		if (!prepare_select(conn, &s_val)){
			printf("File has processed, All clients ended");
			break;
		}

		res = select(s_val.max_d+1, &(s_val.readfds), NULL, NULL, &(s_val.t));

		if (res<0) {
			printf("err select\n");
			continue;
		} else if (res==0){
			printf("\ns timeout s\n");
			continue;
		}

		process_select(conn, s_val, file, count);
	}

	print_result(count, CHAR_CT);	

	return 0;
}

// prepare select

int set_trans_block_sz(int sfd, int size);

int prepare_select(struct srv_connection *conn, struct select_struct* sel){

	FD_ZERO(&(sel->readfds));

	sel->max_d = 0;

	for(i=0; i<conn->cli_ct; i++) {
		if (conn->cli[i].last_hdr.code!=TERM) {
			FD_SET(conn->cli[i].sfd, &(sel->readfds));

			set_trans_block_sz(conn->cli[i].sfd, conn->cli[i].last_hdr.size);

			if (conn->cli[i].sfd > sel->max_d) 
				sel->max_d = conn->cli[i].sfd;
		}
	}

	sel->t.tv_sec = 5;	
	return max_d;
}

int set_trans_block_sz(int sfd, int size){
	int res;
	res = setsockopt(sfd, SOL_SOCKET, SO_RCVLOWAT, (const char *) &size, sizeof(int));
	if (res!=0) {
		printf("Error: set_trans_block_sz\n");
		return 1;
	}

	return 0;
}

// process select

int process_select(struct srv_connection *conn, struct select s_val, FILE* file, int* count){
	int i;
	for (i=0; i<conn->cli_ct; i++){
			
		if (!FD_ISSET(conn->cli[i].sfd, &(s_val.readfds)))
			continue;
		
		process_msg(conn->cli[i], file, count);
	}
}

// process message
int read_msg(struct client* cli, char* buff);
int store_count(int *count_to, int *count_from);

int process_msg(struct client* cli, FILE* file, int* count){

	char buff[BUFFSZ];
	int rc;
	struct msg_head m_msg;
	
	rc = read_msg(cli, buff);

	switch(cli->last_hdr.code){
	case MSG_BLOB:
		store_count(count, (int*) buff);
		cli->last_hdr.code=MSG_NULL;
		break;
	default:
		cli->last_hdr=*((struct msg_head*)buff);

		if (cli->last_hdr.code==MSG_BLOB)
			break;
	
		if (feof(file)){
			cli->last_hdr.code=TERM;
			break;
		}

		rc = fread(buff, sizeof(char), MAX_BLOBSZ, file);

		rc = send_new_blob(cli, buff, rc);
	}
}

int read_msg(struct client* cli, char* buff){
	int msg_sz;

	if (cli->last_hdr.code==MSG_BLOB)
	msg_sz = cli->last_hdr.blob_size;
	else msg_sz = HEADSZ;

	return read(cli[i].sfd, buff, msg_sz);
}

int store_count(int *count_to, int *count_from){

	int j;
	for(j=0; j<CHAR_CT; j++){
		count_to[j] += count_from[j];
	}

	return 0;
}

int send_new_blob(struct client* cli, char* buff, int sz){
	int rc;
	struct msg_head m_msg; 
	
	m_msg.code = MSG_BLOB;
	m_msg.size = sz;

	rc = write(cli->sfd, &m_msg, HEADSZ);
	printf("s send msg\n");

	rc = write(cli->sfd, buff, sz);
	printf("s send blob %d\n", rc);

	return rc;
}

// print result

int print_result(int arr*, int ct){
	int i;
	for(i=0; i<ct; i++) {
		printf("%c : %d\n", i, arr[i]);
	}
}

//
// close connection
//

int close_connection(struct srv_connection *conn){
	int i;
	for(i=0; i<conn->cli_ct; i++){
		send_term(conn->cli[i].sfd);
		close(conn->cli[i].sfd);
	}
	
	close(conn->srv_sfd);
}

int term_cli(int sfd){
	struct msg_head m_msg;

	m_msg.code = TERM;

	return write(sfd, &m_msg, HEADSZ);
}

