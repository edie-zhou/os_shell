#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Purpose:
 *   Read in a line of input from stdin and verify that input line is valid
 * 
 * Args:
 *   input      (char*): Pointer to input c-string
 *   maxInputLen  (int): Maximum length of input
 * 
 * Returns:
 *   (int): Returns 1 if string is valid, 0 if string is invalid
 */
int read(char input[], int maxInputLen){
  const char LINE_FEED = 0x0A;
  char line[maxInputLen];
  if(fgets(line, sizeof line, stdin) != NULL){
    int k = 0;
    while((line[k] != LINE_FEED) && (k < maxInputLen)){
      strcpy(input, line);
      k++;
    }
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
 *   input (char*): Pointer to input c-string
 *  
 * Returns:
 *   (int): Returns 1 is string is valid, 0 if string is invalid
 * 
 */
int parse (char input[]){
  int k = 0;
  const char LINE_FEED = 0x0A;
  while(input[k] != LINE_FEED){
    printf("%c", input[k]);
    k++;
  }
}

/**
 * Purpose:
 *   Execute input commands from command line
 * 
 * Args:
 *   input (char*): Pointer to input c-string
 * 
 * Returns:
 * 
 */
void execute(char* input[]){

}

int main (void){
  const char NEW_LINE = 0x0A;
  const int MAX_LINE_LEN = 2000;
  int validInput;
  int validParse;
  char* input = (char*)malloc(MAX_LINE_LEN * sizeof(char));
  validInput = read(input, MAX_LINE_LEN);
  validParse = parse(input);
  
  free(input); 
  return 0;
}

