#ifndef LinkedList_hpp
#define LinkedList_hpp

//#include <iostream>
#include <CL\cl.hpp>
#include <CL\opencl.h>
//#include <string>
using namespace std;

struct Coord {
    cl_int x;
    cl_int y;
    Coord* next;
};

struct LinkedList
{
    Coord* head = NULL;
    Coord* curr = NULL; // This stores a currently selected node for processing. Resets to head if list size is modified.
    cl_int length = 0;
};

bool insertCoord(int, int, LinkedList*);

Coord removeCoord(LinkedList*);

Coord seek(LinkedList*);

int getListLength(LinkedList*);



/*
// Was forced to change LinkedList class to a struct because OpenCL only supports C99 standard (doesn't support classes)
class LinkedList
{
	private:

		Coord* head;
		Coord* curr;
		int length;

	public:

		LinkedList();

		bool insertCoord(int, int);

		Coord removeCoord();

		Coord seek();

		int getListLength();

    ~LinkedList();
};*/


#endif
