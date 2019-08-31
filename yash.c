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
  const char SPACE = ' ';
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
 *   Parse input line and check if commands are valid
 * 
 * Args:
 *   input (char[]): Pointer to input c-string
 *  
 * Returns:
 *   (int): Returns 1 is string is valid, 0 if string is invalid
 * 
 */
int parse (char input[]){
  int k = 0;
  const char LINE_FEED = '\n';
  while(input[k] != LINE_FEED){
    printf("%c", input[k]);
    k++;
  }
  return 1;
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
  const char SPACE = ' ';
  const char YASH_ACTIVE = '#';
  const int MAX_LINE_LEN = 2000;
  const int MAX_TOKEN_LEN = 30;
  int validInput;
  int validParse;
  int shellRunning = 1;
  char* input = (char*)malloc(MAX_LINE_LEN * sizeof(char));
  
  while(shellRunning){
    printf("%c", YASH_ACTIVE);
    printf("%c", SPACE);
    validInput = checkInput(input, MAX_LINE_LEN, MAX_TOKEN_LEN);
    
    if(validInput){
      validParse = parse(input);
    }

    if(validInput && validParse){
      printf("%c", 'x');
      printf("%c", '\n');
      shellRunning = 1;
    }
    else{
      printf("%c", NEW_LINE);
    }
  }

  free(input); 
  return 0;
}

