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

// Used for debugging
#include <errno.h>

// Directions here:
// https://docs.google.com/document/d/1LBMJslvYvw59uZ_8DNiiPzsp0heW3qesaalOo31IGYg/edit

// TODO: Refactor into several .c and .h files, separating read, parse, and
//       execute would be a good place to start
// haha I'm sorry about this

typedef struct StrNode_t{
  char* jobStr;

  struct StrNode_t* next;
} StrNode_t;

/**
 * Job struct
 */
typedef struct Job_t{
  char* jobStr;
  int jobId;
  int pgid;
  int inFG;
  int status;
}Job_t;

/**
 * JobNode_t struct
 */ 
typedef struct JobNode_t{
  Job_t* job;

  struct JobNode_t* next;
}JobNode_t;

JobNode_t** jobStack = NULL;
int fgExist = 0;
int fromFG = 0;
int pgrp = -1;
char* fgProc;

/**
 * Purpose:
 *   Push string node on to string stack
 * 
 * Args:
 *   head (StrNode_t**): Pointer to string stack head pointer
 * 
 * Returns:
 *   None 
 */ 
void pushStr(StrNode_t** head, char* str){
  StrNode_t* curr = (StrNode_t*)malloc(sizeof(StrNode_t));
  
  curr->jobStr = (char*)malloc(2001 * sizeof(char));
  strcpy(curr->jobStr, str);

  curr->next = (*head);
  (*head) = curr;
  
  return;
}

/**
 * Purpose:
 *   Print string and pop node off of stack
 * 
 * Args:
 *   head (StrNode_t**): Pointer to string stack head pointer
 * 
 * Returns:
 *   None
 */
void popStr(StrNode_t** head){
  StrNode_t* temp = NULL;

  if((*head) == NULL){
    return;
  }

  temp = (*head)->next;
  if((*head)->jobStr != NULL){
    printf("%s", (*head)->jobStr);
    free((*head)->jobStr);
  }
  free(*head);
  (*head) = temp;

  return;
}

/**
 * Purpose:
 *   Push JobNode to background stack
 * 
 * Args:
 *   head  (JobNode_t**): Pointer to job stack head pointer
 *   jobStr      (char*): Job string
 *   pgid          (int): Process group id of job
 *   status        (int): Running state of job
 *   fg            (int): Foreground status of job
 * 
 * Returns:
 *   None
 */ 
