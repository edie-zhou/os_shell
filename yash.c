#include <fcntl.h>
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
  // TODO: Add function to ensure file redir redirects to a file e.g.:# cat hello.txt >
  // TODO: Add function to ensure pipe pipes to a valid command e.g.:# ls |
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
 *   tokenArray (char**): Array of tokens from command input
 *   inIndex      (int*): Index of input redirection token 
 *   outIndex     (int*): Index of output redirection token 
 *   errIndex     (int*): Index of error redirection token 
 *   
 * Returns:
 *   None  
 */ 
void changeRedirToks(char** tokenArray, int* inIndex, int* outIndex,
                     int* errIndex){
  const char* IN_REDIR = "<";
  const char* OUT_REDIR = ">";
  const char* ERR_REDIR = "2>";

  int index = 0;
  while(tokenArray[index] != NULL){
    if(!strcmp(tokenArray[index], IN_REDIR)){
      *inIndex = index + 1;
      // Assign NULL to stop exec() from reading file redirection as part of
      // input
      tokenArray[index] = NULL;
    }
    else if(!strcmp(tokenArray[index], OUT_REDIR)){
      *outIndex = index + 1;
      tokenArray[index] = NULL;
    }
    else if(!strcmp(tokenArray[index], ERR_REDIR)){
      *errIndex = index + 1;
      tokenArray[index] = NULL;
    }
    index++;
  }

  return;
}

/**
 * Purpose:
 *   Execute input line with file redirections
 * 
 * Args: 
 *   tokenArray (char**): Token array from command input
 * 
 * Returns:
 *   None
 */
