#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>

// Directions here:
// https://docs.google.com/document/d/1LBMJslvYvw59uZ_8DNiiPzsp0heW3qesaalOo31IGYg/edit

// TODO: Refactor into several .c and .h files, separating read, parse, and
//       execute would be a good place to start

/**
 * JobNode_t struct
 */ 
typedef struct JobNode_t{
	char* jobStr;
  int jobId;
  int pgid;
  int status; // or enum

  struct JobNode_t* next;
}JobNode_t;

JobNode_t** jobStack = NULL;
int pgrp = -1;
char* fgProc;

/**
 * Purpose:
 *   Count number of nodes in stack
 * 
 * Args:
 *   head (JobNode_t**): Pointer to job stack head pointer
 * 
 * Returns:
 *   (int): Number of nodes in stack
 */
int countNodes(JobNode_t** head){
  JobNode_t* curr = *head;
  int count = 0;

  while(curr != NULL){
    count++;
  }

  return count;
}

/**
 * Purpose:
 *   Check if job is already in job stack
 * 
 * Args:
 *   head (JobNode_t**): Pointer to job stack head pointer
 *   id           (int): Process group ID to look for
 * 
 * Returns:
 *   (int): 1 if job is already in stack
 */ 
int findID(JobNode_t** head, int id){
  const int FOUND = 1;
  const int NOT_FOUND = 0;

  JobNode_t* curr = *head;
  
  while(curr != NULL){
    if(curr->pgid == id){
      return FOUND;
    }
    curr = curr->next;
  }

  return NOT_FOUND;
}

/**
 * Purpose:
 *   Find most recent job that is stopped or in background
 * 
 * Args:
 *   head (JobNode_t**): Pointer to job stack head pointer
 *   id           (int): Process group ID to look for
 * 
 * Returns:
 *   (int): PGID of most recent stopped job
 */ 
int findRecent(JobNode_t** head){
  const int STOPPED_VAL = 1;
  const int NOT_FOUND = -1;

  JobNode_t* curr = *head;
  
  while(curr != NULL){
    if(curr->status == STOPPED_VAL){
      return curr->pgid;
    }
    curr = curr->next;
  }

  return NOT_FOUND;
}

/**
 * Purpose:
 *   Free job stack node memory
 * 
 * Args:
 *   head (JobNode_t**): Pointer to job stack head pointer
 * 
 * Returns:
 *   None
 */ 
// int freeJobStack(JobNode_t** head){
//   return;
// }

/**
 * Purpose:
 *   Push JobNode to background stack
 * 
 * Args:
 *   head  (JobNode_t**): Pointer to job stack head pointer
 *   jobStr      (char*): Job string
 *   pgid          (int): Process group id of job
 *   status        (int): Running state of job
 * 
 * Returns:
 *   None
 */ 
void pushNode(JobNode_t** head, char* jobStr, int pgid, int status){
  JobNode_t* curr = (JobNode_t*)malloc(sizeof(JobNode_t));

  curr->jobStr = (char*)malloc(2001 * sizeof(char));
  strcpy(curr->jobStr, jobStr);
  curr->pgid = pgid;
  if(*head == NULL){
    curr->jobId = 1;
  }
  else{
    curr->jobId = (*head)->jobId + 1;
  }  
  curr->status = status;
  curr->next = *head;
  *head = curr;
  return;
}

/**
 * Purpose:
 *   Pop JobNode off background stack
 * 
 * Args:
 *   head (JobNode_t**): Pointer to job stack head pointer
 *   
 * Returns:
 *   (int): Process group ID, -1 if failed
 */
int popNode(JobNode_t** head){
  const int INVALID = -1;
  JobNode_t* curr;
  int popped;

  if(*head == NULL){
    return INVALID;
  }

  curr = (*head)->next;
  popped = (*head)->pgid;
  free((*head)->jobStr);
  free(*head);
  *head = curr;

  return popped;
}

