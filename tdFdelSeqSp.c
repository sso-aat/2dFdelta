/*+                T D F D E L T A

 *  Module name:
      tdFdeltaSequencerSpecial

 *  Function:
      Responsible for the generation of the sequence of moves necessary to
      change from the current configuration to the target configuration.

      This is a special version designed for instruments like 6dF
      which must always go through park and must move the furtherest
      fibre onto the plate first and do the reverse on when parking.

 *  Description:


 *  Language:
      C

 *  Support: Tony Farrell, AAO

 *-

 *  History:
      01-Nov-2000  TJF  Original version
      {@change entry@}


 *     @(#) $Id: ACMM:2dFdelta/tdFdelSeqSp.c,v 3.4 08-Oct-2004 13:24:02+10 tjf $

 */

/*
 *  Include files
 */


static char *rcsId="@(#) $Id: ACMM:2dFdelta/tdFdelSeqSp.c,v 3.4 08-Oct-2004 13:24:02+10 tjf $";
static void *use_rcsId = ((void)&use_rcsId,(void *) &rcsId);


#include "DitsTypes.h"       /* Basic dits types            */
#include "Dits_Err.h"        /* Dits error codes            */
#include "DitsSys.h"         /* For PutActionHandlers       */
#include "DitsFix.h"         /* For various Dits routines   */
#include "DitsMsgOut.h"      /* For MsgOut                  */
#include "DitsUtil.h"        /* For DitsErrorText           */
#include "sds.h"             /* For SDS_ macros             */
#include "Sdp.h"             /* Sdp routines                */
#include "mess.h"            /* For MessPutFacility         */
#include "Git.h"             /* Git routines                */
#include "Git_Err.h"         /* GIT__ codes                 */
#include "Ers.h"

#include "tdFdelta.h"
#include "tdFdelta_Err.h"

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define SCALE      0.25   /* Used when calculating the DELTA_CONFIG parameter*/
                          /* value in an attempt to make progress v time 
                             linear            */

#define DIST_MAX   0.1   /* if the distance from center of two fibres
                            is similar within this proprotion, then
                            we can allow movements in theta in either
                            order without problems */
/*
 * This type is used to maintain an array of pivots and the distance
 * to the fibre end, when parking or moving fibres.  
 */
typedef struct { 
    unsigned int pivot;   /* Pivot index */
    double distance;      /*  Distance to fibre on plate */
    double extension;     /* Fibre extension */
} pivotDistance;



/*+        T D F D E L T A S E Q U E N C E R S P E C I A L

 *  Function name:
      tdFdeltaSequencerSpecial

 *  Function:
      Determines the sequence of moves needed to change from the current field
      configuration to the target field configuration, using the special
      6dF mode.

 *  Description:
      In 6dF, we can't move in a straight line from between two 
      points on the plate (can't do a blended move and even if we
      could, we would get an archimedes spiral unless we have a
      very special motion controller, which we don't).

      In addition, you can't carry a fibre above a button over its 
      full length  -  You can carry a button above a button, but
      if the fibre length is long then the fibre will drag over the
      top of buttons closer to the edge.

      This leads to the following delta scheme.

      1.  Park all fibres when moving between fields, since it is too
          compilcated to do much else.
      2.  When moving fibres to the plate, move the ones closest to
          the center first.  Fibres on the other side of center from
          their pivots points are considered even closer to the center.
      3.  When moving fibres from the plate, move the ones closest to
          the center last.  Fibres on the other side of center from
          their pivots points are considered even closer to the center.

     We must assume that a field was configured using this scheme.  If so,
     then most of the time we won't find cross-overs when we go to park
     a fibre, since they will be parked in the reverese order of how they
     were moved onto the plate, but.

         1.  The order of fibres at the same distance is not defined.
         2.  Even if 1 where fixed, the distance a fibre is at
             may be different after a move due to positioning 
             inaccuracy, so the relative order of two fibres at a
             similar distance may change.

         So, we must reorder our park list to account for crossovers, but
         compilain if the result puts the distance ordering a long way
         out.

     Additionally, we consider if the last fibres to be parked are the
     same as the first to be moved, and the positioner task has marked
     these fibres as not needing to be moved.  If yes, then we are probably
     restarting an interrupted configuration, and can cull this set of park
     and move operations from the list.

     There are two possible adaptions to the algrothim.  One is to reverse
     the order in which things are done (in the belief that fibres wrapping
     around buttons is not a problem as long as we don't put any down
     outside the current working area).

     The other adaption is to do the fibres with springs starting to come
     out (causing the fibre to rise) after doing the other ones.  

     The adaption of the algrothim used is determined by the extSpringOut
     items (extension at which spring is out).  If set to 0 (zero) the
     first altrothim is used.  If set to -1, the second is used.  Otherwise,
     it should be a positive interger giving the point at which the spring
     is causing the fibre to rise and we use the third altrothim.
     

 *  Language:
      C

 *  Call:
      (void) = tdFdeltaSequencerSpecial (status)

 *  Parameters:  (">" input, "!" modified, "W" workspace, "<" output)
      (!) status      (StatusType *)    Modified status.

 *  Proir Requirements:
      DitsGetActData() must return a pointer to a structure of type 
      tdFdeltaType (see tdFdelta.h for structure details).

 *  Support: Tony Farrell, AAO

 *-

 *  History:
      01-Nov-2000  TJF  Original version
      01-Mar-2001  TJF  A whole bunch of changes - supporting restarting
                        of partially configured files and fixing some
                        bugs.
      26-Mar-2001  TJF  Support different altroghims.
      15-May-2001  TJF  The order for handling spring out fibres is reversed.
 */


