/* Jacob Powers - smallsh  -  CS344 Operating systems  -  5 - 26 - 18*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>
#include <stdbool.h>
#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <math.h>

#define MAX_INPUT 2048 
#define MAX_ARGS 512
#define TEST 0. // 1 prints debug info 0 does not

bool foregroundOnlyMode = false;

void status(int);
int cd(char*);
int exitSmlShl(int, int*);
int getInput(char* );
int spawn(char* , char* , char** );
void signalIntHandler(int);
void signalStp(int);
void printBackGroundProcesses(int, int*);
void checkBackgroundProcesses(int, int*);
char* searchAndReplace(char*, int);

int main (int argc, const char * argv[]) {
	char input[MAX_INPUT];
	char* infile;
	char* outfile;
	char* tmpStr;
	int inputRtnVal, argCnt, currentPid;
	char *token;
	char* args[MAX_ARGS];
	bool bgFlag = false;
	int childExitMethod;
	int bgProcs[500];
	int bgProcsCnt = 0;

	struct sigaction SIGINT_action = {{0}}, SIGTSTP_action = {{0}}, ignore_action = {{0}};
	ignore_action.sa_handler = SIG_IGN;
	SIGINT_action.sa_handler = SIG_DFL;
	SIGTSTP_action.sa_handler = signalStp;
	SIGTSTP_action.sa_flags = SA_RESTART;
	sigaction(SIGTSTP, &SIGTSTP_action, NULL);
	sigaction(SIGINT, &ignore_action, NULL);
	do {
		do {
			inputRtnVal = getInput(input);
		} while (inputRtnVal);
		currentPid = getpid();
		tmpStr = searchAndReplace(input, currentPid);
		strcpy(input, tmpStr);
		free(tmpStr);
		if (strlen(input) > 0 && input[strlen(input) - 1] == '\n') input[strlen(input) - 1] = '\0';
//Break input into individual tokens and append to args array
		infile = NULL;
		outfile = NULL;
		token = NULL;
		bgFlag = false;

		argCnt = 0;
		token = strtok(input, " ");
		while ( token != NULL && strcmp(token, "\0")) {
			if (!strcmp(token, "<")) { //Look for stdin token
				infile = strtok(NULL, " ");
			} else if (!strcmp(token, ">")) { //Look for stdout token
				outfile  = strtok(NULL, " ");
			} else if (!strcmp(token, "&")) { //Look for & token
				if (!foregroundOnlyMode) {bgFlag = true;}
				strtok(NULL, " ");//skip & from args
			} else { //else it is an argument
				args[argCnt] = token;
				argCnt++;
			}
			token = strtok(NULL, " ");
		} //end tokenize while
//		if(foregroundOnlyMode && bgFlag){ bgFlag = false;}// Set bgflag to false if foreground only
		args[argCnt] = NULL;
		if (TEST) {
			printf( "command=%s\n<infile: %s\n>outfile: %s\n bgFlag = %d\n", args[0], infile, outfile, bgFlag);
			for (int i = 0; i < argCnt; i++)
				printf( "args[%d] =%s\n", i, args[i]);
			fflush(stdout);
		}//if TEST

		/*
		*	Check for different cases in command: built in vs system
		*/
		if (!strcmp(args[0], "cd")) {
			cd(args[1]);
		} else if (!strcmp(args[0], "status")) {
			status(childExitMethod);
		} else if (!strcmp(args[0], "exit")) {
			break;
		} else {
			pid_t spawnpid = -5;
			spawnpid = fork();
			switch (spawnpid) {
			case -1: //Error
				perror("Could not fork process.");
				break;
			case 0: //Child process regardless of bgFlag run spawn
				sigaction(SIGINT, &SIGINT_action, NULL);
				sigaction(SIGTSTP, &ignore_action, NULL);
				spawn(infile, outfile, args);
			default:
				if (bgFlag) { // Parent Dont wait and print spawn pid
					printf("Background pid is %d\n", spawnpid);
					fflush(stdout);
					bgProcs[bgProcsCnt] = spawnpid;
					bgProcsCnt++;
				} else { //Else foreground and args are > 1
					waitpid(spawnpid, &childExitMethod, 0);
					if (!WIFEXITED(childExitMethod)) status(childExitMethod); // If child didn't exit normally print status
				}
			}// end spawnpid switch
		}//end else spawn pid
		checkBackgroundProcesses(bgProcsCnt, bgProcs);
	} while (strcmp(input, "exit"));
	exitSmlShl(bgProcsCnt,bgProcs);
	return 0;
}//End main

