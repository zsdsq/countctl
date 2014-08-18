#include "head.h"

void count(const unsigned char* blob, int sz, int* count_a){
		int i;
		for (i=0; i<sz; i++){
			(count_a[blob[i]])++;
		}
}

int set_trans_block_sz(int sockfd, int size){
	int res;
	res = setsockopt(sockfd, SOL_SOCKET, SO_RCVLOWAT, (const char *) &size, sizeof(int));
	if (res!=0) {
		printf("cli setsockopt err");
		return 1;
	}

	return 0;
}

int main(){
	int sockfd, err, len;
	int count_a[CHAR_CT];
	int i, rcs;
	char buff[BUFFSZ];
	fd_set readfds;
	int max_d;
	struct timeval t;

	struct msg_head msg_h;
	struct msg_head m_msg;

	int itr;

	int res;
	struct sockaddr_un serv;

	for (i=0; i<CHAR_CT; i++){
		count_a[i]=0;
	}		

	if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1){
		printf("soket");
		return 1;
	}

	printf("Try to connect\n");

	serv.sun_family = AF_UNIX;
	strcpy(serv.sun_path, SERV_PATH);
	len = strlen(serv.sun_path) + sizeof(serv.sun_family);

	do {
		err = connect(sockfd, (struct sockaddr *)&serv, len);
	} while (err == -1 && (errno = ECONNREFUSED || EINTR)); // listener queue is full, try to reconnect later
																												 // or stopped by signal
	if ( err == -1) {
		printf("connect");
		return 1;	
	}

	printf("  --- Connectd %d --- \n", getpid());

	max_d = sockfd;
	t.tv_sec = 1000;
	t.tv_usec = 0;

	
	m_msg.code = CLI_MSG_WAIT;
	m_msg.size = HEADSZ;

	write(sockfd, &m_msg, HEADSZ);

	printf(" first send\n");
	itr=-1;

	//msg_h = (struct msg_head*) buff;
	msg_h.code = MSG_NULL;
	msg_h.size = HEADSZ;
	

	while(1){
		printf("c client\n");
		itr++;
		//if (itr>10) {printf("cli too much itr\n"); break;}

		// prepare new select iteration
		// fill select structures

		FD_ZERO(&readfds);			
		FD_SET(sockfd, &readfds);
		t.tv_sec = 1000;
		
		// set size of block, which we will be wait avaible
		
		set_trans_block_sz(sockfd, msg_h.size);

		res = select(max_d+1, &readfds, NULL, NULL, &t);

		if (res<0) {
			printf("cli err select\n");
			continue;
		}
		if (res==0){
			printf("cli timeout\n");
			continue;
		}
		
		// need block exist in socket
		// now we can read

		res = read(sockfd, buff, msg_h.size);
		printf("cli rec %d\n", res);
		if (msg_h.code!=MSG_BLOB){    
		// now work with new rec mess
			printf("cli rec msg\n");
			msg_h = *((struct msg_head*) buff);
			if (msg_h.code==TERM) 
				break;

		} else if (msg_h.code==MSG_BLOB){    
			count(buff, msg_h.size, count_a);

			m_msg.code = MSG_BLOB;
			m_msg.size = sizeof(int)*CHAR_CT;

			res = write(sockfd, &m_msg, HEADSZ);
			printf("cli h blob = %d\n", m_msg.size);

			res = write(sockfd, count_a, m_msg.size);
			printf("cli blob = %d\n", res);

			m_msg.code = CLI_MSG_WAIT;
			m_msg.size = HEADSZ;

			res = write(sockfd, &m_msg, HEADSZ);

			msg_h.code = MSG_NULL;
			msg_h.size = HEADSZ;
		}
	}
	sleep(10);

	close(sockfd);

	printf("end %d\ncli grp %d\n", getpid(), getpgrp());
	
	return 0;
}