/*
 *  Function to update the progress parameter.
 */
static void DisplayProgress(
    const unsigned numMoves,
    const unsigned numParks,
    const unsigned pivotsLeft,
    float * const lastUpdate,
    StatusType *status)
{
    float progress;
    if (*status != STATUS__OK) return;
    /*
     *  Set the DELTA_PROG parameter - this indicates the progress of the
     *  ordering process.
     */
    progress = 100.0*(float)(numMoves+numParks)/
        ((float)(numMoves+numParks)+((float)pivotsLeft*SCALE));
    if (((*lastUpdate) - progress > RESOLUTION) ||
        (progress - (*lastUpdate) > RESOLUTION) ||
        (progress == 100.0)) {
        (*lastUpdate) = progress;
        SdpPutf("DELTA_PROG",progress,status);
    }
}

/*
 * Sort routines used by MoveSort when calling qsort().
 */
static int AscendingSortDist(const void *item1, const void *item2) {
    if (((pivotDistance *)(item1))->distance <
        ((pivotDistance *)(item2))->distance)
        return -1;
    else if (((pivotDistance *)(item1))->distance >
             ((pivotDistance *)(item2))->distance)
        return 1;
    else
        return 0;
        
}


static int DescendingSortDist(const void *item1, const void *item2) {
    if (((pivotDistance *)(item1))->distance >
        ((pivotDistance *)(item2))->distance)
        return -1;
    else if (((pivotDistance *)(item1))->distance <
             ((pivotDistance *)(item2))->distance)
        return  1;
    else
        return 0;
        
}

/*  Not used
static int AscendingSortExten(const void *item1, const void *item2) {
    if (((pivotDistance *)(item1))->extension <
        ((pivotDistance *)(item2))->extension)
        return -1;
    else if (((pivotDistance *)(item1))->extension >
             ((pivotDistance *)(item2))->extension)
        return 1;
    else
        return 0;
        
}
*/

static int DescendingSortExten(const void *item1, const void *item2) {
    if (((pivotDistance *)(item1))->extension >
        ((pivotDistance *)(item2))->extension)
        return -1;
    else if (((pivotDistance *)(item1))->extension <
             ((pivotDistance *)(item2))->extension)
        return  1;
    else
        return 0;
        
}


/*
 *  Construct the "distance" item and sort it to 
 *  decending order based on the distance of the fibre from the
 *  center and the algrothim chosen (by the extSpringOut argument)
 * 
 *  Also counts the number of items to be moved (non-zero distance)
 *  and only fills in that number of elements in the pivots array.
 *  
 */
