/* 
 * list.c --
 *
 *	Source code for the List library procedures.
 *
 * Copyright 1988 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */



#include <stdlib.h>
#include "list.h"

typedef int ReturnStatus;
const int FAILURE = -1;
const int SUCCESS = 0;

/*
 * ----------------------------------------------------------------------------
 *
 * List_Init --
 *
 *	Initialize a header pointer to point to an empty list.  The List_Links
 *	structure must already be allocated.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The header's pointers are modified to point to itself.
 *
 * ----------------------------------------------------------------------------
 */
void
List_Init(headerPtr)
    register List_Links *headerPtr;  /* Pointer to a List_Links structure 
					to be header */
{
    if (headerPtr == (List_Links *) LIST_NIL || !headerPtr) {
	panic2(("List_Init: invalid header pointer.\n"));
    }
    headerPtr->nextPtr = headerPtr;
    headerPtr->prevPtr = headerPtr;
}


/*
 * ----------------------------------------------------------------------------
 *
 * List_Insert --
 *
 *	Insert the list element pointed to by itemPtr into a List after 
 *	destPtr.  Perform a primitive test for self-looping by returning
 *	failure if the list element is being inserted next to itself.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The list containing destPtr is modified to contain itemPtr.
 *
 * ----------------------------------------------------------------------------
 */
void
List_Insert(itemPtr, destPtr)
    register	List_Links *itemPtr;	/* structure to insert */
    register	List_Links *destPtr;	/* structure after which to insert it */
{
    if (itemPtr == (List_Links *) LIST_NIL || destPtr == (List_Links *) LIST_NIL
	    || !itemPtr || !destPtr) {
	panic2(("List_Insert: itemPtr (%x) or destPtr (%x) is LIST_NIL.\n",
		  (unsigned int) itemPtr, (unsigned int) destPtr));
	return;
    }
    if (itemPtr == destPtr) {
	panic2(("List_Insert: trying to insert something after itself.\n"));
	return;
    }
    
    itemPtr->nextPtr = destPtr->nextPtr;
    itemPtr->prevPtr = destPtr;
    destPtr->nextPtr->prevPtr = itemPtr;
    destPtr->nextPtr = itemPtr;
}


/*
 * ----------------------------------------------------------------------------
 *
 * List_ListInsert --
 *
 *	Insert the list pointed to by headerPtr into a List after 
 *	destPtr.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The list containing destPtr is modified to contain itemPtr.
 *	headerPtr no longer references a valid list.
 *
 * ----------------------------------------------------------------------------
 */
void
List_ListInsert(headerPtr, destPtr)
    register	List_Links *headerPtr;	/* structure to insert */
    register	List_Links *destPtr;	/* structure after which to insert it */
{
    if (headerPtr == (List_Links *) LIST_NIL || destPtr == (List_Links *) LIST_NIL
	    || !headerPtr || !destPtr) {
	panic2(("List_ListInsert: headerPtr (%x) or destPtr (%x) is NIL.\n",
		  (unsigned int) headerPtr, (unsigned int) destPtr));
	return;
    }

    if (headerPtr->nextPtr != headerPtr) {
	headerPtr->prevPtr->nextPtr = destPtr->nextPtr;
	headerPtr->nextPtr->prevPtr = destPtr;
	destPtr->nextPtr->prevPtr = headerPtr->prevPtr;
	destPtr->nextPtr = headerPtr->nextPtr;
    }

    headerPtr->nextPtr = (List_Links *) LIST_NIL;
    headerPtr->prevPtr = (List_Links *) LIST_NIL;
}


/*
 * ----------------------------------------------------------------------------
 *
 * List_Move --
 *
 *	Move the list element referenced by itemPtr to follow destPtr.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	List ordering is modified.
 *
 * ----------------------------------------------------------------------------
 */
void
List_Move(itemPtr, destPtr)
    register List_Links *itemPtr; /* list element to be moved */
    register List_Links *destPtr; /* element after which it is to be placed */
{
    if (itemPtr == (List_Links *) LIST_NIL || destPtr == (List_Links *) LIST_NIL
	    || !itemPtr || !destPtr) {
	panic2(("List_Move: One of the list items is LIST_NIL.\n"));
    }
    /*
     * It is conceivable that someone will try to move a list element to
     * be after itself.
     */
    if (itemPtr != destPtr) {
	/*
	 * Remove the item.
	 */
        itemPtr->prevPtr->nextPtr = itemPtr->nextPtr;
	itemPtr->nextPtr->prevPtr = itemPtr->prevPtr;
	/*
	 * Insert the item at its new place.
	 */
	itemPtr->nextPtr = destPtr->nextPtr;
	itemPtr->prevPtr = destPtr;
	destPtr->nextPtr->prevPtr = itemPtr;
	destPtr->nextPtr = itemPtr;
    }    
}