/**
 * Purpose:
 *   Change state of completed jobs in job stack
 * 
 * Args:
 *   head (JobNode_t**): Pointer to job stack head pointer
 * 
 * Returns:
 *   None
 */
void checkDoneJobs(JobNode_t** head){
  const int DONE_VAL = 2;
  int jobPGID;
  JobNode_t* curr = *head;

  while(curr != NULL){
    jobPGID = waitpid(curr->pgid, NULL, WNOHANG);
    if(jobPGID == curr->pgid){
      curr->status = DONE_VAL;
    }
    curr = curr->next;
  }
  return;
}

/**
 * Purpose:
 *   Print completed jobs
 * 
 * Args:
 *   head (JobNode_t**): Pointer to job stack head pointer
 * 
 * Returns:
 *   None
 */
void printDoneJobs(JobNode_t** head){
  const int DONE_VAL = 2;
  const char CURRENT = '+';
  const char BACK = '-';
  const char* DONE_TXT = "Done";

  char currentJob;
  int currentJobID;
  
  JobNode_t* curr = *head;
  if(curr != NULL){
    currentJobID = curr->jobId;
  }

  while(curr != NULL){
    if(curr->jobId == currentJobID){
      currentJob = CURRENT;
    }
    else{
      currentJob = BACK;
    }

    if(curr->status == DONE_VAL){
      printf("[%d]%c  %s            %s \n", curr->jobId, currentJob, DONE_TXT,
             curr->jobStr);
    }

    curr = curr->next;
  }
  return;
}

/**
 * Purpose:
 *   Print stack of background jobs
 * 
 * Args:
 *   head (JobNode_t**): Pointer to job stack head pointer
 * 
 * Returns:
 *   None
 */
void printStack(JobNode_t** head){
  const int RUN_VAL = 0;
  const int STOPPED_VAL = 1;
  const int DONE_VAL = 2;
  const char CURRENT = '+';
  const char BACK = '-';
  const char* RUN_TXT = "Running";
  const char* STOP_TXT = "Stopped";
  const char* DONE_TXT = "Done";

  char currentJob;
  int currentID;
  
  JobNode_t* curr = *head;
  if(curr != NULL){
    currentID = curr->jobId;
  }

  while(curr != NULL){
    if(curr->jobId == currentID){
      currentJob = CURRENT;
    }
    else{
      currentJob = BACK;
    }

    if(curr->status == RUN_VAL){
      printf("[%d]%c  %s         %s \n", curr->jobId, currentJob, RUN_TXT,
           curr->jobStr);
    }
    else if(curr->status == STOPPED_VAL){
      printf("[%d]%c  %s         %s \n", curr->jobId, currentJob, STOP_TXT,
           curr->jobStr);
    }
    else if(curr->status == DONE_VAL){
      printf("[%d]%c  %s            %s \n", curr->jobId, currentJob, DONE_TXT,
           curr->jobStr);
    }
    curr = curr->next;
  }
  return;
}

/**
 * Purpose:
 *   Change running status of job in job stack
 * 
 * Args:
 *   head (JobNode_t**): Pointer to job stack head pointer
 *   pgid         (int): Process group ID
 *   newStatus    (int): New running status of process
 * 
 * Returns:
 *   None
 */ 
void changeJobStatus(JobNode_t** head, int pgid, int newStatus){
  JobNode_t* temp = *head;
  int changed = 0;

  while((!changed)&&(temp != NULL)){
    if(temp->pgid == pgid){
      changed = 1;
      temp->status = newStatus;
    }
    temp = temp->next;
  }

  return;
}

/**
 * Purpose:
 *   Change running status of job in job stack
 * 
 * Args:
 *   head (JobNode_t**): Pointer to job stack head pointer
 * 
 * Returns:
 *   None
 */ 