static void MoveSort(
    const unsigned numPivots,
    const long     extSpringOut,
    const long     * const pivotX,
    const long     * const pivotY,
    const long     * const xf,
    const long     * const yf,
    const short    * const park,
    unsigned short * const numOps,
    pivotDistance  * const pivots,
    unsigned short * const numSpringOut,
    StatusType     * const status)
{
    register unsigned int i;

    if (*status != STATUS__OK) return;

    (*numOps) = 0;
    *numSpringOut = 0;
    /*
     * Create the list (assumes the pivotDistance array has
     * been allocated
     */
    for (i = 0; i < numPivots ; ++i) {
        if (!park[i])
        {
            /*
             *  We need the distance of the pivots to the center and the 
             *  fibre length.
             */
            double x_dist = xf[i] - pivotX[i];
            double y_dist = yf[i] - pivotY[i];
            /*
             * Extension is the fibre length.
             */
            pivots[*numOps].extension = hypot(x_dist, y_dist);

            /*
             * Get the distance of this fibre to the center.
             */
            pivots[*numOps].distance = hypot(xf[i], yf[i]);
            /*
             * Save pivot number and increment counter of pivots to move.
             */
            pivots[*numOps].pivot = i;
            ++(*numOps);
        }
            
    }
    /*
     */
    if ((*numOps)>1) {
        /*
         * If ths extSpringOut item is negative, then we just want
         * to treat all fibres the same, we need an ascending sort on
         * distance from the centre such  that the closest furtherest 
         * from the centre are first (to be parked)
         */
        if (extSpringOut <= -1) {
            qsort(pivots, *numOps, sizeof(pivotDistance), AscendingSortDist);
        /*
         * If ths extSpringOut item is 0, then we just want
         * to treat all fibres the same, we need a descending sort on
         * distance from the centre such  that the fibres furtherest 
         * from the centre are first (to be parked)
         */
        } else if (extSpringOut == 0) {
            qsort(pivots, *numOps, sizeof(pivotDistance), DescendingSortDist);
        } else {
            /*
             * Now extSpringOut is telling us the distance a fibre is
             * extended such that the spring is out - for this scheme,
             * we treat such fibres differently from others.
             *
             * Note, always more then two fibres in total at the point.
             */
            int lastSpringOutIndex = -1;
            /*
             * We need to separate the fibres with the springs out from
             * those without - and we need to two groups in the correct
             * order - fibres with springs out are first (to be parked).
             * So, we want an descending order sort of extension.
             */
            qsort(pivots, *numOps, sizeof(pivotDistance), DescendingSortExten);
            
            /*
             * Now we need to find last fibre with the spring out - so
             * that we can break up our two groups.
             */
            for (i = 0 ; 
                 (i < *numOps) && (pivots[i].extension > extSpringOut) ; 
                 ++i)
                lastSpringOutIndex = i;

            /*
             * If lastSpringOutIndex >= 0, then we have some fibres with their
             * springs out.  We must record both groups of fibres independently
             *
             */
            if (lastSpringOutIndex == -1) {
                /*
                 * No springs are out - just sort on descending distance
                 * from centre such  that the fibres furtherest 
                 * from the centre are first (to be parked)
                 */
                qsort(pivots, *numOps, sizeof(pivotDistance), 
                      DescendingSortDist);

            } else if (lastSpringOutIndex+1 == *numOps) {
                /*
                 * Springs are out on all fibers - just sort on ascending
                 * distance from centre such that the firbres furtherest
                 * from the centre are first (to be parked)
                 *
                 */
                qsort(pivots, *numOps, sizeof(pivotDistance), 
                      DescendingSortDist);
                *numSpringOut = *numOps;
            } else {
                /*
                 * Springs are out on some fibres.  Sort the two bits 
                 * appropriately.  We only need to do each sort if
                 * there is more then one of each type.
                 */
                int noSpringCount;
                *numSpringOut = lastSpringOutIndex + 1;
                noSpringCount  = *numOps - *numSpringOut;

                if (lastSpringOutIndex != 0)
                    qsort(pivots, *numSpringOut, 
                          sizeof(pivotDistance), DescendingSortDist);

                if (noSpringCount > 1)
                    qsort(&pivots[*numSpringOut], noSpringCount,
                          sizeof(pivotDistance), DescendingSortDist);
            }
        }
    }
    

}
    
/*
 *   Invoked to record a move.  This will only happen during the
 *   positon field part 
 */
