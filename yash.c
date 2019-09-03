#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Purpose:
 *   Verify that tokens in input line do not exceed maximum token length
 * 
 * Args:
 *   input      (char[]): Input C-string
 *   maxLineLen    (int): Maximum length of input
 *   maxTokenLen   (int): Maximum length of tokens
 * 
 * Returns:
 *   (int): Returns 1 if all tokens are within maximum limit
 */
int checkTokens(char input[], int maxLineLen, int maxTokenLen){
  const char LINE_FEED = '\n';
  const char SPACE = ' ';
  int tokenLen = 0;
  int k = 0;
  while((input[k] != LINE_FEED) && (k < maxLineLen)){
    if((tokenLen < 30) && (input[k] == SPACE)){
      tokenLen = 0;
    }
    else if(tokenLen >= maxTokenLen){
      return 0;
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
 *   input      (char[]): Input C-string
 *   maxLineLen    (int): Maximum length of input
 *   maxTokenLen   (int): Maximum length of tokens
 * 
 * Returns:
 *   (int): Returns 1 if string is valid, 0 if string is invalid
 */
int checkInput(char input[], int maxLineLen, int maxTokenLen){
  const char LINE_FEED = '\n';
  char line[maxLineLen];
  if(fgets(line, sizeof line, stdin) != NULL){
    // Check token length
    if(!checkTokens(line, maxLineLen, maxTokenLen)){
      return 0;
    }

    // Write to memory
    int k = 0;
    while((line[k] != LINE_FEED) && (k < maxLineLen)){
      strcpy(input, line);
      k++;
    }
    printf("%c", line[k]);
    return 1;
  }
  else{
    return 0;
  }
}

/**
 * Purpose:
 *   Count number of tokens in input line
 * 
 * Args:
 *   input (char[]): Pointer to input c-string
 * 
 * Returns:
 *   (int): Number of tokens in input line
 */
int countTokens(char input[]){
  char delimiters[1] = " ";
  int numTokens = 0;
  char* token = strtok (input, delimiters);
  while(token != NULL){
    // printf ("%s\n", token);
    numTokens++;
    token = strtok (NULL, delimiters);
  }
  return numTokens;
} 

/**
 * Purpose:
 *   Parse input line and check if commands are valid
 * 
 * Args:
 *   input (char[]): Pointer to input c-string
 *  
 * Returns:
 *   (int): Returns 1 is string is valid, 0 if string is invalid
 * 
 */
// TODO: Rewrite parse to return a pointer to array of C-strings
// A linked list could also be a possible solution, but would require more
// effort
char** parseInput(char input[], int maxTokenLen){
  char delimiters[1] = " ";
  int index = 0;
  int tokenArraySize = countTokens(input);
  char** tokenArray = (char**) calloc(tokenArraySize+1, sizeof(char*));
  char* token = strtok (input, delimiters);
  tokenArray[index] = token;
  while(token != NULL){
    index++;
    token = strtok (NULL, delimiters);
    tokenArray[index] = token;
  }
  return tokenArray;
}

/**
 * Purpose:
 *   Execute input commands from command line
 * 
 * Args:
 *   input (char[]): Input C-string
 * 
 * Returns:
 * 
 */
void execute(char* input[]){

}

int main (void){
  const char NEW_LINE = '\n';
  const int MAX_LINE_LEN = 2000;
  const int MAX_TOKEN_LEN = 30;
  int validInput;
  int shellRunning = 1;
  char* input = (char*)malloc(MAX_LINE_LEN * sizeof(char));
  
  while(shellRunning){
    printf("# ");
    validInput = checkInput(input, MAX_LINE_LEN, MAX_TOKEN_LEN);
    
    if(validInput){
      // char** tokenArray = parseInput(input, MAX_TOKEN_LEN);
      printf("%d\n", countTokens(input));
      char** tokenArray = parseInput(input, MAX_TOKEN_LEN);
      printf("%s", tokenArray[0]);
      printf("%s", "\nexecuted\n");
      shellRunning = 1;
      free(tokenArray);
    }
    else{
      printf("%c", NEW_LINE);
    }
  }

  free(input); 
  
  return 0;
}
