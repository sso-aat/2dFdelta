/*+           T D F D E L T A

 *  Module name:
      tdFdeltaCrosses

 *  Function:
      Manage the linked lists containing details of fibre crossovers.

 *  Description:
      Each fibre (pivot) has a singly linked list containing the pivot numbers of
      any fibre crossing above and below it. This file contains the functions used
      to manage these linked lists - namely ADD, DELETE, and SEARCH.

 *  Language:
      C

 *  Support: James Wilcox, AAO

 *-

 *  History:
      01-Jul-1994  JW   Original version
      {@change entry@}

 *  @(#) $Id: ACMM:2dFdelta/tdFdelCrosses.c,v 3.14 22-Feb-2013 08:50:01+11 tjf $ (mm/dd/yy)
 */

/*
 *  Include files.
 */


static char *rcsId="@(#) $Id: ACMM:2dFdelta/tdFdelCrosses.c,v 3.14 22-Feb-2013 08:50:01+11 tjf $";
static void *use_rcsId = (0 ? (void *)(&use_rcsId) : (void *) &rcsId);


#include "tdFdelta.h"
#include "tdFdelta_Err.h"
#include "status.h"        /* STATUS__OK definition */

#include <stdio.h>

#include <stdlib.h>


/*
 *+           T D F D E L T A C R O S S E S

 *  Function name:
      tdFdeltaAddCross

 *  Function:
      Creates and adds a new item to the start of the crossover list.

 *  Description:

 *  Language:
      C

 *  Call:
      (Void) = tdFdeltaAddCross (newCross,start,status)

 *  Parameters:   (">" input, "!" modified, "W" workspace, "<" output)
      (>) newCross      (int)           Number of pivot to be added to list
      (>) start         (FibreCross **) Pointer to start of list
      (!) status        (StatusType *)  Modified status.

 *  Prior requirements:

 *  Support: James Wilcox, AAO

 *-

 *  History:
      01-Jul-1994  JW   Original version
      {@change entry@}
 */
TDFDELTA_INTERNAL void  tdFdeltaAddCross (
        int         newCross,  /* Number of pivot to be added to list */
        FibreCross  **start,   /* Pointer to start of list            */
        StatusType  *status)
{
    FibreCross  *p, *new;

    if (*status != STATUS__OK) return;

    /*
     *  Create new item.
     */
    if ((new = (FibreCross *)malloc(sizeof(FibreCross))) == NULL)
        *status = TDFDELTA__MALLOCERR;

    /*
     *  Add item to list.
     */
    else {
        new->piv=newCross;
        p = *start;
        new->next = (p)?  p: NULL;
        *start = new;
    }
}


/*
 *+           T D F D E L T A C R O S S E S

 *  Function name:
      tdFdeltaDeleteCross

 *  Function:
      Deletes an item from the crossover list.

 *  Description:
      Searches the list for the pivot specified, and if found
      will delete it and free the memory that it occupied.

 *  Language:
      C

 *  Call:
      (void) = tdFdeltaDeleteCross (cross,start,status)

 *  Parameters:   (">" input, "!" modified, "W" workspace, "<" output)
      (>) cross         (int)           Number of pivot to be deleted
      (>) start         (FibreCross **) Pointer to start of list
      (!) status        (StatusType *)  Modified status.

 *  Prior requirements:

 *  Support: James Wilcox, AAO

 *-

 *  History:
      01-Jul-1994  JW   Original version
      {@change entry@}
 */
TDFDELTA_INTERNAL void  tdFdeltaDeleteCross (
        int         cross,    /* pivot number of item to be deleted */
        FibreCross  **start,  /* pointer to start of list           */
        StatusType  *status)
{
    FibreCross  *previous, *current;

    if (*status != STATUS__OK) return;

    current = previous = *start;

    /*
     *  If item is the first in the list just increment the start pointer.
     */
    if (current->piv==cross) {
        *start = current->next;
        free((void *)current);
        return;
    }

    /*
     *  Otherwise search for the item, keeping pointers to current and prevoius
     *  enteries.
     */
    current = current->next;
    while (current) {
        if (current->piv==cross) {

            /*
             *  Found item - remove it. Note that last_item->next=NULL therefore
             *  we do not need to consider this case seperately.
             */
            previous->next = current->next;
            free((void *)current);
            return;
        }
        previous = current;
        current = current->next;
    }
}


/*
 *+           T D F D E L T A C R O S S E S

 *  Function name:
      tdFdeltaFindCross

 *  Function:
      Checks a crossover list for the presence of a specified pivot.

 *  Description:
      Searches the list for the pivot specified, and if found
      returns a pointer to it. If not found the function returns a NULL pointer.

 *  Language:
      C

 *  Call:
      (FibreCross *) = tdFdeltaFindCross (cross,start,status)

 *  Parameters:   (">" input, "!" modified, "W" workspace, "<" output)
      (>) cross         (int)           Number of pivot to be searched for.
      (>) start         (FibreCross **) Pointer to start of list.
      (!) status        (StatusType *)  Modified status.

 *  Prior requirements:

 *  Support: James Wilcox, AAO

 *-

 *  History:
      01-Jul-1994  JW   Original version
      {@change entry@}
 */
TDFDELTA_INTERNAL FibreCross  *tdFdeltaFindCross(
        int         cross,     /* Pivot number being searched for */
        FibreCross  **start,   /* Pointer to start of list        */
        StatusType  *status)
{
    FibreCross  *p;

    if (*status != STATUS__OK) return (NULL);

    p = *start;
    while (p) {
        if (p->piv == cross)
            return p;
        p = p->next;
    }
    return NULL;
}