void executeGeneral(char** tokenArray){
  // TODO: Implement piping
  // TODO: Implement job control
  const int INVALID = -1;
  const char NEW_LINE = '\n';
  int inIndex = -1;
  int outIndex = -1;
  int errIndex = -1;
  int fdIn;
  int fdOut;
  int fdErr;
  int status;

  int child = fork();
  if (child < 0) {
    // fork failed; exit
    fprintf(stderr, "Fork failed\n");
    exit(EXIT_FAILURE);
  }
  else if (child == 0) {
    inIndex = -1;
    outIndex = -1;
    errIndex = -1;
    changeRedirToks(tokenArray, &inIndex, &outIndex, &errIndex);

    if(inIndex != INVALID){
      if((fdIn = open(tokenArray[inIndex], O_RDONLY, 0)) == INVALID){
          perror(tokenArray[inIndex]);
          exit(EXIT_FAILURE);
      }
      dup2(fdIn, STDIN_FILENO);
      close(fdIn);
    }
    if(outIndex != INVALID){
      if((fdOut = open(tokenArray[outIndex], O_CREAT | O_WRONLY | O_TRUNC,
          S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == INVALID){ 
        perror(tokenArray[outIndex]);
        exit(EXIT_FAILURE);
      }
      dup2(fdOut, STDOUT_FILENO);
      close(fdOut);
    }
    if(errIndex != INVALID){
      if((fdErr = open(tokenArray[errIndex], O_CREAT | O_WRONLY | O_TRUNC,
          S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == INVALID){ 
        perror(tokenArray[errIndex]);
        exit(EXIT_FAILURE);
      }
      dup2(fdErr, STDERR_FILENO);
      close(fdErr);
    }
    // child (new process)
    execvp(tokenArray[0], tokenArray);

    // print new line if execvp fails (==-1) and exit process
    // these lines will not run unless execvp() has failed
    printf("%c", NEW_LINE);
    exit(EXIT_FAILURE);
  }
  else {
    // parent goes down this path (main)
    // https://stackoverflow.com/questions/903864/how-to-exit-a-child-process-and-return-its-status-from-execvp
    wait(&status);
  } 
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
  // TODO: Implement piping
  // TODO: Implement job control
  const int INVALID = -1;
  const char NEW_LINE = '\n';
  int child1;
  int child2;
  int status1;
  int status2;
  int inIndex;
  int outIndex;
  int errIndex;
  int fdIn;
  int fdOut;
  int fdErr;

  int pfd[2];
  pipe(pfd);

  child1 = fork();
  if (child1 < 0) {
    // fork failed; exit
    fprintf(stderr, "Fork failed\n");
    exit(EXIT_FAILURE);
  }
  else if (child1 == 0){
    // file redir
    inIndex = -1;
    outIndex = -1;
    errIndex = -1;
    changeRedirToks(cmd1, &inIndex, &outIndex, &errIndex);
    if(inIndex != INVALID){
      if((fdIn = open(cmd1[inIndex], O_RDONLY, 0)) == INVALID){
          perror(cmd1[inIndex]);
          exit(EXIT_FAILURE);
      }
      dup2(fdIn, STDIN_FILENO);
      close(fdIn);
    }
    if(outIndex != INVALID){
      if((fdOut = open(cmd1[outIndex], O_CREAT | O_WRONLY | O_TRUNC,
          S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == INVALID){ 
        perror(cmd1[outIndex]);
        exit(EXIT_FAILURE);
      }
      dup2(fdOut, STDOUT_FILENO);
      close(fdOut);
    }
    if(errIndex != INVALID){
      if((fdErr = open(cmd1[errIndex], O_CREAT | O_WRONLY | O_TRUNC,
          S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == INVALID){ 
        perror(cmd1[errIndex]);
        exit(EXIT_FAILURE);
      }
      dup2(fdErr, STDERR_FILENO);
      close(fdErr);
    }

    // piping
    dup2(pfd[1], 1);
    close(pfd[0]);

    // child (new process)
    execvp(cmd1[0], cmd1);

    fprintf(stderr, "Exec failed\n");
    printf("%c", NEW_LINE);
    exit(EXIT_FAILURE);
  }
  else {
    // parent goes down this path (main)
    // https://stackoverflow.com/questions/903864/how-to-exit-a-child-process-and-return-its-status-from-execvp
    wait(&status1);

    child2 = fork();
    if (child2 < 0) {
      // fork failed; exit
      fprintf(stderr, "Fork failed\n");
      exit(EXIT_FAILURE);
    }
    else if (child2 == 0) {
      inIndex = -1;
      outIndex = -1;
      errIndex = -1;
      changeRedirToks(cmd2, &inIndex, &outIndex, &errIndex);
      
      if(inIndex != INVALID){
        if((fdIn = open(cmd2[inIndex], O_RDONLY, 0)) == INVALID){
            perror(cmd2[inIndex]);
            exit(EXIT_FAILURE);
        }
        dup2(fdIn, STDIN_FILENO);
        close(fdIn);
      }
      if(outIndex != INVALID){
        if((fdOut = open(cmd2[outIndex], O_CREAT | O_WRONLY | O_TRUNC,
            S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == INVALID){ 
          perror(cmd2[outIndex]);
          exit(EXIT_FAILURE);
        }
        dup2(fdOut, STDOUT_FILENO);
        close(fdOut);
      }
      if(errIndex != INVALID){
        if((fdErr = open(cmd2[errIndex], O_CREAT | O_WRONLY | O_TRUNC,
            S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == INVALID){ 
          perror(cmd2[errIndex]);
          exit(EXIT_FAILURE);
        }
        dup2(fdErr, STDERR_FILENO);
        close(fdErr);
      }

      // piping
      dup2(pfd[0], 0);
      close(pfd[1]);

      // child (new process)
      execvp(cmd2[0], cmd2);

      fprintf(stderr, "Exec failed\n");
      printf("%c", NEW_LINE);
      exit(EXIT_FAILURE);
    }
    else {
      // parent goes down this path (main)
      // https://stackoverflow.com/questions/903864/how-to-exit-a-child-process-and-return-its-status-from-execvp
      close(pfd[0]);
      close(pfd[1]);
      wait(&status2);
    } 
  }
}

/**
 * Purpose:
 *   Checks input tokens for job control tokens
 *     * bg, fg should be at tokenArray[0]
 *     * & should be at tokenArray[numTokens - 1]
 * 
 * Args:
 *   tokenArray (char**): Array of tokens from command input
 *   numTokens     (int): Number of tokens in string
 * 
 * Returns:
 *   None
 */
void checkJobControl(char** tokenArray, int numTokens){
  const char* BACKGROUND = "&";
  const char* BG_TOKEN = "bg";
  const char* FG_TOKEN = "fg";
  const int BACKGRND_OFFSET = 2;
  
  // TODO: Add input verification to ensure that bg, fg, & are at expected
  // indices
  if(!strcmp(tokenArray[0], BG_TOKEN)){
    // execute bg
    return;
  }
  else if(!strcmp(tokenArray[0], FG_TOKEN)){
    // execute fg
    return;
  }
  else if(!strcmp(tokenArray[numTokens - BACKGRND_OFFSET], BACKGROUND)){
    // execute background
    return;
  }
  else{
    // ??
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
  int index = 0;
  
  char* input;
  
  while(input = readline(PROMPT)){
    validInput = checkInput(input, MAX_LINE_LEN, MAX_TOKEN_LEN);
    if(validInput){
      char** pipeArray = splitStrArray(input, PIPE);
      // checkJobControl(tokenArray, numTokens);
      if(pipeArray[1] == NULL){
        // no pipe
        char** cmd = splitStrArray(input, SPACE_CHAR);

        executeGeneral(cmd);

        // free allocated memory
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

        executePipe(cmd1, cmd2);

        // free allocated memory
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

        index = 0;
        while(pipeArray[index] != NULL){
          free(pipeArray[index]);
          index++;
        }
        free(pipeArray);
      }
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
