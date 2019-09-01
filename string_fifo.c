/**
 * string_fifo.c
 * February 25, 2019
 * Created by: Edie Zhou
 * This file allows a driver to create and use a fifo queues for C-strings
 */

#include <stdio.h>
#include <stdlib.h>

/**
 * Purpose: Struct for FIFO queue node
 * *next: Pointer to next node in queue
 */
typedef struct QNode {
  tokenSize = 30;
  char* token;
  struct QNode *next;
} QNode;

/**
 * Queue data structure
 * *top: Pointer to top of queue
 */
typedef struct Queue {
  struct QNode *top;
} Queue;

/**
 * Purpose:
 *   Create queue in heap
 * 
 * Args:
 *   queue (Queue*): Pointer to head of queue
 * 
 * Returns:
 *   None
 */
void makeQueue(Queue* queue){
  queue->top = NULL;
}

/**@Precondition Accepts a node as input parameter
 * @Postcondition Checks if stack is full and returns boolean result
 * @param s Head of linked list
 * @return Boolean result checking if stack is full
 */
int isFull(Queue s){
  return 0;
}

/**
 * Purpose:
 *   Check if queue is empty
 * 
 * Args:
 *   queue (Queue): Queue object to check for emptiness
 * 
 * Returns:
 *   (int): 1 if empty, 0 if not empty
 */
int isEmpty(Queue queue){
  int result = 0;
  if(queue.top == NULL){
    result = 1;
  }
  return result;
}

/**
 * Purpose:
 *   Create and push first node to queue
 * 
 * Args:
 *   node    (QNode): Node to be pushed to queue
 *   queue  (Queue*): Pointer to queue object
 *   cString (char*): Pointer to C-string
 * 
 * Returns:
 *   None
 */
void pushFirst(QNode node, Queue* queue, char* cString){
  struct StackNode* new = (struct StackNode*) malloc(sizeof(StackNode));
  new->token = cString;
  new->next = s->top;
  s->top = new;
}

/**
 * Purpose:
 *   Create and push nodes other than first node to queue
 * 
 * Args:
 *   node    (QNode): Node to be pushed to queue
 *   queue  (Queue*): Pointer to queue object
 *   cString (char*): Pointer to C-string
 * 
 * Returns:
 *   None
 */
void push(QNode node, Queue* queue, char* cString){
  struct StackNode* new = (struct StackNode*) malloc(sizeof(StackNode));
  new->token = cString;
  new->next = NULL;
  QNode* nodePtr = queue->top;
  while(nodePtr->next != NULL){
    nodePtr = nodePtr->next;
  }
  nodePtr->next = new;
}

/**
 * Purpose:
 *   Remove and return node from queue
 * 
 * Args:
 *   queue (Queue*): Pointer to queue
 * 
 * Returns
 *   (char*): Pointer to C-string
 */
QNode pop(Queue* queue){
  QNode entry;
  Queue* temp;
  temp = queue->top;
  queue->top = temp->next;
  entry = temp->pixel;
  free(temp);
  return entry->token;
}