void pushNode(JobNode_t** head, char* jobStr, int pgid, int status, int inFG){
  Job_t* job = (Job_t*)malloc(sizeof(Job_t));
  JobNode_t* curr = (JobNode_t*)malloc(sizeof(JobNode_t));
  
  job->jobStr = (char*)malloc(2001 * sizeof(char));
  strcpy(job->jobStr, jobStr);
  job->pgid = pgid;
  if(*head == NULL){
    job->jobId = 1;
  }
  else{
    job->jobId = (*head)->job->jobId + 1;
  }  
  job->status = status;
  job->inFG = inFG;

  curr->job = job;

  curr->next = *head;
  *head = curr;
  return;
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
void freeJobStack(JobNode_t** head){
  JobNode_t* curr = *head;
  JobNode_t* temp = NULL;

  while(curr != NULL){
    temp = curr;
    curr = curr->next;
    if(temp->job != NULL){
      if(temp->job->jobStr != NULL)
        free(temp->job->jobStr);
      free(temp->job);
    }
    free(temp);
  }

  return;
}

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
 *   (Job_t*): Job pointer of desired job
 */ 
int findID(JobNode_t** head, int id){
  JobNode_t* curr = *head;
  Job_t* currJob = NULL;
  
  while(curr != NULL){
    currJob = curr->job;
    if(currJob->pgid == id){
      return 1;
    }
    curr = curr->next;
  }

  return 0;
}

/**
 * Purpose:
 *   Chack if pgid is in job stack
 * 
 * Args:
 *   head (JobNode_t**): Pointer to job stack head pointer
 *   id           (int): PGID of desired process
 * 
 * Returns:
 *   (int): 1 (true) if in stack, else 0 (false)
 */ 
int isInFG(JobNode_t** head, int id){
  const int IN_FG = 1;
  JobNode_t* curr = *head;
  Job_t* currJob = NULL;
  
  while(curr != NULL){
    currJob = curr->job;
    if(currJob->inFG == IN_FG){
      return 1;
    }
    curr = curr->next;
  }

  return 0;
}

/**
 * Purpose:
 *   Find most recent job that is in background
 * 
 * Args:
 *   head (JobNode_t**): Pointer to job stack head pointer
 * 
 * Returns:
 *   (Job_t*): Pointer to most recent background job
 */ 
Job_t* findRecentBG(JobNode_t** head){
  const int DONE_VAL = 2;

  JobNode_t* curr = *head;
  Job_t* currJob = NULL;
  
  while(curr != NULL){
    currJob = curr->job;
    if((currJob->status != DONE_VAL) && !(currJob->inFG)){
      return currJob;
    }
    curr = curr->next;
  }

  return NULL;
}

/**
 * Purpose:
 *   Find most recent stopped job
 * 
 * Args:
 *   head (JobNode_t**): Pointer to job stack head pointer
 * 
 * Returns:
 *   (Job_t*): Pointer to most recent stopped job
 */ 
Job_t* findRecentStopBG(JobNode_t** head){
  const int STOPPED_VAL = 1;
  const int DONE_VAL = 2;

  JobNode_t* curr = *head;
  Job_t* currJob = NULL;

  while(curr != NULL){
    currJob = curr->job;
    if((currJob->status == STOPPED_VAL) ||
       ((currJob->status != DONE_VAL) && !(currJob->inFG))){
      return currJob;
    }
    curr = curr->next;
  }

  return NULL;
}

/**
 * Purpose:
 *   Find most recent stopped job
 * 
 * Args:
 *   head (JobNode_t**): Pointer to job stack head pointer
 * 
 * Returns:
 *   (Job_t*): Pointer to most recent stopped job
 */ 
Job_t* findRecentStopped(JobNode_t** head){
  const int STOPPED_VAL = 1;

  JobNode_t* curr = *head;
  Job_t* currJob = NULL;

  while(curr != NULL){
    currJob = curr->job;
    if(currJob->status == STOPPED_VAL){
      return currJob;
    }
    curr = curr->next;
  }

  return NULL;
}

/**
 * Purpose:
 *   Find most recent stopped job
 * 
 * Args:
 *   head (JobNode_t**): Pointer to job stack head pointer
 * 
 * Returns:
 *   (Job_t*): Pointer to most recent stopped job
 */ 
Job_t* findFGProc(JobNode_t** head){
  JobNode_t* curr = *head;
  Job_t* currJob = NULL;

  while(curr != NULL){

    currJob = curr->job;
    // printf("findFGProc: %d\n", currJob->inFG);
    if(currJob->inFG){
      return currJob;
    }
    curr = curr->next;
  }

  return NULL;
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
  const int MAX_PRINT_LEN = 2032;
  const int DONE_VAL = 2;
  const char CURRENT = '+';
  const char BACK = '-';
  const char* DONE_TXT = "Done";
  const char* FORMAT = "[%d]%c  %s            %s\n";

  int currentJobID;
  char currentJob;
  char* strEntry = NULL;
  
  JobNode_t* curr = *head;
  Job_t* currJob = NULL;
  StrNode_t** strHead = (StrNode_t**)malloc(sizeof(StrNode_t*));
  *strHead = NULL;

  if(curr != NULL){
    currJob = curr->job;
    currentJobID = currJob->jobId;
  }

  while(curr != NULL){
    currJob = curr->job;
    if(currJob->jobId == currentJobID){
      currentJob = CURRENT;
    }
    else{
      currentJob = BACK;
    }

    if(currJob->status == DONE_VAL){
      strEntry = (char*)malloc(MAX_PRINT_LEN * sizeof(char));
      sprintf(strEntry, FORMAT, currJob->jobId, currentJob, DONE_TXT,
             currJob->jobStr);
      pushStr(strHead, strEntry);
      free(strEntry);
    }

    curr = curr->next;
  }

  while((*strHead) != NULL){
    popStr(strHead);
  }
  free(strHead);

  return;
}

/**
 * Purpose:
 *   Print stack of jobs
 * 
 * Args:
 *   head (JobNode_t**): Pointer to job stack head pointer
 * 
 * Returns:
 *   None
 */
void printStack(JobNode_t** head){
  const int MAX_PRINT_LEN = 2023;
  const int RUN_VAL = 0;
  const int STOPPED_VAL = 1;
  const int DONE_VAL = 2;
  const char CURRENT = '+';
  const char BACK = '-';
  const char* RUN_TXT = "Running";
  const char* STOP_TXT = "Stopped";
  const char* DONE_TXT = "Done";
  const char* OTHR_FMT = "[%d]%c  %s         %s %d\n";
  const char* DONE_FMT = "[%d]%c  %s            %s %d\n";
  
  int currentID;
  char currentJob;
  char* strEntry;

  JobNode_t* curr = *head;
  Job_t* currJob = NULL;
  StrNode_t** strHead = (StrNode_t**)malloc(sizeof(StrNode_t*));
  *strHead = NULL;
  
  if(curr != NULL){
    currJob = curr->job;
    currentID = currJob->jobId;
  }

  while(curr != NULL){
    currJob = curr->job;
    if(currJob->jobId == currentID){
      currentJob = CURRENT;
    }
    else{
      currentJob = BACK;
    }

    if(currJob->status == RUN_VAL){
      strEntry = (char*)malloc(MAX_PRINT_LEN * sizeof(char));
      sprintf(strEntry, OTHR_FMT, currJob->jobId, currentJob, RUN_TXT,
           currJob->jobStr, currJob->pgid);

      pushStr(strHead, strEntry);
      free(strEntry);
    }
    else if(currJob->status == STOPPED_VAL){
      strEntry = (char*)malloc(MAX_PRINT_LEN * sizeof(char));
      sprintf(strEntry, OTHR_FMT, currJob->jobId, currentJob, STOP_TXT,
           currJob->jobStr, currJob->pgid);

      pushStr(strHead, strEntry);
      free(strEntry);
    }
    else if(currJob->status == DONE_VAL){
      strEntry = (char*)malloc(MAX_PRINT_LEN * sizeof(char));
      sprintf(strEntry, DONE_FMT, currJob->jobId, currentJob, DONE_TXT,
           currJob->jobStr, currJob->pgid);

      pushStr(strHead, strEntry);
      free(strEntry);
    }
    curr = curr->next;
  }

  while((*strHead) != NULL){
    popStr(strHead);
  }
  free(strHead);

  return;
}

/**
 * Purpose:
 *   Change running status of job in job stack
 * 
 * Args:
 *   head   (JobNode_t**): Pointer to job stack head pointer
 *   pgid           (int): Process group ID
 *   newStat        (int): New running status of process
 * 
 * Returns:
 *   None
 */ 
void changeJobStatus(JobNode_t** head, int pgid, int newStat){
  JobNode_t* temp = *head;
  Job_t* currJob = NULL;

  while(temp != NULL){
    currJob = temp->job;
    if(currJob->pgid == pgid){
      currJob->status = newStat;

      return;
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
 *   head   (JobNode_t**): Pointer to job stack head pointer
 *   pgid           (int): Process group ID
 *   newStat        (int): New FG running status of process
 * 
 * Returns:
 *   None
 */ 
void changeJobFGState(JobNode_t** head, int pgid, int newFGStat){
  JobNode_t* temp = *head;
  Job_t* currJob = NULL;

  while(temp != NULL){
    currJob = temp->job;
    if(currJob->pgid == pgid){
      currJob->inFG = newFGStat;

      return;
    }
    temp = temp->next;
  }

  return;
}

/**
 * Purpose:
 *   Remove job by pid from job stack
 * 
 * Args:
 *   head (JobNode_t**): Pointer to job stack head pointer
 *   pgid         (int): PGID of process to remove
 * 
 * Returns:
 *   None
 */ 
void removeJob(JobNode_t** head, int pgid){
  JobNode_t* curr = *head;
  JobNode_t* temp = NULL;
  Job_t* currJob = NULL;

  if(curr == NULL){
    return;
  }
  else if(curr->job->pgid == pgid){
    (*head) = curr->next;

    currJob = curr->job;
    if(currJob != NULL){
      if((currJob->jobStr) != NULL)
        free(currJob->jobStr);
      free(currJob);
    }
    if(curr != NULL)
      free(curr);

    return;
  }
  temp = curr->next;
  currJob = curr->job;
  if(currJob->pgid == pgid){
    if(currJob != NULL){
      if((currJob->jobStr) != NULL)
        free(currJob->jobStr);
      free(currJob);
    }
    if(curr != NULL)
      free(curr);

    *head = temp;
    return;
  }
  while(temp != NULL){
    currJob = temp->job;

    if(currJob->pgid == pgid){
      (*head)->next = temp->next;

      if(currJob != NULL){
        if(currJob->jobStr != NULL)
          free(currJob->jobStr);
      }
      if(temp != NULL)
        free(temp);

      return;
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
  JobNode_t* temp = NULL;
  Job_t* currJob = NULL;

  if(curr->job != NULL){
    currJob = curr->job;
  }

  while((curr != NULL) && (currJob->status == DONE)){
    temp = curr;
    (*head) = curr->next;
    curr = (*head);
    
    currJob = temp->job;
    if(currJob != NULL){
      if((currJob->jobStr) != NULL)
        free(currJob->jobStr);
      free(currJob);
    }
    if(temp != NULL)
      free(temp);

    if(curr != NULL){
      temp = curr->next;
    }
  }

  if(curr == NULL){
    // *head = NULL;
    return;
  }
  else{
    curr = (*head);
    temp = curr->next;

    while(temp != NULL){
      currJob = temp->job;
      if(currJob->status == DONE){
        curr->next = temp->next;

        if(currJob != NULL){
          if(currJob->jobStr != NULL)
            free(currJob->jobStr);
          free(currJob);
        }
        if(temp != NULL)
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
  Job_t* fgJob = findFGProc(jobStack);

	if(fgJob != NULL){
    pgrp = fgJob->pgid;
    // printf("SIGINT: %d\n", pgrp);
    killpg(pgrp, SIGINT);
	}
  else{
    printf("\n%s", PROMPT);
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
  const int IN_BG = 0;
  const char* PROMPT = "# ";
  Job_t* fgJob = findFGProc(jobStack);
  // printf("TSTP Handler run!\n");

	if(fgJob != NULL){
    pgrp = fgJob->pgid;
    // printf("SIGTSTP: %d\n", pgrp);
    changeJobFGState(jobStack, pgrp, IN_BG);
    kill(-pgrp, SIGTSTP);
	}
  else{
    printf("\n%s", PROMPT);
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
  if(signal(SIGINT, sigintHandler) == SIG_ERR){
    printf("signal(SIGINT) error");
  }
  if(signal(SIGTSTP, sigtstpHandler) == SIG_ERR){
    printf("signal(SIGTSTP) error");
  } 
  if(signal(SIGCHLD, sigchldHandler) == SIG_ERR){
    printf("signal(SIGCHLD) error");
  } 

  const int STOPPED = 1;
  const int DONE = 2;
  const int IN_BG = 0;
  int status;
  int waitRet;
  int exists = 0;

  while ((waitRet = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
    // Reaping function
    if (WIFEXITED(status)){
      // Child exited normally
      exists = findID(jobStack, waitRet);
	    if(exists){
        changeJobStatus(jobStack, waitRet, DONE);
        if(isInFG(jobStack, waitRet))
          removeJob(jobStack, waitRet);
      }
    }
    else if (WIFSIGNALED(status)) {
      // Child killed by signal
      exists = findID(jobStack, waitRet);
	    if(exists)
        removeJob(jobStack, waitRet);
    }
    else if (WIFSTOPPED(status)) {
      // Child stopped by signal
      exists = findID(jobStack, waitRet);
      if(!exists)
        pushNode(jobStack, fgProc, waitRet, STOPPED, IN_BG);
      else
        changeJobStatus(jobStack, waitRet, STOPPED);
    }
    // printf("SIGCHLD on pgrp: %d\n", pgrp);
  }

  // signal(SIGCHLD, sigchldHandler);
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
  // TODO: Protect against these cases < ls, > ls, 2> ls, | ls, & ls
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
  const char PIPE_CHAR = '|';
  const char SPACE_CHAR = ' ';
  int allocSize;

  if(delim[0] == PIPE_CHAR){
    allocSize = MAX_LINE_LEN;
  }
  else if(delim[0] == SPACE_CHAR){
    allocSize = MAX_TOK_LEN;
  }

  char* inputCopy = (char*)malloc(MAX_LINE_LEN * sizeof(char));
  strcpy(inputCopy, input);
  
  char** splitted = NULL;
  int numElements = 0;

  char* token = strtok(inputCopy, delim);
  char* entry = (char*)malloc(allocSize * sizeof(char));
  strcpy(entry, token);

  while(token != NULL){
    numElements++;
    splitted = realloc(splitted, numElements * sizeof(char*));
    splitted[numElements - 1] = entry;

    token = strtok(NULL, delim);
    if(token != NULL){
      entry = (char*)malloc(allocSize * sizeof(char));
      strcpy(entry, token);
    }
  }

  // Assign NULL to last index
  numElements++;
  splitted = realloc(splitted, numElements * sizeof(char*));
  splitted[numElements - 1] = 0;

  if(inputCopy != NULL)
    free(inputCopy);
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
        S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH)) == INVALID){ 
      perror(cmd[outIndex]);
      exit(EXIT_FAILURE);
    }
    dup2(fdOut, STDOUT_FILENO);
    close(fdOut);
  }
  if(errIndex != INVALID){
    if((fdErr = open(cmd[errIndex], O_CREAT | O_WRONLY | O_TRUNC,
        S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH)) == INVALID){ 
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
  const int MAX_LINE_LEN = 2001;
  const int RUNNING = 0;
  const int STOPPED = 1;
  const int DONE = 2;

  const int IN_FG = 1;
  const int IN_BG = 0;
  
  int status;
  int exists = 0;

  int pidCh1 = fork();

  if(pidCh1 < 0){
    // fork failed; exit
    exit(EXIT_FAILURE);
  }
  else if(pidCh1 == 0){
    // child (new process)
    setpgid(0,0);
    redirectFile(cmd);
    execvp(cmd[0], cmd);

    // printf("\nEXEC PGID: %d\n", getpgid(0));
    exit(EXIT_FAILURE);
  }

  // parent process
  // printf("EXEC PID: %d\n", pidCh1);
  if(fgProc != NULL)
    free(fgProc);

  fgProc = (char*)malloc(MAX_LINE_LEN * sizeof(char));
  strcpy(fgProc, input);
  if(!back){
    pushNode(head, input, pidCh1, RUNNING, IN_FG);

    // wait for signal
    waitpid(pidCh1, &status, WCONTINUED | WUNTRACED);
    if (WIFEXITED(status)){
      // Child exited normally
      exists = findID(jobStack, pidCh1);
	    if(exists){
        changeJobStatus(jobStack, pidCh1, DONE);
        if(isInFG(jobStack, pidCh1))
          removeJob(jobStack, pidCh1);
      }
    }
    else if (WIFSIGNALED(status)) {
      // Child killed by signal
      exists = findID(jobStack, pidCh1);
	    if(exists)
        removeJob(jobStack, pidCh1);
    }
    else if (WIFSTOPPED(status)) {
      // Child stopped by signal
      exists = findID(jobStack, pidCh1);
      if(!exists)
        pushNode(jobStack, fgProc, pidCh1, STOPPED, IN_BG);
      else
        changeJobStatus(jobStack, pidCh1, STOPPED);
    }
    return;
  }
  else{
    // Add background job to stack
    pushNode(head, input, pidCh1, RUNNING, IN_BG);
    return;
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
  const int STOPPED = 1;
  const int DONE = 2;

  const int IN_FG = 1;
  const int IN_BG = 0;
  
  int status;
  int exists = 0;

  int pidCh1;
  int pidCh2;
  int pfd[2];

  pipe(pfd);
  pidCh1 = fork();
  if(pidCh1 < 0) {
    // fork failed; exit
    exit(EXIT_FAILURE);
  }
  else if(pidCh1 == 0){
    // child 1 (new process)
    setpgid(0,0);
    dup2(pfd[1], 1);
    close(pfd[0]);
    redirectFile(cmd1);
    execvp(cmd1[0], cmd1);

    exit(EXIT_FAILURE);
  }

  if(fgProc != NULL)
    free(fgProc);

  fgProc = (char*)malloc(MAX_LINE_LEN * sizeof(char));
  strcpy(fgProc, input);
  pidCh2 = fork();
  if(pidCh2 < 0){
    // fork failed; exit
    exit(EXIT_FAILURE);
  }
  else if(pidCh2 == 0){\
    // child 2 (new process)
    setpgid(0, pidCh1);
    dup2(pfd[0], 0);
    close(pfd[1]);
    redirectFile(cmd2);
    execvp(cmd2[0], cmd2);

    exit(EXIT_FAILURE);
  }
  // parent process
  close(pfd[0]);
  close(pfd[1]);
  
  if(!back){
    pushNode(head, input, pidCh1, RUNNING, IN_FG);

    waitpid(pidCh1, &status, WCONTINUED | WUNTRACED);
    if(WIFEXITED(status)){
      // Child exited normally
      exists = findID(jobStack, pidCh1);
	    if(exists){
        changeJobStatus(jobStack, pidCh1, DONE);
        if(isInFG(jobStack, pidCh1))
          removeJob(jobStack, pidCh1);
      }
    }
    else if(WIFSIGNALED(status)){
      // Child killed by signal
      exists = findID(jobStack, pidCh1);
	    if(exists)
        removeJob(jobStack, pidCh1);
    }
    else if(WIFSTOPPED(status)){
      // Child stopped by signal
      exists = findID(jobStack, pidCh1);
      if(!exists)
        pushNode(jobStack, fgProc, pidCh1, STOPPED, IN_BG);
      else
        changeJobStatus(jobStack, pidCh1, STOPPED);
    }
    return;
  }
  else{
    // Add background job to stack
    pushNode(head, input, pidCh1, RUNNING, IN_BG);
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
  if(signal(SIGINT, sigintHandler) == SIG_ERR){
    printf("signal(SIGINT) error");
  }
  if(signal(SIGTSTP, sigtstpHandler) == SIG_ERR){
    printf("signal(SIGTSTP) error");
  } 
  if(signal(SIGCHLD, sigchldHandler) == SIG_ERR){
    printf("signal(SIGCHLD) error");
  } 

  const int RUNNING = 0;
  const int STOPPED = 1;
  const int DONE = 2;

  const int IN_FG = 1;
  const int IN_BG = 0;
  
  int status;
  int exists = 0;
  int recentPGID;
  
  Job_t* recent = findRecentStopBG(head);
  fromFG = 1;
  // printf("FG run!\n");

  if(recent != NULL){
    recentPGID = recent->pgid;
    // tcsetpgrp(0, recentPGID); 
    // printf("%s\nFG: %d\n", recent->jobStr, recentPGID);
    changeJobStatus(head, recentPGID, RUNNING);
    changeJobFGState(head, recentPGID, IN_FG);
    kill(-recentPGID, SIGCONT);

    // wait for signal
    waitpid(recentPGID, &status, WCONTINUED | WUNTRACED);
    if (WIFEXITED(status)){
      // Child exited normally
      exists = findID(jobStack, recentPGID);
	    if(exists){
        changeJobStatus(jobStack, recentPGID, DONE);
        if(isInFG(jobStack, recentPGID))
          removeJob(jobStack, recentPGID);
      }
      removeJob(head, recentPGID);
    }
    else if (WIFSIGNALED(status)) {
      // Child killed by signal
      exists = findID(jobStack, recentPGID);
	    if(exists)
        removeJob(jobStack, recentPGID);
      removeJob(head, recentPGID);
    }
    else if (WIFSTOPPED(status)) {
      // Child stopped by signal
      exists = findID(jobStack, recentPGID);
      if(!exists)
        pushNode(jobStack, fgProc, recentPGID, STOPPED, IN_BG);
      else
        changeJobStatus(jobStack, recentPGID, STOPPED);
    }
    // tcsetpgrp(0, getpid()); 
  }
	return;
}

/**
 * Purpose:
 *   Print out background string
 * 
 * Args:
 *   head (JobNode_t**): Pointer to stack head pointer
 *   pgid         (int): PGID of desired job
 * 
 * Returns:
 *   None
 */
void printBGStr(JobNode_t** head, int pgid){
  const char CURRENT = '+';
  const char* BG_TOK = " &";

  JobNode_t* curr = *head;
  Job_t* currJob = NULL;

  while(curr != NULL){
    currJob = curr->job;
    if(currJob->pgid == pgid){
      if(!fromFG){
        strcat(currJob->jobStr, BG_TOK);
      }
      printf("[%d]%c %s\n", currJob->jobId, CURRENT, currJob->jobStr);
      return;
    }
    curr = curr->next;
  }

  return;
}

/**
 * Purpose:
 *   Send SIGCONT to most recent job in job stack and run in background
 * 
 * Args:
 *   head (JobNode_t**): Pointer to stack head pointer
 * 
 * Returns:
 *   None
 */ 
void runBackground(JobNode_t** head){
  const int IN_BG = 0;
  const int INVALID = -1;
  const int RUNNING = 0;
  int recentPGID;

  Job_t* recent = findRecentStopped(head);
  if(recent != NULL){
    recentPGID = recent->pgid;
    printf("BG: %d", recentPGID);
  }
  
  if(recentPGID != INVALID){
    printBGStr(head, recentPGID);
    changeJobStatus(head, recentPGID, RUNNING);
    changeJobFGState(head, recentPGID, IN_BG);
    killpg(recentPGID, SIGCONT);
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
      printStack(head);
    }

    return;
  } 
  else if(!strcmp(cmd[0], BG_TOK)){
    // execute bg
    runBackground(head);

    return;
  }
  else if(!strcmp(cmd[0], FG_TOK)){
    // execute fg
    runForeground(head);

    return;
  }
  else if(!strcmp(cmd[lastIndex], BACKGROUND)){
    // execute in background
    backState = 1;
    cmd[lastIndex] = NULL;

    executeGeneral(cmd, input, head, backState);

    return;
  }
  else{
    // execute normally
    backState = 0;
    fromFG = 0;
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
      printStack(head);
    }

    return;
  } 
  else if(!strcmp(cmd1[0], BG_TOK) || !strcmp(cmd2[0], BG_TOK)){
    // execute bg
    runBackground(head);

    return;
  }
  else if(!strcmp(cmd1[0], FG_TOK) || !strcmp(cmd2[0], FG_TOK)){
    // execute fg
    runForeground(head);

    return;
  }
  else if(!strcmp(cmd2[lastIndex], BACKGROUND)){
    // execute in background
    backState = 1;
    cmd2[lastIndex] = NULL;

    executePipe(cmd1, cmd2, input, head, backState);

    return;
  }
  else{
    // execute normally
    backState = 0;
    fromFG = 0;
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
  fgProc = (char*)malloc(MAX_LINE_LEN * sizeof(char));

  while((input = readline(PROMPT))){
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
    pgrp = -1;

    if((*jobStack) != NULL){
      printDoneJobs(jobStack);
      removeDoneJobs(jobStack);
    }
    if(validInput){
      char** pipeArray = splitStrArray(input, PIPE);
      if(pipeArray[1] == NULL){
        // no pipe
        char** cmd = splitStrArray(input, SPACE_CHAR);

        manageJobs(cmd, input, jobStack);

        index = 0;
        if(cmd != NULL){
          while(cmd[index] != NULL){
            free(cmd[index]);
            index++;
          }
          free(cmd);
        }

        if(pipeArray[0] != NULL)
          free(pipeArray[0]);
      }
      else{
        // pipe exists
        char** cmd1 = splitStrArray(pipeArray[0], SPACE_CHAR);
        char** cmd2 = splitStrArray(pipeArray[1], SPACE_CHAR);

        managePipeJobs(cmd1, cmd2, input, jobStack);
        
        if(cmd1 != NULL){
          index = 0;
          while(cmd1[index] != NULL){
            free(cmd1[index]);
            index++;
          }
          free(cmd1);
        }

        if(cmd2 != NULL){
        index = 0;
        while(cmd2[index] != NULL){
          free(cmd2[index]);
          index++;
        }
        free(cmd2);
        }

        if(pipeArray[0] != NULL)
          free(pipeArray[0]);
        
        if(pipeArray[1] != NULL)
          free(pipeArray[1]);
      }
      if(pipeArray != NULL)
        free(pipeArray);
    }

    if(input != NULL)
      free(input);
  }

  freeJobStack(jobStack);
  if(jobStack != NULL)
    free(jobStack);

  if(fgProc != NULL)
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