/* Function: checkBackGroundProcesses
* --------------------
* Checks to see if any background processes have completed and prints their status
*
* n: number of processes and pointer to array containing PIDs
*
* Returns: void
*/

void checkBackgroundProcesses(int bgProcsCnt, int *bgProcs) {
	int bgChildExitMethod, childPID;
	int tmpCnt = bgProcsCnt;
	childPID = waitpid(-1, &bgChildExitMethod, WNOHANG);
	while (childPID > 0) {
		printf("Background pid %d is done: ", childPID);
		fflush(stdout);
		status(bgChildExitMethod);
		for (int i = 0; i < tmpCnt; i++) {
			if (bgProcs[i] == childPID) {
				bgProcs[i] = -5;
				tmpCnt--;
			}
		}
		childPID = waitpid(-1, &bgChildExitMethod, WNOHANG);
	}
	bgProcsCnt = tmpCnt;
	return;
}

/* Function: printBackGroundProcesses
* --------------------
* Prints array of currently running background processes
*
* n: number of processes and pointer to array containing PIDs
*
* Returns: void
*/
void printBackGroundProcesses(int count, int *bgProcs) {
	for (int i = 0; i < count; i++)
		printf("BG proc %d: %d\t" , i, bgProcs[i]);
	printf("\n");
	fflush(stdout);
}
/* Function: spawn
* --------------------
* Sets up new child instance of smallsh with arguments given as parameters
*
* n: Input file name, output filename, pointer to array of arguments
*
* Returns: 0 on success
*/
int spawn(char* infile, char* outfile, char** args) {
	int input, output, result;

	if (infile != NULL) {
		input = open(infile, O_RDONLY);
		if (input == -1) { perror("infile open()"); exit(1); }
		result = dup2(input, 0);
		if (result == -1) { perror("infile dup2"); exit(2);}
	}
	if (outfile != NULL) {
		output = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if (output == -1) { perror("outfile open()"); exit(1); }
		result = dup2(output, 1);
		if (result == -1) { perror("outfile dup2"); exit(2);}
	} else {
		output = open("/dev/null", O_WRONLY | O_CREAT | O_TRUNC, 0644);
	}
	if (execvp(*args, args) < 0) {
		perror("Exec failure!\n");
		exit(1);
	}
	return 0;
}

/* Function: getInput
* --------------------
* Gets command input from user \n (enter with not string) will simply print new line
* # will print a comment any commands after the # will not execute
*
* n: string to return command sting
*
* Returns: -1 if input sting is \n or # a comment 0 otherwise any
*/
int getInput(char *rtn) {
	printf(": ");
	fflush(stdout);
	rtn[0] = '\0';
	fgets(rtn, MAX_INPUT, stdin);
	if (rtn[0] == '\n' || rtn[0] == '#')
		return -1;
	else
		return 0;
}

