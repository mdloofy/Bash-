#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

//Declaring all the funcions:
int runInter();
int runBatch(char *file);
void parseBuf(char *buf, char ***commands, int* numcommands);
int execute(char **commands, int numcommands, char ***paths, int *numpaths);
int exitHelper(char ***paths, int numpaths, char **buf, char* arg);
int cdHelper(char *token, char **buf);
int pathHelper(char ***paths, int *numpaths, char **token, char **buf);
int execvHelper(char **paths, int numpaths, char **token, char **buf);

int main(int argc, char *argv[]){
	char error[30] = "An error has occurred\n";

	if(argc == 1){	//0 args -> interaction mode
		if(runInter() == -1){	//if issue ocurrs, print error
			int err = write(STDERR_FILENO, error, strlen(error));
			if (err == 0){}
		}
	}
	else if (argc == 2){	//1 arg -> batch mode
		if(runBatch(argv[1]) == -1){
			int err = write(STDERR_FILENO, error, strlen(error));
			if (err == 0){}
		}
	}
	else{	//else print error
		int err = write(STDERR_FILENO, error, strlen(error));
		if (err == 0){}
	}
	return 0;
}

//Batch mode:
int runBatch(char* file){
	FILE* fptr;
	fptr = fopen(file, "r");//open file in read mode

	if(fptr == NULL){
		return -1;
	}

	char* buf;
	size_t bufsize = 1000;
	buf = (char*)malloc(bufsize * sizeof(char));

	char error[30] = "An error has occurred\n";

	char** paths;	//set up init paths
	int numpaths = 2;

	paths = (char **)malloc(2 * sizeof(char*));
	paths[0] = (char *)malloc(strlen("/bin/") * sizeof(char) + 1);
	paths[1] = (char *)malloc(strlen("/user/bin/") * sizeof(char) + 1);
	strcpy(paths[0], "/bin/"); 
	strcpy(paths[1], "/user/bin/");

	int i;	//used for 'for' loops
	int read;	//used to make sure get line is getting valid lines
	while((read = getline(&buf, &bufsize, fptr)) != -1){

		char** commands = NULL;	//parse buf into an array of commands
		int numcommands = 0;

		int numparallel = 0;
		for(i = 0; buf[i]; i++){
			if(buf[i] == '&'){
				numparallel++;
			}
		}

		parseBuf(buf, &commands, &numcommands);

		if(numparallel != numcommands - 1 && numparallel > 0){	//number of & characters should be equal to numcommands - 1
			int err = write(STDERR_FILENO, error, strlen(error));
			if (err == 0){}
		}

		else if(numcommands > 0){	//skip blank lines 
			int success = execute(commands, numcommands, &paths, &numpaths); //execute and if error -> print

			if(success == -1){	//-1 == error -> break out of loop and return error code; 1 == exit -> break out of loop gracefully
				int err = write(STDERR_FILENO, error, strlen(error));
				if (err == 0){}
			}
		}

		for(i = 0; i < numcommands; i++){//free all commands
			free(commands[i]);
		}
		free(commands);		
	}
	free(buf);//free buf
	fclose(fptr);//free fptr
	for(i = 0; i < numpaths; i++){//free all paths
		free(paths[i]);
	}
	free(paths);

	if(read == -1){	//if error in getting line -> print error
		int err = write(STDERR_FILENO, error, strlen(error));
		if (err == 0){} 	
	}

	return 0;
}

