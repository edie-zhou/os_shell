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
 *   input      (char[]): Pointer to input c-string
 *   maxLineLen (int): Maximum length of tokens
 * 
 * Returns:
 *   (int): Number of tokens in input line
 */
int countTokens(char input[], int maxLineLen){
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
 *   Parse input line and check if commands are valid
 * 
 * Args:
 *   input       (char[]): Pointer to input c-string
 *   numTokens   (int): Number of tokens in string
 *   maxLineLen  (int): Maximum length of input
 *   maxTokenLen (int): Maximum length of tokens
 *  
 * Returns:
 *   (char**): Returns array of token c-strings
 * 
 */
char** parseInput(char input[], int numTokens, int maxLineLen, int maxTokenLen){
  // strtok inserts null terminators in space delimiters
  // Remove this copy if input will not be used again
  char* inputCopy = (char*) malloc(maxLineLen * sizeof(char));
  strcpy(inputCopy, input);
  
  int index = 0;
  char* delimiter = " ";
  char** tokenArray = (char**)calloc(numTokens, sizeof(char*));

  // TODO: Look into refactoring this part of the code, possibly using a do-while loop
  char* token = strtok(inputCopy, delimiter);
  char* arrayEntry = (char*)malloc(maxTokenLen * sizeof(char));
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
      char** tokenArray = parseInput(input, numTokens, MAX_LINE_LEN, MAX_TOKEN_LEN);
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