static void RecordMove(
    const unsigned      curPivot,
    const SdsIdType     cmdFileId,
    const unsigned      numPivots,
    tdFinterim          * const current,
    tdFtarget           * const target,
    const tdFconstants  * const constants,
    tdFcrosses          * const crosses,
    int                 * const pivotsLeft,
    unsigned            * const lineNumber,
    const unsigned      numParks,
    unsigned            * const numMoves,
    float               * const lastUpdate,
    StatusType          * const status)
{
    double cosT,sinT;
    unsigned j;
    FibreCross *ptmp;

    if (*status != STATUS__OK) return;

    /*
     *  Update interim field details.
     */
    current->theta[curPivot]       = target->theta[curPivot];
    current->fibreLength[curPivot] = target->fibreLength[curPivot];
    current->fvpX[curPivot]        = target->fvpX[curPivot];
    current->fvpY[curPivot]        = target->fvpY[curPivot];
    current->xf[curPivot]          = target->xf[curPivot];
    current->yf[curPivot]          = target->yf[curPivot];
    current->park[curPivot]        = target->park[curPivot];
    target->mustMove[curPivot]     = NO;
                    
    cosT = cos(target->theta[curPivot]);
    sinT = sin(target->theta[curPivot]);

    current->xb[curPivot] = target->xf[curPivot] -
        ((double)constants->graspX[curPivot]*cosT -
         (double)constants->graspY[curPivot]*sinT);
    current->yb[curPivot] = target->yf[curPivot] -
        ((double)constants->graspX[curPivot]*cosT +
         (double)constants->graspY[curPivot]*sinT);
    (*pivotsLeft)--;

    /*
     *  Update crossover lists.
     *
     *  In order to save some time, take advantage of a couple of our
     *  rules, namely -
     *    - As a fibre can only be moved if there is no fibre crossing
     *      above it, and it can not be placed under another button,
     *      the crossover.above list for the button being moved is
     *      unchanged.
     *    - Any fibre/fibre collisions that will occur must result in
     *      the fibre being moved crossing above them (a fibre can
     *      not be placed under abother button), therefore these
     *      fibres must be added to the crossover.below list for the
     *      moved fibre, with the old crossover.below list being
     *      deleted.
     */
    ptmp = crosses->below[curPivot];
    while (ptmp) {
        tdFdeltaDeleteCross(curPivot+1,
                            &crosses->above[(ptmp->piv)-1],
                            status);
        current->nAbove[(ptmp->piv)-1]--;
        tdFdeltaDeleteCross(ptmp->piv,
                            &ptmp,
                            status);
    }
    crosses->below[curPivot] = NULL;
    current->nBelow[curPivot] = 0;
    for (j=0; j < numPivots; j++) {
        int flag;
        if (curPivot == j) continue;
        if (current->park[j] == YES) continue;
        flag = FpilColFibFib (
            tdFdeltaFpilInst(),
            (double)constants->xPiv[curPivot],
            (double)constants->yPiv[curPivot],
            (double)current->fvpX[curPivot],
            (double)current->fvpY[curPivot],
            (double)constants->xPiv[j],
            (double)constants->yPiv[j],
            (double)current->fvpX[j],
            (double)current->fvpY[j]);


        if (flag == YES) {
            tdFdeltaAddCross(j+1,&crosses->below[curPivot],status);
            tdFdeltaAddCross(curPivot+1,&crosses->above[j],status);
            current->nAbove[j]++;
            current->nBelow[curPivot]++;
        }
    }        
    tdFdeltaCFaddCmd (status,
                      cmdFileId,(*lineNumber)++,
                      "MF",curPivot+1,
                      target->xf[curPivot],
                      target->yf[curPivot],
                      target->theta[curPivot]);
    (*numMoves)++;

    /*
     *  Set the DELTA_PROG parameter - this indicates the progress of the
     *  ordering process.
     */
    DisplayProgress(*numMoves, numParks, *pivotsLeft, 
                        lastUpdate, status);
    if (*status != STATUS__OK) {
        ErsRep(0,status,"Error updating interim field details - %s",
               DitsErrorText(*status));
        return;
    }

}
/*
 *  Record a park. They will only happen during the park phase of the
 *  operation.
 */