//Interacticve mode:
int runInter(){
	char* buf;
	size_t bufsize = 100;
	buf = (char *)malloc(bufsize * sizeof(char));
	int success = 0;
	char error[30] = "An error has occurred\n";
	char** paths;
	int numpaths = 2;
	int i;

	paths = (char **)malloc(2 * sizeof(char*));
	paths[0] = (char *)malloc(strlen("/bin/") * sizeof(char) + 1);
	paths[1] = (char *)malloc(strlen("/user/bin/") * sizeof(char) + 1);
	strcpy(paths[0], "/bin/"); 
	strcpy(paths[1], "/user/bin/");
	
	while (1){	//continue interactive mode until exit command is called or error ocurrs
		printf("dash> ");
		if(getline(&buf, &bufsize, stdin) == -1){//get strings from terminal
			int err = write(STDERR_FILENO, error, strlen(error));
			if (err == 0){}
		}

		char** commands = NULL;	//parse buf into an array of commands
		int numcommands = 0;

		int numparallel = 0;
		for(i = 0; buf[i]; i++){ //Counting parallel commands
			if(buf[i] == '&'){
				numparallel++;
			}
		}

		parseBuf(buf, &commands, &numcommands);

		if(numparallel != numcommands - 1){	//number of & characters should be equal to numcommands - 1
			success = -1;
		}

		if(success != -1){
			success = execute(commands, numcommands, &paths, &numpaths); //execute and if error -> print
		}
		
		for(i = 0; i < numcommands; i++){	//free commands array of strings (commands)
			free(commands[i]);
		}
		free(commands);
		
		if(success == -1){	//-1 == error -> break out of loop and return error code; 1 == exit -> break out of loop gracefully
			int err = write(STDERR_FILENO, error, strlen(error));
			if (err == 0){}
			success = 0;
		}
	}
	return success;
}

//Function to parse buf into commands based on "&" deleiminator"
void parseBuf(char *buf, char ***commands, int *numcommands){
	char *buftoken = strtok_r(buf, "&\n", &buf);

	while(buftoken != 0){
		(*numcommands)++;
				
		(*commands) = (char **)realloc((*commands), (*numcommands) * sizeof(char *));	//allocate space for the next entry
		(*commands)[(*numcommands) - 1] = (char *)malloc(strlen(buftoken) * sizeof(char));	//allocate space for the actual command
		
		strcpy((*commands)[(*numcommands) - 1], buftoken);	//cpy command to array

		buftoken = strtok_r(0, "&\n", &buf);	//get next token
	}
}

//Execute function based on the commands received:
int execute(char **commands, int numcommands, char ***paths, int *numpaths){
	
	char *token;
	int success = 0;
	int cpid = 0;
	char error[30] = "An error has occurred\n";
	int saved_stdout = dup(1);
	int saved_err = dup(2);
	int i;

	for(i = 0; i < numcommands; i++){	//for each command in commands, check first word and exec accordingly
		
		if (strstr(commands[i],">")) //See if command has > sign for redirection
		{
			char* copy = (char *)malloc(strlen(commands[i]) * sizeof(char *));	
			strcpy(copy, commands[i]);	//make copy of command
			char *cmd;
			cmd = strtok_r(copy, ">", &copy);	//get actual command first
			char *output;
			output = strtok_r(0,"> \t\n",&copy);	//get file to redirect to

			if(strtok_r(0, " \t", &copy) != 0 || output == 0){	//if there is a token after the redirect file -> error
				return -1;
			}

			int file = open(output, O_WRONLY | O_CREAT | O_TRUNC, 0777);	//open file and redirect stdout and stderror
			dup2(file,STDOUT_FILENO); //redirect standard output
			dup2(file, STDERR_FILENO); //redirect standae error
			close(file);
			if(file == -1)
			{	 
				success = -1;
			}
			else
			{
				commands[i] = cmd;	//put parsed command back into command
			}
		}
		char* buf = (char *)malloc(strlen(commands[i]) * sizeof(char *));
		strcpy(buf, commands[i]);

		token = strtok_r(buf, " \t\n", &buf);	//get command code (first part of command string)

		if(token != NULL){	//go to respective helper function
			if(strcmp(token, "exit") == 0 || strcmp(token, "exit\n") == 0){
				char* arg1 = strtok_r(0, " \t\n", &buf);
				success = exitHelper(paths, *numpaths, &buf, arg1);
			}
			else if(strcmp(token, "cd") == 0 || strcmp(token, "cd\n") == 0){
				success = cdHelper(token, &buf);
			}
			else if(strcmp(token, "path") == 0 || strcmp(token, "path\n") == 0){
				success = pathHelper(paths, numpaths, &token, &buf);
			}
			else{	//see if external function
				cpid = fork();
				if(cpid < 0){
					success = -1;
				}
				else if(cpid == 0){//call helper function  for executing
					success = execvHelper(*paths, *numpaths, &token, &buf);
					if(success == -1){
						int err = write(STDERR_FILENO, error, strlen(error));
						if (err == 0){}
					}
					exit(success);
				}	
				dup2(saved_stdout, 1); //giving back the standar output to terminal
				dup2(saved_err,2); //giving back the standar error to terminal
			}
		}
		//free(buf);

	}

	while(wait(NULL) > 0);	//wait for all children
	return success;

}

