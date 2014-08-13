#include "head.h"

// client connection status
#define STAT_DEAD -1
#define STAT_COMM 0
#define STAT_MSG CHAR_CT

int set_trans_block_sz(int sockfd, int size){
	int res;
	res = setsockopt(sockfd, SOL_SOCKET, SO_RCVLOWAT, (const char *) &size, sizeof(int));
	if (res!=0) {
		printf("cli setsockopt err");
		return 1;
	}

	return 0;
}

int main(int argc, char *argv[]){

	// sockets
  int sockfd; 	 // this socked

  int *cli_sfd;  // clents socket fds
	struct msg_head *cli_msg_h; // status of client socket connection 
	int cli_ct;
	int active_cli_count;
	
	struct sockaddr_un srv, cli;
	int len;

	// for select
  fd_set readfds;
	int max_d;				// max id of fd in set
	struct timeval t; // timer

	struct msg_head* msg_h;
	struct msg_head m_msg;
	
	int i, j; // counters
	
	char buff[BUFFSZ];	
	int msg;	

	int res, rc;

	int count_c[CHAR_CT];
	int *count_c_t;

	FILE *fin;
	int fend;

	int itr;

	if (argc!=3) {
		printf("bad param\n");
		return 1;
	}

	fin = fopen(argv[1], "r");
	if (fin == NULL) {
		printf("err file");
		return 1;
	}
	fend = 0;

  cli_ct = atoi(argv[2]);

	cli_sfd = (int*) malloc(cli_ct*sizeof(int));
	cli_msg_h = (int*) malloc(cli_ct*sizeof(struct msg_head));

	if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
    printf("s_open_err");
    return 1;
  }

  srv.sun_family = AF_UNIX;
  strcpy(srv.sun_path, SERV_PATH);
  unlink(srv.sun_path);
  len = sizeof(srv.sun_family) + strlen(srv.sun_path);

  if (bind(sockfd, (struct sockaddr *)&srv, len) == -1) {
    printf("bind err");
    return 0;
  }
  
  if (listen(sockfd, 5) == -1) {
    printf("listen err");
    return 0;
  }

	len = sizeof(cli);

  for(i=0; i<cli_ct; i++){
    printf("\ntry con\n");    
		
    if ((cli_sfd[i] = accept(sockfd, (struct sockaddr *)&cli, &len)) == -1) {
      printf("accept err");
      return 0;
    }

    printf("con\n");
  }

	printf("\nall connected\n");

	active_cli_count = cli_ct;
	max_d = 0;
	t.tv_sec = 5;
	t.tv_usec = 0;

	for(i=0; i<cli_ct; i++) {
		cli_msg_h[i].code=MSG_NULL;
		cli_msg_h[i].size=HEADSZ;
	}

	itr = -1;
	while(1){
		printf("s server %d %d\n", itr, active_cli_count);
		itr++;

		//if (itr>10) {printf("serv too much itr\n"); break;}

		if (active_cli_count==0) break;

		FD_ZERO(&readfds);

		max_d = 0;
		for(i=0; i<cli_ct; i++) {
			if (cli_msg_h[i].code!=TERM) {
				FD_SET(cli_sfd[i], &readfds);

				set_trans_block_sz(cli_sfd[i], cli_msg_h[i].size);
				if (cli_sfd[i]>max_d) max_d = cli_sfd[i];
			}
		}

		t.tv_sec = 5;

		res = select(max_d+1, &readfds, NULL, NULL, &t);

		if (res<0) {
			printf("err select\n");
			continue;
		}
		if (res==0){
			printf("\ns timeout s\n");
			continue;
		}
		// can read

		for (i=0; i<cli_ct; i++){
			
			if (!FD_ISSET(cli_sfd[i], &readfds)){

				//printf("s not exist\n");
				continue;
			}

			res = read(cli_sfd[i], buff, cli_msg_h[i].size);

			printf("s rec --- %d %d %d\n", res, cli_msg_h[i].size, cli_msg_h[i].code);

			if (cli_msg_h[i].code!=MSG_BLOB){
				cli_msg_h[i] = *((struct msg_head*)buff);
				printf("s rec not blob\n");
				// now work with new rec mess
				if (cli_msg_h[i].code==MSG_BLOB)
					continue;
				
				if (!fend && feof(fin)){
					printf("s file end\n");
					fend = 1;	
					fclose(fin);		
				}	

				if (fend){ 
					printf("s close %d\n", itr);
					m_msg.code = TERM;
					write(cli_sfd[i], &m_msg, HEADSZ);
					cli_msg_h[i].code=TERM;
					close(cli_sfd[i]);
					active_cli_count--;
					continue;
				}

				rc = fread(buff, sizeof(char), BUFFSZ-HEADSZ, fin);

				m_msg.code = MSG_BLOB;
				m_msg.size = rc;

				res = write(cli_sfd[i], &m_msg, HEADSZ);
				printf("s send msg\n");
				res = write(cli_sfd[i], buff, rc);
				printf("s send blob %d\n", res);

			} else if (cli_msg_h[i].code==MSG_BLOB) {

				//printf("s rec blob %d %d\n", cli_msg_h[i].size, cli_msg_h[i].code);
				//rc = recv(cli_sfd[i], buff, cli_msg_h[i].size, MSG_DONTWAIT);

				
				count_c_t = (int *)buff;

				for(j=0; j<CHAR_CT; j++){
					//printf("arr %d\n", count_c_t[j]);
					count_c[j] += count_c_t[j];
				}
				
				cli_msg_h[i].code=MSG_NULL;
				cli_msg_h[i].size=HEADSZ;
			}
		}
	}	

  printf("\ns -------- good end -----\n");


	for(i=0; i<CHAR_CT; i++) {
		printf("\n %c : %d", i, count_c[i]);
	}

  close(sockfd);

	return 0;
}