static void RecordPark(
    const short         parkFibre,
    const SdsIdType     cmdFileId,
    tdFinterim          * const current,
    tdFtarget           * const target,
    const tdFconstants  * const constants,
    tdFcrosses          * const crosses,
    int                 * const pivotsLeft,
    unsigned            * const lineNumber,
    unsigned            * const numParks,
    const unsigned      numMoves,
    float               * const lastUpdate,
    StatusType          * const status)
{
    FibreCross *ptmp;

    if (*status != STATUS__OK) return;

    /*
     * Update counter.
     */
    (*pivotsLeft)--;

    if (target->mustMove[parkFibre] == NO) {
        target->mustMove[parkFibre] = YES;
    } else if (target->mustMove[parkFibre] == IF_NEEDED) {
        target->mustMove[parkFibre] = NO;
    }

    current->theta[parkFibre]       = constants->tPark[parkFibre];
    current->fibreLength[parkFibre] = 0;
    current->fvpX[parkFibre]        = constants->xPark[parkFibre];
    current->fvpY[parkFibre]        = constants->yPark[parkFibre];
    current->park[parkFibre]        = YES;
    current->xf[parkFibre] = current->xb[parkFibre] =
        constants->xPark[parkFibre];
    current->yf[parkFibre] = current->yb[parkFibre] =
        constants->yPark[parkFibre];

    /*
     *  Update crossover lists.
     */
    ptmp = crosses->below[parkFibre];
    while (ptmp) {
        tdFdeltaDeleteCross(parkFibre+1,
                            &crosses->above[(ptmp->piv)-1],
                            status);
        current->nAbove[(ptmp->piv)-1]--;
        tdFdeltaDeleteCross(ptmp->piv,
                            &ptmp,
                            status);
    }
    crosses->below[parkFibre] = NULL;
    current->nBelow[parkFibre]  = 0;

    /*
     *  Add park to command file.
     */
    tdFdeltaCFaddCmd (status,
                      cmdFileId,(*lineNumber)++,
                      "PF",parkFibre+1);
    (*numParks)++;


    /*
     *  Set the DELTA_PROG parameter - this indicates the progress of the
     *  ordering process.
     */
    DisplayProgress(numMoves, *numParks, *pivotsLeft, 
                        lastUpdate, status);
    if (*status != STATUS__OK) {
        ErsRep(0,status,"Error updating interim field details - %s",
               DitsErrorText(*status));
        return;
    }
}

/*
 * When parking, we may have to swap elements in the distance array
 * due to cross overs.
 */
static void CrossSwap(
    const unsigned int index,   /* Index of the items with a cross over */
    const unsigned int otherPiv,/* Pivot crossing this one              */
    const unsigned int numPivots,/* Number of pivots in array           */
    short * const lastParkIndex, /* Index of last entry to be parked    */
    short * const firstMoveIndex,/* Index of first move operation       */
    int   * const pivotsLeft,    /* Remaning operations                 */
    pivotDistance * const distanceArray, /* Distance array */
    StatusType    *const status)
{
    pivotDistance savedValue;
    register unsigned i;
    double distanceDiff;

    if (*status != STATUS__OK) return;

    /*
     * Scan through the rest of the array to find the other pivot.
     * 
     */
    for (i = index+1 ; i < numPivots ; ++i) {
        if (distanceArray[i].pivot == otherPiv)
            break;
    }
    /*
     * If we  didn't find it, something is really wrong - we must have
     * already parked it so why is it crossing.  Probable programming
     * error.
     */
    if (i >= numPivots) {
        unsigned int pivot = distanceArray[index].pivot;
        *status = TDFDELTA__DELTAERR;
        ErsRep(0, status, 
               "Cannot park pivot %d since it is crossed by other fibres.", 
               pivot+1);
        ErsRep(0, status, "And the first crossing fibre - %d - is not in the list to be parked.", otherPiv+1);
        ErsRep(0, status, "This is probably a programming error.");
        return;
    }

    /*
     * The two pivots to be swaped must be at a similar distance. Check
     * this.
     */
    distanceDiff = fabs(distanceArray[index].distance - 
                        distanceArray[i].distance);

   
    if (distanceDiff/fabs(distanceArray[index].distance) > DIST_MAX) {
        unsigned int pivot = distanceArray[index].pivot;
        *status = TDFDELTA__DELTAERR;
        ErsRep(0, status, 
               "Cannot park pivot %d since it is crossed by other fibres.", 
               pivot+1);
        ErsRep(0, status, "And the first crossing fibre - %d - is not at a similar distance from the center.", otherPiv+1);
        ErsRep(0, status, "This is probably because the field was not configured using the 6dF delta technique.");
        return;


    }
 
   /*
     * If the item to be swapped is after the last item to be parked in 
     * the array of park operations - then it was taken out by the cull.  We
     * must re-insert all operations cull up to this one, by adjusting
     * *lastParkIndex, *firstMoveIndex and *pivotsLeft.
     */
    if ((short)i > *lastParkIndex) {
        int diff = i - *lastParkIndex;
        *lastParkIndex   = i;
        *firstMoveIndex += diff;
        *pivotsLeft     += (diff *2);
    }

    /*
     * Now we want to move distanceArray[i]'s value to before 
     * distanceArray[index]
     */
    savedValue = distanceArray[i];
    for ( ; i > index ; --i) {
        distanceArray[i] = distanceArray[i-1];
    }
    distanceArray[index] = savedValue;
}
    

