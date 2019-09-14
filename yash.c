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

int pidCh1 = -1;
int pidCh2 = -1;

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
  printf("PID: %d\n", getpid());
  printf("child1 pid: %d\n", pidCh1);
  printf("child2 pid: %d\n", pidCh2);
	if(pidCh2 != -1){
		kill(pidCh2, SIGINT);
    // pidCh2 = -1;
    kill(pidCh1, SIGINT);
    // pidCh1 = -1;
	}
  else if(pidCh1 != -1){
    kill(pidCh1, SIGINT);
    // pidCh1 = -1;
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
void sigtstpHandler(int sigNum){
  // TODO: Change handler to background process instead of killing
  const char* PROMPT = "# ";
  printf("\nSIGTSTP\n");
  printf("PID: %d\n", getpid());
  printf("child1 pid: %d\n", pidCh1);
  printf("child2 pid: %d\n", pidCh2);
	if(pidCh2 != -1){
		kill(pidCh2, SIGTSTP);
    pidCh2 = -1;
    kill(pidCh1, SIGTSTP);
    pidCh1 = -1;
	}
  else if(pidCh1 != -1){
    kill(pidCh1, SIGTSTP);
    pidCh1 = -1;
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
 *   maxLineLen   (int): Maximum length of input
 *   maxTokenLen  (int): Maximum length of tokens
 * 
 * Returns:
 *   (int): Returns 1 if string is valid, 0 if string is invalid
 */
int checkInput(char* input, int maxLineLen, int maxTokenLen){
  // TODO: Add function to ensure file redir goes to a file e.g.:# cat hello.txt >
  // TODO: Add function to ensure pipe goes to a valid command e.g.:# ls |
  // TODO: Add input verification to ensure that bg, fg, & are at expected indices
  const int INVALID = 0;
  const int VALID = 1;
  if(strlen(input) > maxLineLen + 1){
    return INVALID;
  }
  else if(!checkTokens(input, maxLineLen, maxTokenLen)){
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
  char** splitted = NULL;
  int numElements = 0;

  char* token = strtok(input, delim);
  while(token != NULL){
    numElements++;
    splitted = realloc(splitted, numElements * sizeof(char*));
    splitted[numElements - 1] = token;
    token = strtok(NULL, delim);
  }

  // Assign NULL to last index
  numElements++;
  splitted = realloc(splitted, numElements * sizeof(char*));
  splitted[numElements - 1] = 0;

  return splitted;
}

/**
 * Purpose:
 *   Find indices for input, output, and error redirection in a command and
 *   replace tokens with NULL
 * 
 * Args:
 *   input   (char**): Array of tokens from command input
 *   inIndex   (int*): Index of input redirection token 
 *   outIndex  (int*): Index of output redirection token 
 *   errIndex  (int*): Index of error redirection token 
 *   
 * Returns:
 *   None  
 */ 
void changeRedirToks(char** input, int* inIndex, int* outIndex, int* errIndex){
  const char* IN_REDIR = "<";
  const char* OUT_REDIR = ">";
  const char* ERR_REDIR = "2>";

  int index = 0;
  while(input[index] != NULL){
    if(!strcmp(input[index], IN_REDIR)){
      *inIndex = index + 1;
      // Assign NULL to stop exec() from reading file redirection as part of
      // input
      input[index] = NULL;
    }
    else if(!strcmp(input[index], OUT_REDIR)){
      *outIndex = index + 1;
      input[index] = NULL;
    }
    else if(!strcmp(input[index], ERR_REDIR)){
      *errIndex = index + 1;
      input[index] = NULL;
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
 *   input (char**): input command
 *   
 * Returns:
 *   None
 */
void redirectFile(char** input){
  const int INVALID = -1;
  int inIndex = -1;
  int outIndex = -1;
  int errIndex = -1;
  int fdIn;
  int fdOut;
  int fdErr;

  changeRedirToks(input, &inIndex, &outIndex, &errIndex);

  if(inIndex != INVALID){
    if((fdIn = open(input[inIndex], O_RDONLY, 0)) == INVALID){
        perror(input[inIndex]);
        exit(EXIT_FAILURE);
    }
    dup2(fdIn, STDIN_FILENO);
    close(fdIn);
  }
  if(outIndex != INVALID){
    if((fdOut = open(input[outIndex], O_CREAT | O_WRONLY | O_TRUNC,
        S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == INVALID){ 
      perror(input[outIndex]);
      exit(EXIT_FAILURE);
    }
    dup2(fdOut, STDOUT_FILENO);
    close(fdOut);
  }
  if(errIndex != INVALID){
    if((fdErr = open(input[errIndex], O_CREAT | O_WRONLY | O_TRUNC,
        S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == INVALID){ 
      perror(input[errIndex]);
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
 *   input (char**): Token array from command input
 *   bg       (int): Boolean var for background status
 * 
 * Returns:
 *   None
 */
void executeGeneral(char** input){
  // TODO: Implement job control
  // TODO: Investigate WNOHANG waitpid flag
  const char NEW_LINE = '\n';
  int status;

  pidCh1 = fork();
  if (pidCh1 < 0) {
    // fork failed; exit
    fprintf(stderr, "Fork failed\n");
    exit(EXIT_FAILURE);
  }
  else if (pidCh1 == 0) {
    // child (new process)
    if (signal(SIGINT, sigintHandler) == SIG_ERR){
	    printf("signal(SIGINT) error");
    }
    if (signal(SIGTSTP, sigtstpHandler) == SIG_ERR){
    	printf("signal(SIGTSTP) error");
    }

    // setpgid(0,0);
    // tcsetpgrp(0, getpid());
    // tcsetpgrp(1, getpid());
    redirectFile(input);
    execvp(input[0], input);

    fprintf(stderr, "Exec failed\n");
    printf("%c", NEW_LINE);
    exit(EXIT_FAILURE);
  }
  // parent goes down this path (main)
  // setpgid(pidCh1, pidCh1);
  // tcsetpgrp(0, pidCh1);
  // tcsetpgrp(1, pidCh1);
  waitpid (pidCh1, &status, WCONTINUED | WUNTRACED);
  pidCh1 = -1;
  // tcsetpgrp(0, getpid());
  // tcsetpgrp(1, getpid());
}

/**
 * Purpose:
 *   Execute input line with file redirections and pipes
 * 
 * Args: 
 *   cmd1 (char**): Token array for command before pipe
 *   cmd2 (char**): Token array for command after pipe
 * 
 * Returns:
 *   None
 */
void executePipe(char** cmd1, char** cmd2){
  const char NEW_LINE = '\n';
  // int pidCh1;
  // int pidCh2;
  int status1;
  int status2;

  int pfd[2];
  pipe(pfd);

  pidCh1 = fork();
  if (pidCh1 < 0) {
    // fork failed; exit
    fprintf(stderr, "Fork failed\n");
    exit(EXIT_FAILURE);
  }
  else if (pidCh1 == 0){
    // child 1 (new process)
    if (signal(SIGINT, sigintHandler) == SIG_ERR){
	    printf("signal(SIGINT) error");
    }
    if (signal(SIGTSTP, sigtstpHandler) == SIG_ERR){
    	printf("signal(SIGTSTP) error");
    } 
    dup2(pfd[1], 1);
    close(pfd[0]);
    redirectFile(cmd1);
    execvp(cmd1[0], cmd1);

    fprintf(stderr, "Exec failed\n");
    printf("%c", NEW_LINE);
    exit(EXIT_FAILURE);
  }

  pidCh2 = fork();
  if (pidCh2 < 0) {
    // fork failed; exit
    fprintf(stderr, "Fork failed\n");
    exit(EXIT_FAILURE);
  }
  else if (pidCh2 == 0){
    // child 2 (new process)
    if (signal(SIGINT, sigintHandler) == SIG_ERR){
	    printf("signal(SIGINT) error");
    }
    if (signal(SIGTSTP, sigtstpHandler) == SIG_ERR){
    	printf("signal(SIGTSTP) error");
    } 
    dup2(pfd[0], 0);
    close(pfd[1]);
    redirectFile(cmd2);
    execvp(cmd2[0], cmd2);

    fprintf(stderr, "Exec failed\n");
    printf("%c", NEW_LINE);
    exit(EXIT_FAILURE);
  }
  // parent goes down this path (main)
  close(pfd[0]);
  close(pfd[1]);
  waitpid(-1, &status1, WCONTINUED | WUNTRACED);
  pidCh1 = -1;
  waitpid(-1, &status2, WCONTINUED | WUNTRACED);
  pidCh2 = -1;
}

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

/**
 * Purpose:
 *   Count number of nodes in stack
 * 
 * Args:
 *   head (JobNode_t**): Pointer to stack head pointer
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
 *   Push JobNode to background stack
 * 
 * Args:
 *   head  (JobNode_t**): Pointer to stack head pointer
 *   jobStr      (char*): Job string
 *   pid           (int): Process group id of job
 *   status        (int): Running state of job
 * 
 * Returns:
 *   None
 */ 
void pushNode(JobNode_t** head, char* jobStr, int pgid, int status){
  JobNode_t* temp = (JobNode_t*)malloc(sizeof(JobNode_t));

  temp->jobStr = (char*)malloc(2000*sizeof(char));
  strcpy(temp->jobStr, jobStr);
  temp->pgid = pgid;
  temp->jobId = countNodes(head) + 1;
  temp->status = status;
  temp->next = *head;

  *head = temp;

  return;
}

/**
 * Purpose:
 *   Pop JobNode off background stack
 * 
 * Args:
 *   head (JobNode_t**): Pointer to stack head pointer
 *   
 * Returns:
 *   (int): Process group ID, -1 if failed
 */
int popNode(JobNode_t** head){
  const int INVALID = -1;
  JobNode_t* temp;
  int popped;

  if(*head == NULL){
    return INVALID;
  }

  temp = (*head)->next;
  popped = (*head)->pgid;
  free((*head)->jobStr);
  free(*head);
  *head = temp;

  return popped;
}

/**
 * Purpose:
 *   Print stack of background jobs
 * 
 * Args:
 *   head (JobNode_t**): Pointer to stack head pointer
 * 
 * Returns:
 *   None
 */
void printStack(JobNode_t** head){
  const int RUN_VAL = 0;
  const int STOPPED_VAL = 1;
  const char CURRENT = '+';
  const char BACK = '-';

  const char* RUN_TXT = "RUNNING";
  const char* STOP_TXT = "STOPPED";
  const char* status;
  char currentJob;
  
  JobNode_t* temp = *head;
  int currentID = temp->jobId;

  while(temp != NULL){
    if(temp->jobId == currentID){
      currentJob = CURRENT;
    }
    else{
      currentJob = BACK;
    }

    if(temp->status == RUN_VAL){
      status = RUN_TXT;
    }
    else if(temp->status == STOPPED_VAL){
      status = STOP_TXT;
    }

    printf("[%d] %c %s         %s \n", temp->jobId, currentJob, status,
           temp->jobStr);
    temp = temp->next;
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
 *   input     (char**): Array of tokens from command input
 *   head (JobNode_t**): Pointer to stack head pointer
 * 
 * Returns:
 *   None
 */
void manageJobs(char** input, JobNode_t** head){
  const char* BACKGROUND = "&";
  const char* BG_TOK = "bg";
  const char* FG_TOK = "fg";
  const char* JOBS_TOK = "jobs";

  int lastIndex = 0;
  while(input[lastIndex] != NULL){
    lastIndex++;
  }
  lastIndex--;

  if(!strcmp(input[0], JOBS_TOK)){
    // execute jobs list
    printf("RUNNING JOBS\n");
    printStack(head);
    return;
  } 
  else if(!strcmp(input[0], BG_TOK)){
    // execute bg
    printf("RUNNING BG\n");
    return;
  }
  else if(!strcmp(input[0], FG_TOK)){
    // execute fg
    printf("RUNNING FG\n");
    return;
  }
  else if(!strcmp(input[lastIndex], BACKGROUND)){
    // execute background
    printf("RUNNING &\n");
    input[lastIndex] = NULL;
    return;
  }
  else{
    // execute normally
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
void shellLoop(void){
  const char NEW_LINE = '\n';
  const char* PIPE = "|";
  const char* SPACE_CHAR = " ";
  const char* PROMPT = "# ";
  const int MAX_LINE_LEN = 2001;
  const int MAX_TOKEN_LEN = 31;
  int validInput = 0;

  // index for memory frees
  // int index = 0;
  
  // Initialize job control stack
  JobNode_t* nullEntry = NULL;
  JobNode_t** jobStack = (JobNode_t*)malloc(sizeof(JobNode_t*));
  *jobStack = nullEntry;

  char* input;

  // Reset pid's
  pidCh1 = -1;
  pidCh2 = -1;

  // Block signals outside of shell
  signal(SIGINT, SIG_IGN);
  signal(SIGTSTP, SIG_IGN);
  signal(SIGTTIN, SIG_IGN);
  signal(SIGTTOU, SIG_IGN);

  while(input = readline(PROMPT)){
    if (signal(SIGINT, sigintHandler) == SIG_ERR){
      printf("signal(SIGINT) error");
    }
    if (signal(SIGTSTP, sigtstpHandler) == SIG_ERR){
      printf("signal(SIGTSTP) error");
    } 
    printf("***\n1: %d\n", pidCh1);
    printf("2: %d\n***\n", pidCh2);
    validInput = checkInput(input, MAX_LINE_LEN, MAX_TOKEN_LEN);
    if(validInput){
      // TODO: fix free statements
      char** pipeArray = splitStrArray(input, PIPE);
      if(pipeArray[1] == NULL){
        // no pipe
        char** cmd = splitStrArray(input, SPACE_CHAR);

        manageJobs(cmd, jobStack);
        executeGeneral(cmd);

        // free allocated memory
        // index = 0;
        // while(cmd[index] != NULL){
        //   free(cmd[index]);
        //   index++;
        // }
        // free(cmd[index]);
        // free(cmd);
      }
      else{
        // pipe exists
        char** cmd1 = splitStrArray(pipeArray[0], SPACE_CHAR);
        char** cmd2 = splitStrArray(pipeArray[1], SPACE_CHAR);

        executePipe(cmd1, cmd2);

        // free allocated memory
        // index = 0;
        // int cmd1Freed = 0;
        // int cmd2Freed = 0;
        // while((cmd1[index] != NULL) || (cmd2[index] != NULL)){
        //   if((cmd1[index] != NULL) && !cmd1Freed){
        //     free(cmd1[index]);
        //   }
        //   if((cmd2[index] != NULL) && !cmd2Freed){
        //     free(cmd2[index]);
        //   }
        //   index++;
        // }
        // free(cmd1);
        // free(cmd2);
      }
      // free(pipeArray);
    }
    else{
      printf("%c", NEW_LINE);
    }
    free(input);
  }

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
  
  shellLoop();

  return 0;
}