void removeDoneJobs(JobNode_t** head){
  const int DONE = 2;

  JobNode_t* curr = *head;
  JobNode_t* temp = curr->next;

  while((curr != NULL) && (curr->status == DONE)){
    free(curr);

    curr = temp;
    if(temp != NULL){
      temp = temp->next;
    }
  }
  if(curr == NULL){
    *head = NULL;

    return;
  }
  else{
    *head = curr;
    temp = curr->next;

    while(temp != NULL){
      if(temp->status == DONE){
        curr->next = temp->next;

        free(temp);
        temp = curr->next;
      }
      else{
        curr = temp;
        temp = temp->next;
    
      }
    }

    return;
  }
}

/**
 * Purpose:
 *   Handler for SIGKILL signal
 * 
 * Args:
 *   sigNum (int): Signal number
 * 
 * Returns:
 *   None
 */
static void sigintHandler(int sigNum){
  const char* PROMPT = "# ";
  printf("\nSIGINT\n");
  printf("PGID: %d\n", getpid());
  printf("pgrp: %d\n", pgrp);
	if(pgrp != -1){
    killpg(pgrp, SIGINT);
	}
  else{
    printf("%s", PROMPT);
		return;
  }
  // reset handler
  signal(SIGINT, sigintHandler);
  return;
}

/**
 * Purpose:
 *   Handler for SIGTSTP signal
 * 
 * Args:
 *   sigNum (int): Signal number
 * 
 * Returns:
 *   None
 */
static void sigtstpHandler(int sigNum){
  const int STOPPED = 1;
  const char CURRENT = '+';
  const char* PROMPT = "# ";
  const char* STOP_STR = "Stopped";
  
  printf("\nSIGTSTP\n");
  printf("PGID: %d\n", getpid());
  printf("pgrp: %d\n", pgrp);
	if((pgrp != -1) && !findID(jobStack, pgrp)){
    killpg(pgrp, SIGTSTP);
    pushNode(jobStack, fgProc, pgrp, STOPPED);
    printf("[%d]%c  %s         %s \n", (*jobStack)->jobId, CURRENT, STOP_STR,
           (*jobStack)->jobStr);
	}
  else{
		printf("%s", PROMPT);
  }
  // reset handler
  signal(SIGTSTP, sigtstpHandler);
  
  return;
}

/**
 * Purpose:
 *   Handler for SIGCHLD signals
 * 
 * Args:
 * 
 * Returns:
 *   None
 */
static void sigchldHandler(int sigNum){
  // printf("\nSIGCHLD EXECUTED\n");

  signal(SIGCHLD, sigchldHandler);
}

/** 
 * Purpose:
 *   Verify that tokens in input line do not exceed maximum token length
 * 
 * Args:
 *   input      (char*): Input C-string
 *   maxLineLen   (int): Maximum length of input
 *   maxTokenLen  (int): Maximum length of tokens
 * 
 * Returns:
 *   (int): Returns 1 if all tokens are within maximum limit
 */
int checkTokens(char* input, int maxLineLen, int maxTokenLen){
  const int VALID = 1;
  const int INVALID = 0;
  const char SPACE_CHAR = ' ';
  int tokenLen = 0;
  int k = 0;
  while(k < strlen(input)){
    if(tokenLen >= maxTokenLen){
      return INVALID;
    }
    else if((tokenLen < maxTokenLen) && (input[k] == SPACE_CHAR)){
      tokenLen = 0;
    }
    else{
      tokenLen++;
    }
    k++;
  }
  return VALID;
}

/**
 * Purpose:
 *   Read in a line of input from stdin and verify that input line length and
 *   token length is valid
 * 
 * Args:
 *   input      (char*): Input C-string
 * 
 * Returns:
 *   (int): Returns 1 if string is valid, 0 if string is invalid
 */
