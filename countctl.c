#include "head.h"

int main(int argc, char *argv[]){
	
	int i;

	if (argc!=3) {
		printf("bad param\n");
		return 1;
	}

	
	if (fork() == 0) {
		execvp("./server", argv);
		printf("------- err\n");
	}

	printf ("1\n");
	for (i=0; i<atoi(argv[2]); i++){
		if (fork() == 0) {
			execlp("./client", "./client", (char *)NULL);
		}
	}
	wait(NULL);
	printf("good\n");
	//while (wait4(-1, NULL, WHNOHANG, NULL)>0);
	return 0;
}