//Function to help with exit command
int exitHelper(char ***paths, int numpaths, char **buf, char* arg){
	if(arg == 0 || strcmp(arg, "\n") == 0){
		int i;
		for(i = 0; i < numpaths; i++){
			free((*paths)[i]);
		}
		exit(0);
	}
	return -1;
}

//Function to help with changing directory
int cdHelper(char *token, char **buf){
	int success = 0;
	if(strcmp(token, "cd\n") == 0){	//if no argument -> error
		success = -1;
	}		
	else{
		token = strtok_r(0, " \t\n", &(*buf));	//get argument

		if(strtok_r(0, " \t\n", &(*buf)) != 0){	//if there is a second argument -> error
			success = -1;
		}
		else{
			if(token != NULL){
				success = chdir(token);	//else try to change directory to value of token string
			}
			else{
				success = -1;
			}
		}
	}	
	return success;
}

//Function to help with path command
int pathHelper(char ***paths, int *numpaths, char **token, char **buf){
	int i;
	for (i = 0; i < (*numpaths); i++){		//Free paths array and entries; set numpaths to 0
		free((*paths)[i]);
	}
	free(*paths);
	*paths = NULL;
	*numpaths = 0;

	*token = strtok_r(0, " \t\n", buf);			//iterate through tokens; add to path
	while(*token != 0){
		
		(*numpaths)++;

		*paths = (char **)realloc((*paths), (*numpaths) * sizeof(char *));	//reallocate memory for each new path

		if((*token)[strlen(*token) - 1] != '/'){
			(*paths)[(*numpaths) - 1] = (char *)malloc((strlen(*token) + 1) * sizeof(char));
			strcpy((*paths)[(*numpaths) - 1], *token);		//if the path is missing the last '/' char, insert it at end
			strcat((*paths)[(*numpaths) - 1], "/");
		}
		else{
			(*paths)[(*numpaths) - 1] = (char *)malloc(strlen(*token) * sizeof(char));
			strcpy((*paths)[(*numpaths) - 1], *token);		//finally, copy path to array and get next token
		}		
		*token = strtok_r(0, " \t\n", buf);
	}
	return 0;
}

//Functions to help with external commands execution
int execvHelper(char **paths, int numpaths, char **token, char** buf){
	char** args = NULL;	
	int numargs = 0;
	char* command = (char *)malloc(strlen(*token) * sizeof(char) + 1);
	int success = 0;
	strcpy(command, *token);	//create command argument to send to execv
	if (command[strlen(*token) - 1] == '\n'){
		command[strlen(*token) - 1] = '\0'; 
	}
	
	numargs++;		//first entry into args is filename associated with function
	args = (char **)realloc(args, numargs * sizeof(char *));
	args[numargs - 1] = (char *)malloc(strlen(*token) * sizeof(char));
	strcpy(args[numargs - 1], *token);

	*token = strtok_r(0, " \t\n", buf);	//next is first argument

	while(*token != 0){	//get args string into array of args
		numargs++;
		args = (char **)realloc(args, numargs * sizeof(char *));
		args[numargs - 1] = (char *)malloc(strlen(*token) * sizeof(char));
				
		strcpy(args[numargs - 1], *token);
		*token = strtok_r(0, " \t\n", buf);
	}

	int path = 0;
	int accessed = 1;	//1 == not accessed; 0 == accessed; -1 == accessed error
	char* cpath = NULL;

	while(path < numpaths && (accessed == 1 || accessed == -1)){	//see if executable file accessable from paths
		cpath = (char *)realloc(cpath, (strlen(paths[path]) + strlen(command)) * sizeof(char));
		strcpy(cpath, paths[path]);
		strcat(cpath, command);
		accessed = access(cpath, X_OK); 
		path++;
	}
	if(accessed == 0){		//if accessable: execute
		int executed = 0;
		executed = execv(cpath, args);
		if(executed != 0){
			success = -1;
		}
	}
	else{
		success = -1;
	}
	free(command);	//free command
	free(cpath);	//free cpath (used to get path to command)
	int i;
	for(i = 0; i < numargs; i++){	//free args array (array of argv sent to execv)
		free(args[i]);
	}
	free(args);	//free actual array itself
	return success;
}