/*
 * Called to park the field
 */
static int ParkField(
    tdFdeltaType        * const data,
    const SdsIdType     cmdFileId,
    const short         numOps,
    short               * const lastParkIndex,
    short               * const firstMoveIndex,
    pivotDistance       * const distanceArray,
    int                 * const pivotsLeft,
    unsigned            * const lineNumber,
    unsigned            * const numParks,
    const unsigned      numMoves,
    float               * const lastUpdate,
    StatusType          * const status)
{
    register int i;

    /*
     * Now work through the sorted pivot array.
     */
    for (i = 0; (i <= *lastParkIndex)&&(*status == STATUS__OK) ; ++i) {
        unsigned pivot = distanceArray[i].pivot;
        while (data->crosses.above[pivot]) {
            /*
             * We have a cross-over.
             *
             * We need to put the fibre which is crossing us into
             * this position, and push the reset of the array down.
             */                
            CrossSwap(i,
                      data->crosses.above[pivot]->piv-1,
                      numOps,
                      lastParkIndex,
                      firstMoveIndex,
                      pivotsLeft,
                      distanceArray,
                      status);
            
            if (*status != STATUS__OK) return 0;
            pivot = distanceArray[i].pivot;
        }           
        
        /*
         * We want to park this one.
         */
        RecordPark(pivot,
                   cmdFileId,
                   &data->current,
                   &data->target,
                   &data->constants,
                   &data->crosses,
                   pivotsLeft,
                   lineNumber,
                   numParks,
                   numMoves,
                   lastUpdate,
                   status);
        
    }

    if (*status != STATUS__OK) 
        return 0;
    else
        return 1;
}

/*
 * Called to position the field.  Can be assumed that all fibres
 * are parked.
 */ 
static int PositionField(
    tdFdeltaType        * const data,
    const unsigned      numPivots,
    const SdsIdType     cmdFileId,
    const short         firstMoveIndex,
    const pivotDistance * const distanceArray,
    int                 * const pivotsLeft,
    unsigned            * const lineNumber,
    const unsigned      numParks,
    unsigned            * const numMoves,
    float               * const lastUpdate,
    StatusType          * const status)
{
    register int i;
             
    /*
     * Now work through the sorted pivot array, in reverse order since
     * the sort produces it in the park order.
     */
    for (i = firstMoveIndex; (i >= 0)&&(*status == STATUS__OK) ; --i) {
        unsigned int pivot = distanceArray[i].pivot;
        
        if (data->crosses.above[pivot]) {
            *status = TDFDELTA__DELTAERR;
            ErsRep(0, status, "Cannot move pivot %i since it is crossed both others", pivot+1);
            ErsRep(0, status, "This should not be happending in this algrothim");
            ErsRep(0, status, "Probably a programming error");
            return 0;
            
            
        }
        /*
         * We want to move this one.
         */
        RecordMove(pivot,
                   cmdFileId,
                   numPivots,
                   &data->current,
                   &data->target,
                   &data->constants,
                   &data->crosses,
                   pivotsLeft,
                   lineNumber,
                   numParks,
                   numMoves,
                   lastUpdate,
                   status);
        
    }
    
    if (*status != STATUS__OK) 
        return 0;
    else
        return 1;
}

/*
 * The idea here is to ensure that if we are restarting a partly configured
 * field, that we don't try to reconfigure fibres which are already in the
 * right spot.
 */
