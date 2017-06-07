#include "LinkedList.hpp"

bool insertCoord(cl_int x, cl_int y, LinkedList* list)
{
	if (x < 0 || y < 0)
		return false;

	Coord* newNode = new Coord;
	newNode->x = x;
	newNode->y = y;

	if (list->head == NULL)
		newNode->next = NULL;
    else
		newNode->next = list->head;


	list->head = newNode;
	list->curr = list->head;

	list->length++;
	return true;
}

Coord removeCoord(LinkedList* list)
{
	if (list->head == NULL){
		Coord coord;
		coord.x = -1;
		coord.y = -1;
		coord.next = NULL;
		return coord;
	}

	Coord pop = *list->head;
	Coord* removed = list->head;
	pop.next = NULL;

	if (list->head->next != NULL){
		list->head = list->head->next;
		list->curr = list->head;
	}else{
		list->head = NULL;
		list->curr = NULL;
	}

	list->length--;
	delete removed;
	return pop;
}

Coord seek(LinkedList* list)
{
	if (list->curr == NULL){
		Coord coord;
		coord.x = -1;
		coord.y = -1;
		coord.next = NULL;
		return coord;
	}

	Coord seek = *list->curr;
	seek.next = NULL;

	if (list->curr->next != NULL)
		list->curr = list->curr->next;
	else
		list->curr = list->head;

	return seek;
}

// Prints each node in the list in consecutive order,
// starting at the head and ending at the tail.
// Prints the data to the console.
int getListLength(LinkedList* list)
{
   return list->length;
}





















//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/*LinkedList::LinkedList()
{
    head = NULL;
    curr = NULL; // This stores a currently selected node for processing. Resets to head if list size is modified.
    length = 0;
}


bool LinkedList::insertCoord(int x, int y)
{
	if (x < 0 || y < 0)
		return false;

	Coord* newNode = new Coord;
	newNode->x = x;
	newNode->y = y;

	if (head == NULL)
		newNode->next = NULL;
    else
		newNode->next = head;


	head = newNode;
	curr = head;

	length++;
	return true;
}

Coord LinkedList::removeCoord()
{
	if (head == NULL){
		Coord coord;
		coord.x = -1;
		coord.y = -1;
		coord.next = NULL;
		return coord;
	}

	Coord pop = *head;
	Coord* removed = head;
	pop.next = NULL;

	if (head->next != NULL){
		head = head->next;
		curr = head;
	}else{
		head = NULL;
		curr = NULL;
	}

	length--;
	delete removed;
	return pop;
}

Coord LinkedList::seek()
{
	if (curr == NULL){
		Coord coord;
		coord.x = -1;
		coord.y = -1;
		coord.next = NULL;
		return coord;
	}

	Coord seek = *curr;
	seek.next = NULL;

	if (curr->next != NULL)
		curr = curr->next;
	else
		curr = head;

	return seek;
}

// Prints each node in the list in consecutive order,
// starting at the head and ending at the tail.
// Prints the data to the console.
int LinkedList::getListLength()
{
   return length;
}

// Destructor de-allocates memory used by the list.
LinkedList::~LinkedList()
{
    while (head != NULL)
       removeCoord();
}*/
