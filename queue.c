#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "queue.h"
#include <errno.h>
#include <assert.h>
#include <stdio.h>

//we are creating a linked list implementation of a queue. 

//struct node holds a data item, and the queue is made up of multiple nodes
//linked together.
struct node
{  
	void* data;
	struct node* next;
};

typedef struct node* node_t;

//struct queue consists of nodes indicating the beginning and end of the 
//queue
struct queue {
	node_t head;
	node_t tail;
	int length;
};

typedef struct queue* queue_t; 

//create_node will create a node, and populate it with @void *data
node_t create_node(void* data)
{
	node_t new_node = (node_t)malloc(sizeof(struct node)); 
	new_node->data = data;
	new_node->next = NULL;
	return new_node;
}

//queue_create creates the queue, and initializes the head and tail.
queue_t queue_create(void)
{
	queue_t myqueue = (queue_t)malloc(sizeof(struct queue)); 
	myqueue->head = NULL;
	myqueue->tail = NULL;
	myqueue->length = 0;
	return myqueue;
}
//queue_destroy frees the queue, only if it is empty.
int queue_destroy(queue_t queue)
{
	if (queue->head == NULL || queue->head->data == NULL || queue == NULL)  
	{
		free(queue);
    	return 0;
	}
	else
	{
		return -1;
	}	
}
//queue_enqueue creates a new node with the data item provided, and places
//the node at the end of the queue; the tail.
int queue_enqueue(queue_t queue, void *data)
{	
	node_t temp_node = create_node(data);
	if(create_node(data)==NULL)
	{
		return -1;
	}

	if (queue->tail == NULL|| queue->head == NULL)
    {
    	queue->head = temp_node;
		queue->tail = temp_node;
	}
	else
	{
		queue->tail->next = temp_node;
		queue->tail = temp_node;
	}
	if (queue->head == NULL || queue->head->data == NULL || queue == NULL)
	{
		return -1;
	}  

	queue->length += 1; //incrementing length
	return 0;
}
//queue_dequeue dequeues from the front of the queue; the head.Then it places 
//the item into @void **data. 
int queue_dequeue(queue_t queue, void **data)
{
	if(queue->head == NULL || queue->head->data == NULL || queue == NULL)
	{
    	return -1;
	}
	node_t popped_node = queue->head;
	*data = popped_node->data;
	queue->head = queue->head->next;
	free(popped_node);

	queue->length -= 1; //decrementing length
	if(queue->head == NULL)
	{
    	queue->tail = NULL;
	}
	return 0;
}

//queue_delete iterates through the queue and deletes the first instance of
//a node containing @void *data from the queue. 
int queue_delete(queue_t queue, void *data)
{
	if (queue->head == NULL || queue->head->data == NULL || queue == NULL)
	{
    	return -1;
	}

	if(queue->head->data == data)
	{	
		node_t temp_node = queue->head->next;
		free(queue->head);
		queue->head = temp_node;
		temp_node = NULL;
		queue->length -=1;//decrementing length
		return 0;	
	}

	node_t previous = queue->head;
	node_t current = queue->head->next;

	while(current != NULL)
	{	
		if(current->data == data)
		{	
			previous->next = current->next;
			current->next = NULL;
			free(current); //deletes the node from the queue 
			queue->length -= 1;//decrementing length
			current = previous->next;
			return 0;	
		}

		current = current->next;
	
	}
	return -1;

}

//queue_length returns the length of the queue
int queue_length(queue_t queue)
{
	if (queue->head == NULL || queue->head->data == NULL || queue == NULL)
	{
    	return -1;
	}
	return queue->length;	
}

//queue_iterate iterates through the queue, and calls a function @func, with
//@arg an argument when applicable. If the function @func returns 1,
//we store that particular nodes data into @void **data.
int queue_iterate(queue_t queue, queue_func_t func, void *arg, void **data)
{	
	int iterator =0;
	if (queue->head == NULL || queue->head->data == NULL || queue == NULL)
	{
    	return -1;
	}
	node_t current_node = queue->head;
	while(iterator < queue_length(queue))
	{	
		if(func(current_node->data, arg) == 1)
		{	
			if(data!= NULL)
			{	
				*data = current_node->data;
			}
			return 0;
		}
		current_node = current_node->next;	
		iterator++;
	}
	return 0;
}

