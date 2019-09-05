#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>


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
 *   input      (char[]): Input C-string
 *   maxLineLen    (int): Maximum length of input
 *   maxTokenLen   (int): Maximum length of tokens
 * 
 * Returns:
 *   (int): Returns 1 if string is valid, 0 if string is invalid
 */
int checkInput(char input[], int maxLineLen, int maxTokenLen){
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
 *   input (char[]): Pointer to input c-string
 * 
 * Returns:
 *   (int): Number of tokens in input line
 */
int countTokens(char input[]){
  char delimiters[1] = " ";
  char* token = strtok(input, delimiters);
  int numTokens = 1;
  while(token != NULL){
    token = strtok(NULL, delimiters);
    numTokens++;
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
 *   (char**): Returns array of token c-strings
 * 
 */
char** parseInput(char input[]){
  char delimiter[] = " ";
  int index = 0;
  int tokenArraySize = countTokens(input);
  char** tokenArray = (char**) calloc(tokenArraySize, sizeof(char*));
  char* token = strtok(input, delimiter);
  tokenArray[index] = token;
  while(token != NULL){
    printf("BUT\n");
    printf("%s\n", token);
    index++;
    tokenArray[index] = token;
    token = strtok(NULL, delimiter);
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

/**
 * Purpose:
 *   Driver for shell program
 * 
 * Args:
 *   None
 * 
 * Returns:
 *   (int): 0
 */
int main (void){
  const char NEW_LINE = '\n';
  const int MAX_LINE_LEN = 2000;
  const int MAX_TOKEN_LEN = 30;
  int validInput = 0;
  char* prompt = "# ";
  char* input;
  
  while(input = readline(prompt)){
    validInput = checkInput(input, MAX_LINE_LEN, MAX_TOKEN_LEN);
    if(validInput){
      printf("\nTokens: %d\n", countTokens(input));
      char** tokenArray = parseInput(input);
      // printf("FUCK\n");
      // int numTokens = countTokens(input);
      // for(int k = 0; k < numTokens; k++){
      //   printf("%d %s\n", k, tokenArray[k]);
      // }
      // printf("FUCK");
      printf("%s", "\nexecute\n");
      free(tokenArray);
    }
    else{
      printf("%c", NEW_LINE);
    }

  }

  free(input); 
  
  return 0;
}