int checkInput(char* input){
  // TODO: Add function to ensure file redir goes to a file e.g.:# cat hello.txt >
  // TODO: Add function to ensure pipe goes to a valid command e.g.:# ls |
  // TODO: Add input verification to ensure that bg, fg, & are at expected indices
  const int MAX_LINE_LEN = 2001;
  const int MAX_TOKEN_LEN = 31;
  const int INVALID = 0;
  const int VALID = 1;
  if(strlen(input) > MAX_LINE_LEN){
    return INVALID;
  }
  else if(strlen(input) == 0){
    return INVALID;
  }
  else if(!checkTokens(input, MAX_LINE_LEN, MAX_TOKEN_LEN)){
    return INVALID;
  }
  return VALID;
}

/**
 * Purpose:
 *   Create array of token C-strings from input line split on input delimiter
 * 
 * Args:
 *   input (char*): Pointer to input c-string
 *   delim   (int): Delimiter to split on
 *  
 * Returns:
 *   (char**): Returns array of token c-strings
 * 
 */
char** splitStrArray(char* input, const char* delim){
  const int MAX_LINE_LEN = 2001;
  const int MAX_TOK_LEN = 31;
  char* copy = (char*)malloc(MAX_LINE_LEN * sizeof(char));
  strcpy(copy, input);
  
  char** splitted = NULL;
  int numElements = 0;

  char* token = strtok(copy, delim);
  char* entry = (char*)malloc(MAX_TOK_LEN * sizeof(char));
  strcpy(entry, token);

  while(token != NULL){
    numElements++;
    splitted = realloc(splitted, numElements * sizeof(char*));
    splitted[numElements - 1] = entry;

    token = strtok(NULL, delim);
    if(token != NULL){
      entry = (char*)malloc(MAX_TOK_LEN * sizeof(char));
      strcpy(entry, token);
    }
  }

  // Assign NULL to last index
  numElements++;
  splitted = realloc(splitted, numElements * sizeof(char*));
  splitted[numElements - 1] = 0;

  free(copy);
  return splitted;
}

/**
 * Purpose:
 *   Find indices for input, output, and error redirection in a command and
 *   replace tokens with NULL
 * 
 * Args:
 *   cmd     (char**): Array of tokens from command
 *   inIndex   (int*): Index of input redirection token 
 *   outIndex  (int*): Index of output redirection token 
 *   errIndex  (int*): Index of error redirection token 
 *   
 * Returns:
 *   None  
 */ 
void changeRedirToks(char** cmd, int* inIndex, int* outIndex, int* errIndex){
  const char* IN_REDIR = "<";
  const char* OUT_REDIR = ">";
  const char* ERR_REDIR = "2>";

  int index = 0;
  while(cmd[index] != NULL){
    if(!strcmp(cmd[index], IN_REDIR)){
      *inIndex = index + 1;
      // Assign NULL to stop exec() from reading file redirection as part of cmd
      cmd[index] = NULL;
    }
    else if(!strcmp(cmd[index], OUT_REDIR)){
      *outIndex = index + 1;
      cmd[index] = NULL;
    }
    else if(!strcmp(cmd[index], ERR_REDIR)){
      *errIndex = index + 1;
      cmd[index] = NULL;
    }
    index++;
  }

  return;
}

/**
 * Purpose:
 *   Handle file redirect statements
 * 
 * Args:
 *   cmd (char**): Command token array
 *   
 * Returns:
 *   None
 */