static int CullOk(
    const short          * const mustMove,
    const unsigned short numParkOps,
    const unsigned short numMoveOps,
    const pivotDistance  * const parkDistArray,
    const pivotDistance  * const moveDistArray,
    int                  * const pivotsLeft,
    short                * const lastParkIndex,
    short                * const firstMoveIndex,
    StatusType           * const status)
{
    if (*status != STATUS__OK) return(0);
/*
 *  When we return, we must have lastParkIndex set to the
 *  index in parkDistArray of the last park operation to be done, whilst
 *  firstMoveIndex is the index of the first move to be done.  Note that both
 *  parkDistArray and moveDistArray are sorted in park order.
 */
    *lastParkIndex = numParkOps - 1;
    *firstMoveIndex  = numMoveOps - 1;

/*
 *  Assume for the moment that both parkDistArray and moveDistArray are
 *  in move order.  Then as we work down this list, if the pivots are the
 *  same and the pivot does not have to be moved, then we can leave it 
 *  where it is.  The first time we find no match, we must park all the fibres
 *  from that point (in reverse order) and then move all the new fibres in.
 *
 *  If the first fibre does not match, then all fibres are parked and moved
 *  again.
 *
 *  Now remember that the arrays are in the reverse order.
 */
    while (1)
    {
        if ((*lastParkIndex < 0) || (*firstMoveIndex < 0))
            /*
             * No more work to do - bail out 
             */
            break;

        else if (parkDistArray[*lastParkIndex].pivot !=
                 moveDistArray[*firstMoveIndex].pivot)
            /* 
             * No match in pivot number- bail out 
             */
            break;
        else if (mustMove[parkDistArray[*lastParkIndex].pivot])
            /* 
             * pivot number matchs but button must be moved - bail out 
             */
            break;
        else {
            /*
             * Don't both be parking and moving this one, it is in the 
             * right spot.
             */
            --(*lastParkIndex);
            --(*firstMoveIndex);
            /*
             * Subtract 2 from the number of moves to do.
             */
            (*pivotsLeft) -= 2;
        }
    }

    return (1);
}
/*
 *  Returns 1 if ok to continue tdFdeltaSequencer.  Returns 0 to indicate
 *  tdFdeltaSequencer should return immediately.
 */
static int SequencerInit(
    tdFdeltaType        * const data,
    unsigned            * const numPivots,
    time_t              * const tStart,
    float               * const lastUpdate,
    SdsIdType           * const cmdFileId,
    int                 * const pivotsLeft,
    unsigned short      * const numParkOps,
    unsigned short      * const numMoveOps,
    pivotDistance       * const parkDistArray,
    pivotDistance       * const moveDistArray,
    unsigned short      * const numSpringOutPark,
    StatusType * const status)
{
    register unsigned i;
    unsigned short numSpringOutMove;

    if (*status != STATUS__OK) return(0);

    /*
     *  Get the number of pivots in this instrument
     */
    (*numPivots) = FpilGetNumPivots(tdFdeltaFpilInst());

    /*
     *  Start timing and set parameters/variables.
     */
    time(tStart);
    *lastUpdate = 0.0;
    SdpPutf("DELTA_PROG",0.0,status);
    if (*status != STATUS__OK) {
        ErsRep(0,status,"Error updating parameter - %s",
               DitsErrorText(*status));
        free((void *)data);
        goto ERROR_RETURN;
    }

    /*
     *  Bypass checking if requested.
     */
    if (data->check & NO_DELTA) {
        MsgOut(status,"Delta (ordering) process NOT performed");
        goto ERROR_RETURN;
    } else
        MsgOut(status,"Performing special delta (ordering) process...");

    /*
     *  Open a new command file.
     */
    (*cmdFileId) = tdFdeltaCFnew (data->name,&data->current,
                                  &data->above,status);
    /*
     * Handle command file opening error.
     */
    if (*status != STATUS__OK) {
        ErsRep(0,status,"Error opening new command file - %s",
               DitsErrorText(*status));
        goto ERROR_RETURN;
    }
    /*
     *  Initialise parameters.
     */
    for (i=0; i < (*numPivots); i++) {
        /* One move to park each pivot not already parked */
        if (data->current.park[i] == NO)
            (*pivotsLeft)++;
        /* One move to put each pivot back on the the plate when required */
        if (data->target.park[i] == NO) {
            (*pivotsLeft)++;
        }
    }


    /*
     * Get the park order
     */
    MoveSort(*numPivots, 
             data->extSpringOut,
             data->constants.xPiv,
             data->constants.yPiv,
             data->current.xf,
             data->current.yf,
             data->current.park,
             numParkOps,
             parkDistArray,
             numSpringOutPark,
             status);

    /*
     * Get the move order (actually the reverse of the move order).
     */ 
    MoveSort(*numPivots, 
             data->extSpringOut,
             data->constants.xPiv,
             data->constants.yPiv,
             data->target.xf,
             data->target.yf,
             data->target.park,
             numMoveOps,
             moveDistArray,
             &numSpringOutMove,
             status);


    if (*status != STATUS__OK)  goto ERROR_RETURN;

#if 0
    /*
     * Now work through the sorted park array, swaping entries due to
     * cross overs if needed.  This may be needed if due to buttons not
     * being placed exactly in the correct place, the distance has changed.
     */
    for (i = 0; i < *numParkOps ; ++i) {
        unsigned pivot = parkDistArray[i].pivot;
        while (data->crosses.above[pivot]) {
            
            /*
             * We have a cross-over.
             *
             * We need to put the fibre which is crossing us into
             * this position, and push the reset of the array down.
             */                
            CrossSwap(i,
                      data->crosses.above[pivot]->piv-1,
                      *numParkOps,
                      parkDistArray,
                      status);
            
            if (*status != STATUS__OK)  goto ERROR_RETURN;
        }
        
    }
#endif
    return (1);

 ERROR_RETURN:
    /*
     * This is used to ensure the data structure is freeed correctly
     * when we return with an error.
     */
    if (data->above)
    {
        StatusType ignore = STATUS__OK;
        SdsDelete(data->above,&ignore);
        SdsFreeId(data->above,&ignore);
        data->above = 0;
    }
    free (data);
    return (0);
    
}