/*
 * ----------------------------------------------------------------------------
 *
 * List_Remove --
 *
 *	Remove a list element from the list in which it is contained.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The given structure is removed from its containing list.
 *
 * ----------------------------------------------------------------------------
 */
void
List_Remove(itemPtr)
    register	List_Links *itemPtr;	/* list element to remove */
{
    if (itemPtr == (List_Links *) LIST_NIL || !itemPtr ||
	itemPtr == itemPtr->nextPtr) {
	panic2(("List_Remove: invalid item to remove.\n"));
    }
    if (itemPtr->prevPtr->nextPtr != itemPtr ||
	itemPtr->nextPtr->prevPtr != itemPtr) {
	panic2(("List_Remove: item's pointers are invalid.\n"));
    }
    itemPtr->prevPtr->nextPtr = itemPtr->nextPtr;
    itemPtr->nextPtr->prevPtr = itemPtr->prevPtr;
    itemPtr->prevPtr = (List_Links *)LIST_NIL;
    itemPtr->nextPtr = (List_Links *)LIST_NIL;
}


/*
 * at least one item in headp, namely itemp
 */
void
List_Break (headp, itemp, newheadp)
    register List_Links *headp;
    register List_Links *itemp;
    register List_Links *newheadp;
{
    register List_Links *prevp;
    register List_Links *tailp;

    prevp = itemp->prevPtr;
    tailp = headp->prevPtr;

    prevp->nextPtr = headp;
    headp->prevPtr = prevp;

    tailp->nextPtr = newheadp;
    newheadp->prevPtr = tailp;
    
    newheadp->nextPtr = itemp;
    itemp->prevPtr = newheadp;
}


/*
 *----------------------------------------------------------------------
 *
 * List_Verify --
 *
 *	Verifies that the given list isn't corrupted. The descriptionPtr
 *	should be at least 80 characters.
 *
 * Results:
 *	SUCCESS if the list looks ok, FAILURE otherwise. If FAILURE is
 *	returned then a description is returned in the descriptionPtr
 *	array.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

ReturnStatus
List_Verify(headerPtr, descriptionPtr)
    List_Links	*headerPtr; 	/* Header of list to check. */
    char	*descriptionPtr;/* Description of what went wrong. */
{
    List_Links	*itemPtr;
    List_Links	*prevPtr;
    List_Links	*nextPtr;
    int		index;;

    if ((List_Prev(headerPtr) == LIST_NIL) || (List_Prev(headerPtr) == NULL)) {
	sprintf(descriptionPtr,
	    "Header prevPtr is bogus: 0x%x.\n", (int) List_Prev(headerPtr));
	return FAILURE;
    }
    if ((List_Next(headerPtr) == LIST_NIL) || (List_Next(headerPtr) == NULL)) {
	sprintf(descriptionPtr, 
	    "Header nextPtr is bogus: 0x%x.\n", (int) List_Next(headerPtr));
	return FAILURE;
    }
    itemPtr = List_First(headerPtr);
    prevPtr = headerPtr;
    index = 1;
    while(List_IsAtEnd(headerPtr, itemPtr)) {
	if (List_Prev(itemPtr) != prevPtr) {
	    sprintf(descriptionPtr, 
		"Item %d doesn't point back at previous item.\n", index);
	    return FAILURE;
	}
	nextPtr = List_Next(itemPtr);
	if ((nextPtr == LIST_NIL) || (nextPtr == NULL)) {
	    sprintf(descriptionPtr, 
		"Item %d nextPtr is bogus: 0x%x.\n", index, (int) nextPtr);
	    return FAILURE;
	}
	if (List_Prev(nextPtr) != itemPtr) {
	    sprintf(descriptionPtr,
		"Next item doesn't point back at item %d.\n", index);
	    return FAILURE;
	}
	prevPtr = itemPtr;
	itemPtr = nextPtr;
	index++;
    }
    return SUCCESS;
}