void redirectFile(char** cmd){
  const int INVALID = -1;
  int inIndex = -1;
  int outIndex = -1;
  int errIndex = -1;
  int fdIn;
  int fdOut;
  int fdErr;

  changeRedirToks(cmd, &inIndex, &outIndex, &errIndex);

  if(inIndex != INVALID){
    if((fdIn = open(cmd[inIndex], O_RDONLY, 0)) == INVALID){
        perror(cmd[inIndex]);
        exit(EXIT_FAILURE);
    }
    dup2(fdIn, STDIN_FILENO);
    close(fdIn);
  }
  if(outIndex != INVALID){
    if((fdOut = open(cmd[outIndex], O_CREAT | O_WRONLY | O_TRUNC,
        S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == INVALID){ 
      perror(cmd[outIndex]);
      exit(EXIT_FAILURE);
    }
    dup2(fdOut, STDOUT_FILENO);
    close(fdOut);
  }
  if(errIndex != INVALID){
    if((fdErr = open(cmd[errIndex], O_CREAT | O_WRONLY | O_TRUNC,
        S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == INVALID){ 
      perror(cmd[errIndex]);
      exit(EXIT_FAILURE);
    }
    dup2(fdErr, STDERR_FILENO);
    close(fdErr);
  }

  return;
}

/**
 * Purpose:
 *   Execute input line with file redirections
 * 
 * Args: 
 *   cmd   (char**): Token array from command input
 *   input  (char*): Command input C-string
 *   head  (char**): Pointer to job stack head pointer
 *   back     (int): Boolean var indicating background status
 * 
 * Returns:
 *   None
 */
void executeGeneral(char** cmd, char* input, JobNode_t** head, int back){
  // TODO: (WIP) Implement job control
  // TODO: Investigate WNOHANG waitpid flag
  const int MAX_LINE_LEN = 2001;
  const int RUNNING = 0;
  const char NEW_LINE = '\n';

  int status;
  int pidCh1 = fork();

  if(pidCh1 < 0){
    // fork failed; exit
    fprintf(stderr, "Fork failed\n");
    exit(EXIT_FAILURE);
  }
  else if(pidCh1 == 0){
    // child (new process)
    if(signal(SIGINT, sigintHandler) == SIG_ERR){
	    printf("signal(SIGINT) error");
    }
    if(signal(SIGTSTP, sigtstpHandler) == SIG_ERR){
    	printf("signal(SIGTSTP) error");
    }

    // TODO: Check if tcsetpgrp is needed
    // if(back) {
    //   // tcsetpgrp(0, getpid());
    // }

    setpgid(0,0);
    redirectFile(cmd);
    execvp(cmd[0], cmd);

    fprintf(stderr, "Exec failed\n");
    printf("%c", NEW_LINE);
    exit(EXIT_FAILURE);
  }

  // parent process
  pgrp = pidCh1;
  free(fgProc);
  fgProc = (char*)malloc(MAX_LINE_LEN * sizeof(char));
  strcpy(fgProc, input);
  if(!back){
    // If not background proc, wait to execution completion
    waitpid(pidCh1, &status, WCONTINUED | WUNTRACED);
  }
  else{
    // Return to prompt for background process
    fprintf(stderr, "Starting background process\n");

    // Add background job to stack
    pushNode(head, input, pidCh1, RUNNING);

    // TODO: Check if txsetpgrp is needed
    // tcsetpgrp(0, pidCh1);
    // waitID = waitpid(-1, &status, WNOHANG);
    // printf("waitID: %d\n", waitID);
    // tcsetpgrp(0, getpid());
  }
}

/**
 * Purpose:
 *   Execute input line with file redirections and pipes
 * 
 * Args: 
 *   cmd1 (char**): Token array for command before pipe
 *   cmd2 (char**): Token array for command after pipe
 *   input (char*): Command input C-string
 *   head (char**): Pointer to job stack head pointer
 *   back    (int): Boolean var indicating background status
 * 
 * Returns:
 *   None
 */
void executePipe(char** cmd1, char** cmd2, char* input, JobNode_t** head, int back){
  const int MAX_LINE_LEN = 2001;
  const int RUNNING = 0;
  const char NEW_LINE = '\n';

  int pidCh1;
  int pidCh2;
  int status1;
  int status2;
  int pfd[2];

  pipe(pfd);
  pidCh1 = fork();
  if(pidCh1 < 0) {
    // fork failed; exit
    fprintf(stderr, "Fork failed\n");
    exit(EXIT_FAILURE);
  }
  else if(pidCh1 == 0){
    // child 1 (new process)
    if(signal(SIGINT, sigintHandler) == SIG_ERR){
	    printf("signal(SIGINT) error");
    }
    if(signal(SIGTSTP, sigtstpHandler) == SIG_ERR){
    	printf("signal(SIGTSTP) error");
    } 

    setpgid(0,0);
    int pgid = getpid(0);
    printf("Child 1 PGID: %d\n", pgid);

    dup2(pfd[1], 1);
    close(pfd[0]);
    redirectFile(cmd1);
    execvp(cmd1[0], cmd1);

    fprintf(stderr, "Exec failed\n");
    printf("%c", NEW_LINE);
    exit(EXIT_FAILURE);
  }

  pgrp = pidCh1;
  free(fgProc);
  fgProc = (char*)malloc(MAX_LINE_LEN * sizeof(char));
  strcpy(fgProc, input);
  pidCh2 = fork();
  if(pidCh2 < 0){
    // fork failed; exit
    fprintf(stderr, "Fork failed\n");
    exit(EXIT_FAILURE);
  }
  else if(pidCh2 == 0){
    // child 2 (new process)
    // TODO: Find out if these handlers are needed
    if(signal(SIGINT, sigintHandler) == SIG_ERR){
	    printf("signal(SIGINT) error");
    }
    if(signal(SIGTSTP, sigtstpHandler) == SIG_ERR){
    	printf("signal(SIGTSTP) error");
    } 
    
    setpgid(0, pidCh1);
    int one = getpid(0);
    printf("Child 2 pgid: %d", one);
    dup2(pfd[0], 0);
    close(pfd[1]);
    redirectFile(cmd2);
    execvp(cmd2[0], cmd2);

    fprintf(stderr, "Exec failed\n");
    printf("%c", NEW_LINE);
    exit(EXIT_FAILURE);
  }
  // parent process
  close(pfd[0]);
  close(pfd[1]);
  
  if(!back){
    // If not background proc, wait to execution completion
    waitpid(pidCh1, &status1, WCONTINUED | WUNTRACED);
    waitpid(pidCh2, &status2, WCONTINUED | WUNTRACED);
  }
  else{
    // Return to prompt for background process
    fprintf(stderr, "Starting background process\n");

    // Add background job to stack
    pushNode(head, input, pgrp, RUNNING);

    // TODO: Check if txsetpgrp is needed
    // tcsetpgrp(0, pidCh1);
    // waitID = waitpid(-1, &status, WNOHANG);
    // printf("waitID: %d\n", waitID);
    // tcsetpgrp(0, getpid());
  }
}

/**
 * Purpose:
 *   Send SIGCONT to most recent job in job stack and run in foreground
 * 
 * Args:
 *   head (JobNode_t**): Pointer to stack head pointer
 * 
 * Returns:
 *   None
 */ 
void runForeground(JobNode_t** head){
  int recent = findRecent(head);
  kill(recent, SIGCONT);
	// // printf("IN fg");
	// // printf("IN kjob");
	// int count = 0, i;
	// for ( i = 0; args[i] != NULL; ++i)
	// 	count = count + 1;
	// // printf("%d\n", count );

	// if(count >=3)
	// 		fprintf(stderr,RED "fg: Too many arguments\nUsage: fg <jobNumber>\n" RESET);
	// else if(count <= 1)
	// 		fprintf(stderr,RED "fg: Too few arguments\nUsage: fg <jobNumber>\n" RESET);
	// else
	// {

	// 	int inC = atoi(args[1]);
	// 	qjob* job_node = getjob(head, inC);
	// 	if(job_node !=NULL)
	// 	{
	// 		int pid = job_node -> pid;
	// 		int flag = 1;
	// 		kill(pid, SIGCONT);
	// 		// kill(pid, SIGTTIN);
	// 		// kill(pid, SIGTTOU);
	// 		childPID = pid;	
	// 		strcpy(nowProcess, job_node->name);
	// 		removeLL(head, pid);
	// 		// wait(NULL);
	// 		waitpid(-1,NULL,WUNTRACED);
	// 	}
	// 	else
	// 	{
	// 		fprintf(stderr,RED "fg: No such pid exists\n" RESET);
	// 	}
	// }

	return;
}

/**
 * Purpose:
 *   Send SIGCONT to most recent job in job stack and run in foreground
 * 
 * Args:
 *   head (JobNode_t**): Pointer to stack head pointer
 * 
 * Returns:
 *   None
 */ 
void run_bg(JobNode_t** head){
	// printf("IN bg");
	// printf("IN kjob");
	int count = 0, i;
	for ( i = 0; args[i] != NULL; ++i)
		count = count + 1;
	// printf("%d\n", count );

	if(count >=3)
			fprintf(stderr,RED "bg: Too many arguments\nUsage: bg <jobNumber>\n" RESET);
	else if(count <= 1)
			fprintf(stderr,RED "bg: Too few arguments\nUsage: bg <jobNumber>\n" RESET);
	else
	{

		int inC = atoi(args[1]);
		qjob* job_node = getjob(head, inC);
		if(job_node !=NULL)
		{
			int pid = job_node -> pid;
			int flag = 1;
			kill(pid, SIGTTIN);
			kill(pid, SIGCONT);
			changestatLL(head,pid,1);
			// signal(SIGINT,SIG_IGN);
		}
		else
		{
			fprintf(stderr,RED "bg: No such pid exists\n" RESET);
		}

	}

	return;
}

/**
 * Purpose:
 *   Manages jobs based on user input
 *     * bg, fg should be at input[0]
 *     * & should be at input[numTokens - 1]
 * 
 * Args:
 *   cmd       (char**): Array of tokens from command input
 *   input      (char*): Input C-string
 *   head (JobNode_t**): Pointer to stack head pointer
 * 
 * Returns:
 *   None
 */
void manageJobs(char** cmd, char* input, JobNode_t** head){
  const char* BACKGROUND = "&";
  const char* BG_TOK = "bg";
  const char* FG_TOK = "fg";
  const char* JOBS_TOK = "jobs";
  int backState = 0;

  int lastIndex = 0;
  while(cmd[lastIndex] != NULL){
    lastIndex++;
  }
  lastIndex--;

  if(strcmp(cmd[0], JOBS_TOK) == 0){
    // print job stack
    if((*head) != NULL){
      checkDoneJobs(head);
      printStack(head);
      removeDoneJobs(head);
    }
    return;
  } 
  else if(!strcmp(cmd[0], BG_TOK)){
    // execute bg
    printf("RUNNING BG\n");
    return;
  }
  else if(!strcmp(cmd[0], FG_TOK)){
    // execute fg
    printf("RUNNING FG\n");
    return;
  }
  else if(!strcmp(cmd[lastIndex], BACKGROUND)){
    // execute in background
    cmd[lastIndex] = NULL;
    backState = 1;

    // print done jobs
    if((*head) != NULL){
      checkDoneJobs(head);
      printDoneJobs(head);
      removeDoneJobs(head);
    }
    executeGeneral(cmd, input, head, backState);
    return;
  }
  else{
    // execute normally
    backState = 0;
    executeGeneral(cmd, input, head, backState);
    return;
  }
}

/**
 * Purpose:
 *   Manages pipe jobs based on user input
 *     * bg, fg should be at input[0]
 *     * & should be at input[numTokens - 1]
 * 
 * Args:
 *   cmd1      (char**): Token array from first command
 *   cmd2      (char**): Token array from second command
 *   input      (char*): Input C-string
 *   head (JobNode_t**): Pointer to stack head pointer
 * 
 * Returns:
 *   None
 */
void managePipeJobs(char** cmd1, char** cmd2, char* input, JobNode_t** head){
  const char* BACKGROUND = "&";
  const char* BG_TOK = "bg";
  const char* FG_TOK = "fg";
  const char* JOBS_TOK = "jobs";
  int backState = 0;

  int lastIndex = 0;
  while(cmd2[lastIndex] != NULL){
    lastIndex++;
  }
  lastIndex--;

  if(strcmp(cmd1[0], JOBS_TOK) == 0){
    // print job stack
    if((*head) != NULL){
      checkDoneJobs(head);
      printStack(head);
      removeDoneJobs(head);
    }
    return;
  } 
  else if(!strcmp(cmd1[0], BG_TOK) || !strcmp(cmd2[0], BG_TOK)){
    // execute bg
    printf("RUNNING BG\n");
    return;
  }
  else if(!strcmp(cmd1[0], FG_TOK) || !strcmp(cmd2[0], FG_TOK)){
    // execute fg
    printf("RUNNING FG\n");
    return;
  }
  else if(!strcmp(cmd2[lastIndex], BACKGROUND)){
    // execute in background
    cmd2[lastIndex] = NULL;
    backState = 1;

    // print done jobs
    if((*head) != NULL){
      checkDoneJobs(head);
      printDoneJobs(head);
      removeDoneJobs(head);
    }
    executePipe(cmd1, cmd2, input, head, backState);
    return;
  }
  else{
    // execute normally
    backState = 0;
    executePipe(cmd1, cmd2, input, head, backState);
    return;
  }
}

/**
 * Purpose:
 *   Loops yash shell until user terminates program (CTRL+D)
 * 
 * Args:
 *   None
 * 
 * Returns:
 *   None
 */
void shell(void){
  const int MAX_LINE_LEN = 2001;
  const char NEW_LINE = '\n';
  const char* PIPE = "|";
  const char* SPACE_CHAR = " ";
  const char* PROMPT = "# ";

  int validInput = 0;
  int index = -1;
  char* input;
  
  // Block signals outside of shell
  signal(SIGINT, SIG_IGN);
  signal(SIGTSTP, SIG_IGN);
  signal(SIGTTIN, SIG_IGN);
  signal(SIGTTOU, SIG_IGN);
  
  // Initialize job control stack
  jobStack = (JobNode_t**)malloc(sizeof(JobNode_t*));
  *jobStack = NULL;

  // Reset pgrp
  pgrp = -1;
  fgProc = (char*)malloc(MAX_LINE_LEN * sizeof(char));

  while(input = readline(PROMPT)){
    if(signal(SIGINT, sigintHandler) == SIG_ERR){
      printf("signal(SIGINT) error");
    }
    if(signal(SIGTSTP, sigtstpHandler) == SIG_ERR){
      printf("signal(SIGTSTP) error");
    } 
    if(signal(SIGCHLD, sigchldHandler) == SIG_ERR){
      printf("signal(SIGCHLD) error");
    } 

    validInput = checkInput(input);
    if(validInput){
      char** pipeArray = splitStrArray(input, PIPE);
      if(pipeArray[1] == NULL){
        // no pipe
        char** cmd = splitStrArray(input, SPACE_CHAR);

        manageJobs(cmd, input, jobStack);

        index = 0;
        while(cmd[index] != NULL){
          free(cmd[index]);
          index++;
        }
        free(cmd);
      }
      else{
        // pipe exists
        char** cmd1 = splitStrArray(pipeArray[0], SPACE_CHAR);
        char** cmd2 = splitStrArray(pipeArray[1], SPACE_CHAR);

        managePipeJobs(cmd1, cmd2, input, jobStack);

        index = 0;
        while(cmd1[index] != NULL){
          free(cmd1[index]);
          index++;
        }
        free(cmd1);
        
        index = 0;
        while(cmd2[index] != NULL){
          free(cmd2[index]);
          index++;
        }
        free(cmd2);
      }
      free(pipeArray);
    }
    else{
      printf("%c", NEW_LINE);
    }
    free(input);
  }

  free(jobStack);
  // TODO: free jobStack nodes
  // freeJobStack();
  free(fgProc);

  return;
}

/**
 * Purpose:
 *   Driver for shell program
 * 
 * Args:
 *   None
 * 
 * Returns:
 *   (int): 0 on exit success
 */
int main (void){

  shell();

  return 0;
}