TDFDELTA_INTERNAL void  tdFdeltaSequencerSpecial (
        StatusType  *status)
{
    tdFdeltaType  *data = DitsGetActData();/* Function parameters            */
    SdsIdType     cmdFileId;        /* Command file Id (Sds structure id)    */
    time_t        tStart, tEnd;     /* Used for timing this function         */
    static float  lastUpdate;       /* DELTA_PROG at last update             */
    int           pivotsLeft = 0;   /* Number of pivots left to move         */
    unsigned      lineNumber = 1,   /* Counters                              */
                  numMoves = 0,     /* Final number of pivots to be moved    */
                  numParks = 0;     /* Final number of pivots to be parked   */
    unsigned numPivots;             /* Number of pivots                      */

    unsigned short numParkOps = 0;
    unsigned short numMoveOps = 0;
    pivotDistance parkDistArray[FPIL_MAXPIVOTS];
    pivotDistance moveDistArray[FPIL_MAXPIVOTS];

    short lastParkIndex;
    short firstMoveIndex;
    unsigned short  numSpringOutParks;

    if (*status != STATUS__OK) return;

    /*
     * Initialise this function's variables etc.
     */
    if (!SequencerInit(data,&numPivots, &tStart, &lastUpdate, &cmdFileId,
                       &pivotsLeft, &numParkOps, &numMoveOps,
                       parkDistArray, moveDistArray, &numSpringOutParks,
                       status))
        return;

    if (!CullOk(data->target.mustMove,
                numParkOps,
                numMoveOps,
                parkDistArray,
                moveDistArray,
                &pivotsLeft,
                &lastParkIndex,
                &firstMoveIndex,
                status))
        return;


    if (!ParkField(data, cmdFileId, numParkOps, &lastParkIndex, 
                   &firstMoveIndex, parkDistArray,
                   &pivotsLeft, &lineNumber,
                   &numParks, numMoves, &lastUpdate, status))
        return;

    if ((pivotsLeft)&&
        (!PositionField(data, numPivots, cmdFileId, firstMoveIndex, 
                        moveDistArray, &pivotsLeft, &lineNumber,
                        numParks, &numMoves, &lastUpdate, status)))
        return;

                    

    /*
     * The cull could result in this happeing - the spring out parks
     * are always first
     */
    if (numParks < numSpringOutParks)
        numSpringOutParks = numParks;
    /*
     *  Record the number of moves and parks in the command file.
     */
    tdFdeltaCFaddMoves (cmdFileId,(long int)numMoves,(long int)numParks,status);
    tdFdeltaCFaddSpringOutParks(cmdFileId, (long int)numSpringOutParks, status);

    /*
     *  End timimg.
     */
    time(&tEnd);

    /*
     *  Command file generated - report and return.
     */
    MsgOut(status,"Command file generated - %s (%d %s, %d %s) - in %ld %s",
           data->name,
           numMoves, numMoves == 1?  "move": "moves",
           numParks, numParks == 1?  "park": "parks",
           (long)tEnd-tStart, tEnd-tStart == 1?  "second": "seconds");
    
    DitsPutArgument(cmdFileId,DITS_ARG_DELETE,status);

    /*
     * data->above should be gone by now, but just be sure to avoid
     * memory leaks.
     */
    if (data->above)
    {
        StatusType ignore = STATUS__OK;
        SdsDelete(data->above,&ignore);
        SdsFreeId(data->above,&ignore);
        data->above = 0;
    }
    free((void *)data);
}