/* Function: cd
* --------------------
* Changes the working directory of your shell. With no arguments it changes to the directory specified in the HOME environment
* variable (not to the location where smallsh was executed from, unless your shell executable is located in the HOME directory, in which case these are the same).
* This command can also take one argument: the path of a directory to change to. Your cd command should support both absolute and relative paths.
* When smallsh terminates, the original shell it was launched from will still be in its original working directory, despite your use of chdir() in smallsh.
* Your shell's working directory begins in whatever directory your shell's executable was launched from.
*
* n: Path of directory to change to
*/
int cd(char* arg1) {
	if (arg1 == NULL)
		chdir(getenv("HOME"));
	else
		chdir(arg1);

	if (TEST) {
		char cwd[1024];
		printf("cd arg = %s\n", arg1);
		getcwd(cwd, sizeof(cwd));
		printf("cd PWD = %s\n", cwd);
		fflush(stdout);
	}
	return 1;
}
/* Function: exitSmlShl
* --------------------
* Performs clean up prior to closing shell and exiting program
* n: count of back ground processes and pointer to background processes PID array.
*
* return 0 on success 1 on empty array
*/
int exitSmlShl(int count, int *bgProcs) {

	if (count > 0) {
		for (int i = 0; i < count; i++) {
			if (bgProcs[i] > 0) {
				kill(bgProcs[i], SIGTERM);
				bgProcs[i] = -5;
			}
		}
	} else {
		return 1;
	}
	return 0;
}

/* Function: status
* --------------------
* prints out either the exit status or the terminating signal of the last foreground
* process ran by your shell.
*
* n: integer of last exited process
*
*  returns: void
*/

void status(int childExitMethod) {
	if (WIFEXITED(childExitMethod) != 0) {
		printf("exit value %d\n", WEXITSTATUS(childExitMethod));
		fflush(stdout);
	}
	if (WIFSIGNALED(childExitMethod) != 0 ) {
		printf("terminated by signal %d\n", WTERMSIG(childExitMethod));
		fflush(stdout);
	}
	return;
}

/*
* Function:  signalStp
* --------------------
* Signal handler function for SIGSTP command
*
**  n: signal number passed by SIGSTP
*
*  returns: void
*/

void signalStp(int signo) {
	if (!foregroundOnlyMode) {
		foregroundOnlyMode = true;
		char* enterMessage = "Entering foreground-only mode (& is now ignored)\n";
		write(1, enterMessage, strlen(enterMessage));
		fflush(stdout);
	} else {
		foregroundOnlyMode = false;
		char* exitMessage = "Exiting foreground-only mode\n";
		write(1, exitMessage, strlen(exitMessage));
		fflush(stdout);
	}
}

/*
* Function:searchAndReplace
* --------------------
* Find all instances of $$ in given string and replaces with PID passed as arguments
*
* Suggested by https://osu-cs.slack.com/archives/C06K9HNRH/p1527025309000356
* https://stackoverflow.com/questions/8257714/how-to-convert-an-int-to-string-in-c
* https://stackoverflow.com/questions/779875/what-is-the-function-to-replace-string-in-c
* All of the above sites contributed to this great solution.
*
* *  n: Original string pointer and program ID
*
*  returns: Modified string with pid in place of $$ in original string
*/

char* searchAndReplace(char* input, int pid) {
	char* expandedInput;
	char* insertLocation;
	char* tmp;
	int len_front;
	int pidLen = (int)((ceil(log10(pid)) + 1) * sizeof(char)); //get length of pid
	char strPid[pidLen];
	sprintf(strPid, "%d", pid); // Convert to string
	int count = 0;
	for (int i = 0; i < strlen(input) - 1; i++) {
		if (input[i] == '$' && input[i + 1] == '$')
			count++;
	} // Count number of instances of $$
	tmp = expandedInput = malloc(strlen(input) + ( pidLen - 2) * count + 1);
	if (!expandedInput)
		return NULL;
	while (count--) {
		insertLocation = strstr(input, "$$");//find occurrences of $$
		len_front = insertLocation - input;
		tmp = strncpy(tmp, input, len_front) + len_front;
		tmp = strcpy(tmp, strPid) + pidLen;
		input += len_front + 2;
	}
	strcpy(tmp, input);
	return expandedInput;
}
