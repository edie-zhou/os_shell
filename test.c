#include <stdio.h>

int main (void){
  printf("Hello, world!\n");

  char line[2000];

  if(fgets(line, sizeof line, stdin) != NULL){
    /* Now inspect and further parse the string in line. */
    int k = 0;
    char nullTerm = '\0';
    while(line[k] != nullTerm){
      printf("%c", line[k]);
      k++;
    }
  }
  return 0;
}

