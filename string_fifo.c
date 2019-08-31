/**
 * string_fifo.c
 * February 25, 2019
 * Created by: Edie Zhou
 * This file allows a driver to create and use a fifo queues for C-strings
 */

#include <stdio.h>
#include <stdlib.h>

/**Pixel data structure
 * row: Row number of pixel
 * col: Column number of pixel
 * color: Char that represents color of pixel
 */
typedef struct Pixel {
  int row;
  int col;
  char color;
} Pixel;

typedef Pixel StackEntry;

/**StackNode data structure
 * StackEntry: Pixel stored at node
 * *next: Pointer to next node in linked list
 */
typedef struct StackNode {
  StackEntry pixel;
  struct StackNode *next;
} StackNode;

/**Stack data structure
 * *top: Pointer to top of linked list
 */
typedef struct Stack312 {
  struct StackNode *top;
} Stack312;

/**@Precondition Accepts a pointer as input parameter
 * @Postcondition Creates a linked list in the heap
 * @param *s Pointer to head of linked list
 */
void makeStack(Stack312 *s){
  s->top = NULL;
}

/**@Precondition Accepts a node as input parameter
 * @Postcondition Checks if stack is full and returns boolean result
 * @param s Head of linked list
 * @return Boolean result checking if stack is full
 */
int isFull(Stack312 s){
  return false;
}

/**@Precondition Accepts a node as input parameter
 * @Postcondition Checks if stack is full and returns boolean result
 * @param s Head of linked list
 * @return Boolean result checking if stack is empty
 */
int isEmpty(Stack312 s){
  int result = 0;
  if(s.top == NULL){
    result = 1;
  }
  return result;
}

/**@Precondition Accepts a StackEntry and pointer as input parameters
 * @Postcondition Pushes StackEntry on to stack
 * @param e StackEntry to be pushed on to stack
 * @param *s Pointer to head of linked list
 */
void push(StackEntry e,Stack312 *s){
  struct StackNode* new = (struct StackNode*) malloc(sizeof(StackNode));
  new->pixel = e;
  new->next = s->top;
  s->top = new;
}

/**@Precondition Accepts a pointer as input parameter
 * @Postcondition Returns popped entry from stack
 * @param *s Pointer to head of linked list
 * @return StackEntry that is popped off the stack
 */
StackEntry pop(Stack312 *s){
  StackEntry entry;
  StackNode* temp;
  temp = s->top;
  s->top = temp->next;
  entry = temp->pixel;
  free(temp);
  return entry;
}
