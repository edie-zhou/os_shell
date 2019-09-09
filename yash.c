#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>

// Directions here:
// TODO: https://docs.google.com/document/d/1LBMJslvYvw59uZ_8DNiiPzsp0heW3qesaalOo31IGYg/edit

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
  if(strlen(input) > maxLineLen + 1){
    return 0;
  }
  if(!checkTokens(input, maxLineLen, maxTokenLen)){
    return 0;
  }
  return 1;
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
char** createTokenArray(char* input, int numTokens, int maxLineLen, int maxTokenLen){
  // strtok inserts null terminators in space delimiters
  // Remove this copy if input will not be used again
  char* inputCopy = (char*) malloc(maxLineLen * sizeof(char));
  strcpy(inputCopy, input);
  
  int index = 0;
  char* delimiter = " ";
  char** tokenArray = (char**)calloc(numTokens, sizeof(char*));
  char* arrayEntry = (char*)malloc(maxTokenLen * sizeof(char));
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

// see fig 5.4 in OSTEP
// TODO: Rewrite this stub to be used when file redirection is used
#include <fcntl.h>
#include <sys/stat.h>
int executeFileRedirect(int argc, char *argv[]) {
  int rc = fork();
  if(rc < 0){
    // fork failed; exit
    fprintf(stderr, "fork failed\n");
    exit(1);
  }
  else if(rc == 0){ // child: redirect standard output to a file
    close(STDOUT_FILENO);
    open("./p4.output", O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU);
    // now exec "wc"...
    char *myargs[3];
    myargs[0] = strdup("wc");
    myargs[1] = strdup("p4.c");
    myargs[2] = NULL;
    execvp(myargs[0], myargs);
  }
  else{
    // parent
    int rc_wait = wait(NULL);
  }
  return 0;
  // program: "wc" (word count)
  // argument: file to count
  // marks end of array
  // runs word count
}
// TODO: Write execute function that handles piped i/o
// TODO: Write execute handler function that decides which execute function to use,
//       consider using indices of tokenArray

/**
 * Purpose:
 *   Execute general input commands from command line
 * 
 * Args:
 *   tokenArray (char**): Array of tokens from input line
 * 
 * Returns:
 *   None 
 */
void executeGeneral(char** tokenArray){
  const char NEW_LINE = '\n';
  int child = fork();
  if (child < 0) {
    // fork failed; exit
    fprintf(stderr, "fork failed\n");
    exit(1);
  }
  else if (child == 0) {
    // child (new process)
    execvp(tokenArray[0], tokenArray);

    // print new line if execvp fails (==-1) and exit process
    // these lines will not run unless execvp has failed
    printf("%c", NEW_LINE);
    exit(1);
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
 *     * bg and fg will always be at tokenArray[0]
 *     * & will always be at tokenArray[numTokens - 1]
 * 
 * Args:
 *   tokenArray (char**): Array of tokens from input line
 *   numTokens     (int): Number of tokens in string
 * 
 * Returns:
 *   None
 */
void checkJobControl(char** tokenArray, int numTokens){
  const char* BACKGROUND = "&";
  const char* BG_TOKEN = "bg";
  const char* FG_TOKEN = "fg";
  
  if(tokenArray[0] == BG_TOKEN){
    // execute bg
  }
  else if(tokenArray[0] == FG_TOKEN){
    // execute fg
  }
  else if(tokenArray[numTokens - 1] == BACKGROUND){
    // execute background
  }
  else{
    executeGeneral(tokenArray);
  }
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
      char** tokenArray = createTokenArray(input, numTokens, MAX_LINE_LEN, MAX_TOKEN_LEN);
      // execute(tokenArray, numTokens, MAX_TOKEN_LEN);
      executeGeneral(tokenArray);

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
  
  return 0;
}
