#include "head.h"

#define LOCK_FILE "lock"

int printHelp();
int stop();
int start(int argc, char *argv[]);

int main(int argc, char *argv[]){
	if (argc==2 && strcmp(argv[1], "-stop")==0) {
    stop();
  }else if (argc==3){
		start(argc, argv);
  } else {
		printHelp();
	}
	
	return 0;
}

int printHelp(){
	printf("Bad param.\nUsage:\n");
	printf("1: countctl filename nworkers\n");
	printf("2: countctl -stop\n");

	return 0;
}

int stop(){
	pid_t pid;
	FILE* fd = NULL;

	if ((fd = fopen(LOCK_FILE, "r")) == NULL) {
		printf("Currently no runned process\n");
		return 0;
	}

	fscanf(fd, "%d", &pid);
	printf("proc id %d\n", (int)pid);

	if (pid==0) printf("Read err\n");

	printf("Try to kill\n");

	if (kill(pid, SIGTERM)==-1) {
		switch(errno){
		case EPERM: printf("Not enought rights\n"); break;
		case ESRCH: printf("No active process\n"); break;
		}
	} else {
		printf("Done\n");
	}
	fclose(fd);
	return 0;
}

int start_server(int argc, char *argv[]){
		execlp("./server", "./server", argv[1], argv[2], NULL);
		// will be executed only if error
		printf("SERVER BIG ERROR\n");
}

int start(int argc, char *argv[]){
	FILE *lock;
	pid_t pid;

	pid = fork();
	if (pid){
		lock=fopen(LOCK_FILE, "w");
		if (lock) {
			fprintf(lock, "%d", (int)pid);
			fclose(lock);
		} else {
			printf("Error open lock");
		}
	} else {
		start_server(argc, argv);
	}
}
