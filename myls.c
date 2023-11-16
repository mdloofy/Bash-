#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>


int main(){
	
	char *name = "foo";
     
    int rc = fork();
	if (rc == 0){// child
  		//close(1);
  		// int fd = open("output.txt", O_RDWR|O_CREAT);
		// execute a program
		char *myargs[] = {"ls", "/cs4348-xv6/src/testcases/P1/test", 0};
		execv("/bin/ls", myargs);
		printf("EXEC FAILED");
	}
	else { // parent takes this path
	//	printf("PARENT: I am waiting for my child to finish \n");
		int ret = wait(NULL);
	//	printf("PARENT: Child %d finished \n", ret);
	}

    return 0;

}

