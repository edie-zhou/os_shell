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
  const char SPACE_CHAR = ' ';
  int tokenLen = 0;
  int k = 0;
  while(k < strlen(input)){
    if(tokenLen >= maxTokenLen){
      return 0;
    }
    else if((tokenLen < maxTokenLen) && (input[k] == SPACE_CHAR)){
      tokenLen = 0;
    }
    else{
      tokenLen++;
    }
    k++;
  }
  return 1;
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
 *   Count number of tokens in input line
 * 
 * Args:
 *   input     (char*): Pointer to input c-string
 *   maxLineLen  (int): Maximum length of tokens
 * 
 * Returns:
 *   (int): Number of tokens in input line
 */
int countTokens(char* input, int maxLineLen){
  // strtok inserts null terminators in space delimiters
  char* inputCopy = (char*) malloc(maxLineLen * sizeof(char));
  strcpy(inputCopy, input);

  char* delimiters = " ";
  char* token = strtok(inputCopy, delimiters);
  int numTokens = 1;
  while(token != NULL){
    token = strtok(NULL, delimiters);
    numTokens++;
  }
  free(inputCopy);
  return numTokens;
} 

/**
 * Purpose:
 *   Create array of token C-strings from input line
 * 
 * Args:
 *   input      (char*): Pointer to input c-string
 *   numTokens    (int): Number of tokens in string
 *   maxLineLen   (int): Maximum length of input
 *   maxTokenLen  (int): Maximum length of tokens
 *  
 * Returns:
 *   (char**): Returns array of token c-strings
 * 
 */
char** createTokenArray(char* input, int numTokens, int maxLineLen,
                        int maxTokenLen){

  // strtok inserts null terminators in space delimiters
  // Remove this copy if input will not be used again
  char* inputCopy = (char*) malloc(maxLineLen * sizeof(char));
  strcpy(inputCopy, input);

  char** tokenArray = (char**)calloc(numTokens, sizeof(char*));
  char* arrayEntry = (char*)malloc(maxTokenLen * sizeof(char));

  const char* delimiter = " ";
  int index = 0;
  char* token = strtok(inputCopy, delimiter);
  strcpy(arrayEntry, token);
  tokenArray[index] = arrayEntry;
  while(token != NULL){
    index++;
    token = strtok(NULL, delimiter);
    if(token != NULL){
      char* arrayEntry = (char*)malloc(maxTokenLen * sizeof(char));
      strcpy(arrayEntry, token);
      tokenArray[index] = arrayEntry;
    }
  }
  free(inputCopy);
  free(token);

  return tokenArray;
}

/**
 * Purpose:
 *   Execute general input commands from command line
 * 
 * Args:
 *   tokenArray (char**): Array of tokens from command input
 * 
 * Returns:
 *   None 
 */
void execGeneral(char** tokenArray){
  const char NEW_LINE = '\n';
  int child = fork();
  if (child < 0) {
    // fork failed; exit
    fprintf(stderr, "Fork failed\n");
    exit(EXIT_FAILURE);
  }
  else if (child == 0) {
    // child (new process)
    execvp(tokenArray[0], tokenArray);

    // print new line if execvp fails (==-1) and exit process
    // these lines will not run unless execvp has failed
    printf("%c", NEW_LINE);
    exit(EXIT_FAILURE);
  }
  else {
    // parent goes down this path (main)
    // https://stackoverflow.com/questions/903864/how-to-exit-a-child-process-and-return-its-status-from-execvp
    wait(NULL);
  }

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
  const char* ERR_REDIR = ">>";

  int index = 0;
  while(tokenArray[index] != NULL){
    if(!strcmp(tokenArray[index],IN_REDIR)){
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
 *   tokenArray (char**): Array of tokens from command input
 * 
 * Returns:
 *   None
 */
void execute(char** tokenArray){
  // TODO: Finish this function
  const int INVALID = -1;
  const char NEW_LINE = '\n';
  int inIndex = -1;
  int outIndex = -1;
  int errIndex = -1;
  int fdIn;
  int fdOut;

  changeRedirToks(tokenArray, &inIndex, &outIndex, &errIndex);

  int child = fork();
  if (child < 0) {
    // fork failed; exit
    fprintf(stderr, "Fork failed\n");
    exit(EXIT_FAILURE);
  }
  else if (child == 0) {
    if(inIndex != INVALID){
      close(STDIN_FILENO);
      if((fdIn = open(tokenArray[inIndex], O_RDONLY, 0)) == INVALID){
          perror(tokenArray[inIndex]);
          exit(EXIT_FAILURE);
      }
      dup2(fdIn, STDIN_FILENO);
      close(fdIn);
    }
    if(outIndex != INVALID){
      close(STDOUT_FILENO);
      if((fdOut = open(tokenArray[outIndex], O_CREAT | O_WRONLY | O_TRUNC,
          S_IRUSR | S_IRUSR | S_IRGRP | S_IROTH)) == INVALID){ 
        perror(tokenArray[outIndex]);
        exit(EXIT_FAILURE);
      }
      dup2(fdOut, STDOUT_FILENO);
      close(fdOut);
    }
    if(errIndex != INVALID){
      close(STDERR_FILENO);
      if((fdOut = open(tokenArray[errIndex], O_CREAT | O_WRONLY | O_TRUNC,
          S_IRUSR | S_IRUSR | S_IRGRP | S_IROTH)) == INVALID){ 
        perror(tokenArray[errIndex]);
        exit(EXIT_FAILURE);
      }
      dup2(fdOut, STDERR_FILENO);
      close(fdOut);
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
    wait(NULL);
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
  else if(!strcmp(tokenArray[numTokens - 1], BACKGROUND)){
    // execute background
    return;
  }
  else{
    execute(tokenArray);
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
  const int MAX_LINE_LEN = 2000;
  const int MAX_TOKEN_LEN = 30;
  int validInput = 0;
  int numTokens = 0;
  char* prompt = "# ";
  char* input;
  
  while(input = readline(prompt)){
    validInput = checkInput(input, MAX_LINE_LEN, MAX_TOKEN_LEN);
    if(validInput){

      numTokens = countTokens(input, MAX_LINE_LEN);
      char** tokenArray = createTokenArray(input, numTokens, MAX_LINE_LEN,
                                           MAX_TOKEN_LEN);

      checkJobControl(tokenArray, numTokens);

      for(int k = 0; k < numTokens; k++){
        free(tokenArray[k]);
      }
      free(tokenArray);
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
