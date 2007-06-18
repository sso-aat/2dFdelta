/*+                T D F D E L T A

 *  Module name:
      tdFdeltaSequencer

 *  Function:
      Responsible for the generation of the sequence of moves necessary to
      change from the current configuration to the target configuration.

 *  Description:


 *  Language:
      C

 *  Support: James Wilcox, AAO

 *-

 *  History:
      01-Jul-1994  JW   Original version
      29-Oct-1996  KS   Fixed bug in tdFdelta___DeltaDirectMove
                        that was causing unnecessary parks.
      26-Apr-2000  TJF  Some support added for instruments where the
                        park position may collide, but more thourght
                        required.
      01-Nov-2000  TJF  Remove ability to check FPIL against old
                        tdFcollision routines.
      09-Apr-2003  TJF  Invesitage and fix double park problem.  Several changes
                        and lots of debugging added.
                        Drop   tdFdelta___DirectField().  Note needed.
                         

      {@change entry@}

 *  @(#) $Id: ACMM:2dFdelta/tdFdelSeq.c,v 3.8 19-Jun-2007 09:32:34+10 tjf $
 */

/*
 *  Include files
 */


static char *rcsId="@(#) $Id: ACMM:2dFdelta/tdFdelSeq.c,v 3.8 19-Jun-2007 09:32:34+10 tjf $";
static void *use_rcsId = (0 ? (void *)(&use_rcsId) : (void *) &rcsId);


#include "DitsTypes.h"       /* Basic dits types            */
#include "Dits_Err.h"        /* Dits error codes            */
#include "DitsSys.h"         /* For PutActionHandlers       */
#include "DitsFix.h"         /* For various Dits routines   */
#include "DitsMsgOut.h"      /* For MsgOut                  */
#include "DitsUtil.h"        /* For DitsErrorText           */
#include "DitsParam.h"       /* For DitsGetParId            */
#include "DitsPeek.h"        /* For DitsPeek                */
#include "Dmon.h"            /* To DmonCommand              */
#include "arg.h"             /* For ARG_ macros             */
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

/*
 *  Definitions.
 */
#define MAX_PARKS  1      /* Maximum number of times a button can be returned to its.. */
                          /* ..park position during a single field configuration       */
#define SCALE      0.25   /* Used when calculating the DELTA_CONFIG parameter value..  */
                          /* ..in an attempt to make progress v time linear            */


#define DEBUG_DELTA

static void checkMoveOk(
    const unsigned      piv,
    const tdFinterim    * const iField,
    const tdFcrosses    * const iCrosses,
    StatusType          * const status)
{
    if (*status != STATUS__OK) return;
    if (iField->nAbove[piv] != 0)
    {
        *status = TDFDELTA__CROSSESERR;
        ErsRep(0, status, "Last chance cross check triggered");
        ErsRep(0, status, "Attempt to move fibre %d when crossed %d times", 
               piv+1, iField->nAbove[piv]);
        if (iCrosses->above[piv])
            ErsRep(0, status, "First crossing fibre = %d",
                   iCrosses->above[piv]->piv+1);
        else
            ErsRep(0, status, "Inconsist cross list");
    }
}


/*
 *  Internal Function, name:
      tdFdelta___CheckFibresUnder

 *  Description:
      This is invoked if we think we can do a direct move of a fibre from
      its current position to its new positions.

 *  History:
      09-Apr-2003 TJF   Original version
      {@change entry@}
 */

#ifdef DEBUG_DELTA
/*
 * Debugging function to print cross over lists.
 */
static void PrintCrosses(
    const unsigned    piv,
    const tdFinterim  * const cur,
    const tdFcrosses  * const crosses,
    StatusType        * const status)
{
    if (*status != STATUS__OK) return;
    if (cur->nBelow[piv] > 0)
    {
        fprintf(stderr, " Fibre %d crosses %.2d fibres, being ", 
                piv+1, cur->nBelow[piv]);
        
        if (crosses->below[piv])
        {
            FibreCross *ptmp = crosses->below[piv];
            while (ptmp)
            {
                fprintf(stderr, "%.3d ", ptmp->piv);
                ptmp = ptmp->next;
            }
        }
        else
        {
            fprintf(stderr, "[INVALID CROSS LIST]");
        }
    }
    else
    {
        fprintf(stderr, "Fibre %d cross no fibres", piv+1);
    }
    fprintf(stderr, "\n");

}
#endif

/*
 * CheckUnder - does the actual check using the correct cross over
 * list.
 */
static int CheckUnder(
    const unsigned    piv,            /* Index of pivot to check */
    const tdFinterim  * const cur,
    const tdFtarget   * const tField,
    const tdFcrosses  * const crosses,
    StatusType        * const status)
{
    if (*status != STATUS__OK) return 0;

    /*
     * Now - the cross over lists are appropiate.  What we want to do is
     * try to work out if we will ever have to move "piv" - this may be
     * required if any fibre underneath has (mustMove[otherPiv] == YES) or
     * any fibre under those etc.  
     */

    if (cur->nBelow[piv])
    {
        FibreCross *ptmp = crosses->below[piv];
        /*
         * Look at the fibres under us, if any must move, then it trigger
         * us to get out, returning the number.
         */
        while (ptmp)
        {
            unsigned p = ptmp->piv;
            if (tField->mustMove[p-1] == YES)
                return p;
            ptmp = ptmp->next;
        }
        /*
         * Go through again, this time, recurisvely check each fibre
         * under us.
         */
        ptmp = crosses->below[piv];
        while (ptmp)
        {
            unsigned p = ptmp->piv;
            int result;
            result = CheckUnder(p-1, cur, tField, crosses, status);
            if (result)
                return result;
            ptmp = ptmp->next;
        }
    }
    return 0;
    

    
}

TDFDELTA_PRIVATE int  tdFdelta___CheckFibresUnder (
    const int           parkMayCollide,
    tdFinterim          * const iField,  /*Changed but returned to orig state*/
    tdFcrosses          * const iCrosses,/*Changed but returned to orig state*/
    const tdFtarget     * const tField,
    const tdFconstants  * const con,
    const unsigned      piv,             /* Index if pivot to check under */
    StatusType          * const status)
{
    register unsigned pIndex;
    unsigned numPivots;
    FibreCross *ptmp;
    int result = 0;
    /*
     * We need to generate the cross over lists which would apply after
     * piv is moved to its target field position.  Whilst ideally we should
     * do this by generating an enitrely new list, the is far quicker to
     * do it by changing the existing list and then resetting it back to
     * the way it was.
     */


    if (*status != STATUS__OK) return 0;

#ifdef DEBUG_DELTA
    fprintf(stderr, "Interim Field Crosses:");
    PrintCrosses(piv, iField, iCrosses, status);            
#endif

    /*
     *  Get the number of pivots in this instrument
     */
    numPivots = FpilGetNumPivots(tdFdeltaFpilInst());

    /* 
     * We can only have crosses below at this point - otherwise we
     * wouldn't get to this point 
     *
     * Delete any crosses below
     */
    ptmp = iCrosses->below[piv];
    while (ptmp) {
        tdFdeltaDeleteCross(piv+1,
                            &iCrosses->above[(ptmp->piv)-1],
                            status);
        iField->nAbove[(ptmp->piv)-1]--;
        tdFdeltaDeleteCross(ptmp->piv,
                            &ptmp,
                            status);
    }
    /*
     * Now are ready to set up the new list -> again, we can only have
     * crosses below at this point.
     */
    iCrosses->below[piv] = NULL;
    iField->nBelow[piv] = 0;
    for (pIndex=0; pIndex < numPivots; pIndex++) {
        int flag;
        if (piv == pIndex) continue;
        if ((iField->park[pIndex] == YES)&&(!parkMayCollide)) continue;

        flag = FpilColFibFib (tdFdeltaFpilInst(),
                              (double)con->xPiv[piv],
                              (double)con->yPiv[piv],
                              (double)tField->fvpX[piv],/* Use target rather */
                              (double)tField->fvpY[piv],/* then existing pos */
                              (double)con->xPiv[pIndex],
                              (double)con->yPiv[pIndex],
                              (double)iField->fvpX[pIndex],
                              (double)iField->fvpY[pIndex]);


        /*
         * A cross occurs -> add it.
         */ 
        if (flag == YES) {
            tdFdeltaAddCross(pIndex+1,&iCrosses->below[piv],status);
            tdFdeltaAddCross(piv+1,&iCrosses->above[pIndex],status);
            iField->nAbove[pIndex]++;
            iField->nBelow[piv]++;
        }
    }
#ifdef DEBUG_DELTA
    fprintf(stderr, "After Move Field Crosses:");
    PrintCrosses(piv, iField, iCrosses, status);            
#endif
    /*
     * We only have an issue if we have any fibres crossing below piv at
     * this point.
     */
    if (iField->nBelow[piv])
    {
        result = CheckUnder(piv, iField, tField, iCrosses, status);
#ifdef DEBUG_DELTA
        fprintf(stderr, "Check under says that %d may move\n", result);
#endif
    }
    /*
     * * * *  Restore actual current cross over list * * * *
     *
     * 
     * Once again, we can only have crosses below at this point - delete them.
     */
    ptmp = iCrosses->below[piv];
    while (ptmp) {
        tdFdeltaDeleteCross(piv+1,
                            &iCrosses->above[(ptmp->piv)-1],
                            status);
        iField->nAbove[(ptmp->piv)-1]--;
        tdFdeltaDeleteCross(ptmp->piv,
                            &ptmp,
                            status);
    }
    /*
     * Now are ready to set up the new list -> again, we can only have
     * crosses below at this point.
     */
    iCrosses->below[piv] = NULL;
    iField->nBelow[piv] = 0;
    for (pIndex=0; pIndex < numPivots; pIndex++) {
        int flag;
        if (piv == pIndex) continue;
        if ((iField->park[pIndex] == YES)&&(!parkMayCollide)) continue;

        flag = FpilColFibFib (tdFdeltaFpilInst(),
                              (double)con->xPiv[piv],
                              (double)con->yPiv[piv],
                              (double)iField->fvpX[piv],
                              (double)iField->fvpY[piv],
                              (double)con->xPiv[pIndex],
                              (double)con->yPiv[pIndex],
                              (double)iField->fvpX[pIndex],
                              (double)iField->fvpY[pIndex]);


        /*
         * A cross occurs -> add it.
         */ 
        if (flag == YES) {
            tdFdeltaAddCross(pIndex+1,&iCrosses->below[piv],status);
            tdFdeltaAddCross(piv+1,&iCrosses->above[pIndex],status);
            iField->nAbove[pIndex]++;
            iField->nBelow[piv]++;
        }
    }
#ifdef DEBUG_DELTA
    fprintf(stderr, "Restored Field Crosses:");
    PrintCrosses(piv, iField, iCrosses, status);            
#endif


    return result;
}



/*
 *  Internal Function, name:
      tdFdelta___DeltaDirectMove

 *  Description:
      Check if it is possible to move a pivot directly from its current 
      position to its target position.

      The following conditions must be checked to ensure that a direct move 
      is possible:

        - No button can be moved if its fibre is being crossed above by 
           another fibre.
        - No button can be moved if there is another fibre crossing above the 
           buttons target position, or another button currently at its target 
           position (ie, if it target position is being obstructed by an 
           un-moved button or fibre).
        - No button can be moved if by moving it, its fibre crosses above the 
           fibre of another button that is not yet moved.

      It is assumed that THE TARGET FIELD IS A VALID FIELD CONFIGURATION.

 *  History:
      01-Jul-1994  JW   Original version
      29-Oct-1996  KS   Call to tdFcollisionButFib() was mis-coded, passing
                        a nonsensical fibre specification that would almost
                        always cause a collision to be signalled. This in turn
                        would cause a large number of unnecessary parks to 
                        be generated. Now fixed.
      28-Jul-1998  TJF  Make use of From Park positioning offsets.  Tidy
                        up offset rotation code.
      10-Aug-1998  TJF  I was a mistake in the original code to use positioning
                        offsets.  It was intended that the grasp offsets
                        be used.  Corrected.  Dropped off arguments.
      31-Jan-2000  TJF  Use FpilCol* instead of tdFcollisionFib* to
                        all us to support other instruments (see 
                        tdFdeltaActivate()).  For the moment, we keep the
                        call to tdFcollisoin* as well as a check on 
                        the Fpil code, but only if the Macro TDF_CHK_FPIL
                        is defined.
      01-Feb-2000  TJF  Instead of using NUM_PIVOTS macro, use the new
                        variable numPivots, which is initialised to the
                        value returned by FpilGetNumPivots().
      26-Apr-2000  TJF  In 6dF, a fibre may collide with the park position
458
                        of another fibre.  We use FpilParkMayCollide() to
                        determine if we are dealing with such an instrument
                        and in these cases, will check against park positions.
      05-Jun-2000  TJF  We can't correctly handle ParkMayCollide and the
                        fibre bend angles have been modified to prevent it,
                        so clear the flag.
      01-Nov-2000  TJF  Remove ability to check FPIL against old
                        tdFcollision routines.
      20-Dec-2000  TJF  Use xf/yf instead of xb/yb in FpilColButBut call and
                        second FpilColButFib call.
      07-May-2002  TJF  Ensure that were status is set, fibre details are output.
      09-Apr-2003  TJF  Invesitage and fix double park problem - must ensure
                         that we don't direct move a fibre which, in the 
                         resulting position, is crossing a fibre we will 
                         later want to move.
      19-Jun-2007  TJF  This was the only collision detection code that
                         was passing the button position rather then the
                         fibre position to the collission check, by 
                         subtracting the grasp, but this being done in the
                         wrong direction - it should have been adding the
                         grasp to the fibre to get the button position.  This
                         was not a problem until AAOmega when the grasp offset
                         became significant (500 microns, so this becomes 
                         a 1mm mistake).  For the moment, go to using only
                         the fibre position, since this makes the code
                         consistent - but it does rely on the collision 
                         detection having enough clearance to account for
                         varying grasp offsets (the object fibres are about
                         500 microns, the guide fibres are closer to zero).

      {@change entry@}
 */
TDFDELTA_PRIVATE int  tdFdelta___DeltaDirectMove (
    tdFinterim          * const iField,/*Changed but returned to orig state*/
    tdFcrosses          * const iCrosses,/*Changed but returned to orig state*/
    const tdFtarget     * const tField,
    const tdFconstants  * const con,
    const long int      butClearG,
    const long int      butClearO,
    const long int      fibClearG,
    const long int      fibClearO,
    const int           piv,
    StatusType          * const status)
{
    double    pivotDist,                /* Dist between 2 pivot points      */
              graspXt DUNUSED, graspYt DUNUSED; /* X and Y rotated grasp values,
                                           target      */
    long int  fibreClear, buttonClear;  /* Clearances used during colision 
                                           detection  */
    unsigned  otherPiv;                 /* Pivot that piv is being checked 
                                           against    */
    int       flag;                     /* Returned value from collision 
                                           functions    */
    double    cosT DUNUSED, sinT DUNUSED ;/* Sine and Cosine of theta */
    unsigned  numPivots;                /* Number of pivots */
    int ParkMayCollide;                 /* Can fibres collided with parked 
                                           fibres? */
                                           

    if (*status != STATUS__OK) return 0;

    /*
     *  Get the number of pivots in this instrument
     */
    numPivots = FpilGetNumPivots(tdFdeltaFpilInst());
    /*
     * We need to determine if we have to check for collisions against
     * parked fibres.
     */
    ParkMayCollide = FpilParkMayCollide(tdFdeltaFpilInst());
    ParkMayCollide = 0;   /* Override since we can't correctly
                             handle it and the 6dF bend angles have
	                     be modified to prevent it */
    /*
     *  Can not move a button if there is a fibre crossing above its fibre.
     */
    if (iField->nAbove[piv] != 0) {
        if (iCrosses->above[piv])
            return (iCrosses->above[piv]->piv);
        else {
            *status = TDFDELTA__CROSSESERR;
            ErsRep(0, status, "Crossover list error - fibre %d should have %d fibres acrossing above, but list is empty",
		   piv+1, iField->nAbove[piv]);
            return (0);
        }
    }
   
    if (piv == 325) {
        fprintf(stderr,"My fibre\n");
    }

    /*
     *  Calculate the offsets for this button for the target pos's.
     */
#if 0
    cosT = cos(iField->theta[piv]);
    sinT = sin(iField->theta[piv]);
    graspXt = ((double)con->graspX[piv])*cosT -
              ((double)con->graspY[piv])*sinT;
    graspYt = ((double)con->graspY[piv])*cosT +
              ((double)con->graspX[piv])*sinT;
#endif
    /*
     *  Loop through different checks, return the number of any pivot that is
     *  preventing the current button from being moved.
     */
    for (otherPiv=0; otherPiv<numPivots; otherPiv++) {

        /*
         *  Do not check fibre against itself.
         */
        if (otherPiv == (unsigned)piv) continue;

        /*
         *  If park positions don't collide with field positions, then
         *  if otherPiv is in its park position it can not prevent piv from
         *  being moved.
         */
        if ((!ParkMayCollide)&&(iField->park[otherPiv] == YES)) continue;

        /*
         *  These checks are not necessary if a button does not have to be
         *  moved
         */
        if (tField->mustMove[otherPiv] == NO) continue;

        /*
         *  Calculate distance between two pivot points and the offsets.
         */
        pivotDist = SQRD((double)(con->xPiv[piv] - con->xPiv[otherPiv])) +
                    SQRD((double)(con->yPiv[piv] - con->yPiv[otherPiv]));
        pivotDist = sqrt(pivotDist);

        /*
         *  Check if piv(target) and otherPiv(interim) could collide.
         */
        if ((tField->fibreLength[piv] + iField->fibreLength[otherPiv]) 
            > pivotDist) {

            /*
             *  Will the fibre of otherPiv preventing piv from being placed 
             *  in its target position?
             */
            fibreClear = (con->type[otherPiv] == GUIDE)?  fibClearG: fibClearO;

            FpilSetFibClear(tdFdeltaFpilInst(), fibreClear);
            flag = FpilColButFib (
                                  tdFdeltaFpilInst(),
                                  (double)tField->xf[piv] /*- graspXt*/,
                                  (double)tField->yf[piv] /*- graspYt*/,
                                  tField->theta[piv],
                                  (double)iField->fvpX[otherPiv],
                                  (double)iField->fvpY[otherPiv],
                                  (double)con->xPiv[otherPiv],
                                  (double)con->yPiv[otherPiv]);


            if (flag > 0) return (otherPiv+1);

            /*
             *  Will the button of otherPiv prevent piv from being placed 
             *  in its target position?
             */
            buttonClear = ((con->type[piv] == GUIDE) ||
                           (con->type[otherPiv] == GUIDE))?  butClearG: butClearO;
            FpilSetButClear(tdFdeltaFpilInst(), buttonClear);

            flag = FpilColButBut (
                                  tdFdeltaFpilInst(),
                                  (double)tField->xf[piv] /*- graspXt*/,
                                  (double)tField->yf[piv] /*- graspYt*/,
                                  tField->theta[piv],
                                  (double)iField->xf[otherPiv],
                                  (double)iField->yf[otherPiv],
                                  iField->theta[otherPiv]);


            if (flag > 0) return (otherPiv+1);

            /*
             *  Will the fibre of piv cross above a fibre that is not yet 
             *  moved?
             */
            flag = FpilColFibFib (
                                  tdFdeltaFpilInst(),
                                  (double)con->xPiv[piv],
                                  (double)con->yPiv[piv],
                                  (double)tField->fvpX[piv],
                                  (double)tField->fvpY[piv],
                                  (double)con->xPiv[otherPiv],
                                  (double)con->yPiv[otherPiv],
                                  (double)iField->fvpX[otherPiv],
                                  (double)iField->fvpY[otherPiv]);

            if (flag > 0) return (otherPiv+1);

            /*
             *  Will the fibre of piv collide with another button?
             */
            fibreClear = (con->type[piv] == GUIDE)?  fibClearG: fibClearO;
            FpilSetFibClear(tdFdeltaFpilInst(), fibreClear);
            flag = FpilColButFib (
                                  tdFdeltaFpilInst(),
                                  (double)iField->xf[otherPiv],
                                  (double)iField->yf[otherPiv],
                                  iField->theta[otherPiv],
                                  (double)tField->fvpX[piv],
                                  (double)tField->fvpY[piv],
                                  (double)con->xPiv[piv],
                                  (double)con->yPiv[piv]);


            if (flag > 0) return (otherPiv+1);
        }
    }
    /*
     * If we cross other fibres (after moving), then it is required that 
     * none of the other fibres or ones they cross or ones they cross etc
     * needs to be moved.  Otherwise we have to park this one anyway.
     *
     * This returns the pivot in question or 0 which indicates it is ok
     * to move directly.
     */
    return (tdFdelta___CheckFibresUnder(ParkMayCollide,
                                        iField, iCrosses, tField, 
                                        con, piv, status));
    
}


/*
 *  Internal Function, name:
      tdFdelta___DeltaChoosePark

 *  Description:
      Choose the optimum fibre to return to its park position.

 *  History:
      01-Jul-1994  JW   Original version
      01-Feb-2000  TJF  Instead of using NUM_PIVOTS macro, use the new
                        variable numPivots, which is initialised to the
                        value returned by FpilGetNumPivots().  The dimension
                        of parkList should be FPIL_MAXPIVOTS instead
                        of NUM_PIVOTS.
      01-Jun-2000  TJF  There appears to be no reason for us to be
                        mallocing a fibre cross.  Remove the malloc
                        and move the declartion of ptmp to where
                        it is used.
      09-Apr-2003  TJF  Invesitage and fix double park problem - increment
                         numUnParkedNotMovedLeft when we change a fibre from
                         no move to must move.
      {@change entry@}
 */
TDFDELTA_PRIVATE int  tdFdelta___DeltaChoosePark (
        tdFinterim  *iField,
        tdFcrosses  *iCrosses,
        tdFtarget   *tField,
        int         *pivotsLeft,
        short       numMovesPrevented[FPIL_MAXPIVOTS],
        short       *numUnParkedNotMovedLeft,
        StatusType  *status)
{
    int         candidate = 0;
    unsigned    j;
    short       listReset = YES,
                altNumMovesPrevented[FPIL_MAXPIVOTS];
    unsigned    numPivots;

#ifdef DEBUG_DELTA
    fprintf(stderr, "ChoosePark: 193 prevents %d moves, 190 %d\n", 
            numMovesPrevented[192],  numMovesPrevented[189]);
#endif

    if (*status != STATUS__OK) return 0;


    /*
     *  Get the number of pivots in this instrument
     */
    numPivots = FpilGetNumPivots(tdFdeltaFpilInst());

    /*
     *  Initialise altNumMovesPrevented.
     */
    for (j=0; j < numPivots; j++)
    {
        altNumMovesPrevented[j] = 0;
    }
    /*
     *  Loop until either a parkable fibre is found, or an error occurs.
     */
    while (1) {

	/*
         * Find the fibre that is preventing the most fibres from being moved
         */
        for (j=0; j < numPivots; j++)
            if (numMovesPrevented[j] > numMovesPrevented[candidate])
                candidate = j;

        /*
         *  If the candidate button is preventing another button from being 
         *  moved,  see if we can park this button. If not, note the number 
         *  of the button preventing movement (ie., update 
         *  altNumMovesPrevented).
         */
        if (numMovesPrevented[candidate] > 0) {
	    /*
	     * Set the listReset flag to NO - if can't park this
	     * one then may have to reset the list
	     */ 
            listReset = NO;
            /*
             *  Check if we can park this button.
             *  (ie, are there any fibres crossing above this button).
             */
            if (iField->nAbove[candidate] != 0) {
		/*
		 * There are one or more fibres crossing above the candidate
		 * so add these crossing fibres to the alternate park list
		 */
                FibreCross  *ptmp;
                ptmp = iCrosses->above[candidate];
                while (ptmp) {
#ifdef DEBUG_DELTA
                    if ((ptmp->piv == 193)||(ptmp->piv == 190))
                    {
                        fprintf(stderr,"Fibre %d is on fibre %d cross list\n",
                                ptmp->piv, candidate+1);
                    }
#endif
                    altNumMovesPrevented[ptmp->piv-1]++;
                    ptmp = ptmp->next;
                }
		/*
		 * Set the candidate so it will not be chosen again
		 */
                numMovesPrevented[candidate] = -1;
            }

            /*
             *  We can safely park this fibre if 
             *    numMovesPrevented[candidate] != 0.
             */
            if (numMovesPrevented[candidate] > 0)
            {
#ifdef DEBUG_DELTA
                if ((candidate == 189)||(candidate == 192))
                    fprintf(stderr,
                        "Selecting %d to park, prevents %d fibres moving\n",
                            candidate+1, numMovesPrevented[candidate]);
#endif
                return (candidate+1);   /* piv# = array index + 1 */
            }
        } else  /* numMovesPrevented[candidate] <= 0 */{
	    /*
	     * If no candidate was found, and this is the first time through or
	     * the list was reset on the last attempt, so is an error
	     */
            if (listReset) {
                *status = TDFDELTA__DELTAPARKERR;
                return 0;
            } else {

#ifdef DEBUG_DELTA
                fprintf(stderr,
                        "Triggering reset of numMovesPrevented array\n");
                fprintf(stderr, "ChoosePark:AltList: 193 prevents %d moves\n", 
                        altNumMovesPrevented[192]);
                fprintf(stderr, "ChoosePark:AltList: 190 prevents %d moves\n", 
                        altNumMovesPrevented[190]);
#endif               
 
                /*
                 * If a fibre is preventing others from moving, then
                 * park it.
                 */
                for (j=0; j < numPivots; j++) 
                {
                    if (numMovesPrevented[j] != 0) 
                    {
#ifdef DEBUG_DELTA
                        if ((j == 189)||(j == 192))
                            fprintf(stderr,
                                "Fibre %d preventing %d(%d) moves, park it\n",
                                j+1, altNumMovesPrevented[j],
                                numMovesPrevented[j]);
#endif
                        if (tField->mustMove[j] == NO) 
                        {
                            tField->mustMove[j] = YES;
                            *pivotsLeft += 1;
                            (*numUnParkedNotMovedLeft)++;
                        }
                    }
                    numMovesPrevented[j] = altNumMovesPrevented[j];
                    altNumMovesPrevented[j] = 0;
                }
#ifdef DEBUG_DELTA
                fprintf(stderr, "ChoosePark:NewList: 193 prevents %d moves\n", 
                        numMovesPrevented[192]);
                fprintf(stderr, "ChoosePark:NewList: 190 prevents %d moves\n", 
                        numMovesPrevented[189]);
#endif
                listReset = YES;
            }
        }
    }
}


/*+        T D F D E L T A S E Q U E N C E R

 *  Function name:
      tdFdeltaSequencer

 *  Function:
      Determines the sequence of moves needed to change from the current field
      configuration to the target field configuration.

 *  Description:

 *  Language:
      C

 *  Call:
      (void) = tdFdeltaSequencer (status)

 *  Parameters:  (">" input, "!" modified, "W" workspace, "<" output)
      (!) status      (StatusType *)    Modified status.

 *  Proir Requirements:
      DitsGetActData() must return a pointer to a structure of type tdFdeltaType (see
      tdFdelta.h for structure details).

 *  Support: James Wilcox, AAO

 *-

 *  History:
      01-Jul-1994  JW   Original version
      10-Aug-1998  TJF  Use grasp offset instead of positioning offsets
                        to calculate new button positions.
      17-Jan-2000  TJF  Comment out DitsPeek call due to a DRAMA bug,
			as we don't really need it anyway.
      31-Jan-2000  TJF  Use FpilColFibFib instead of tdFcollisionFibFib to
                        all us to support other instruments (see 
                        tdFdeltaActivate()).  For the moment, we keep the
                        call to tdFcollisoinFibFib as well as a check on 
                        the Fpil code, but only if the Macro TDF_CHK_FPIL
                        is defined.
                        Comment out variables only used by DitsPeek stuff.
      01-Feb-2000  TJF  Instead of using NUM_PIVOTS macro, use the new
                        variable numPivots, which is initialised to the
                        value returned by FpilGetNumPivots().  The dimension
                        of park lists should be FPIL_MAXPIVOTS instead
                        of NUM_PIVOTS.
      01-Feb-2000  TJF  Break tdFdeltaSequencer up into a bunch of small
                        functions to make it more readable
      01-Jun-2000  TJF  The use of the alreadyParked array is not consistent,
                        ensure that it is a count of times the fibre has
                        been parked, rather then a logical.  Changes to
                        SequencerInit and CanPark_RecordMoveUpdate.
      05-Jun-2000  TJF  We can't correctly handle ParkMayCollide and the
                        fibre bend angles have been modified to prevent it,
                        so clear the flag.
      20-Oct-2000  TJF  Pass the above item when creating command file
                        and then free it if on return or on errors.
      01-Nov-2000  TJF  Remove ability to check FPIL against old
                        tdFcollision routines.
      22-Oct-2001  TJF  Renamed notParkMoves to numUnParkedNotMovedLeft.
                        Rename parkList to numMovesPrevented and document
                        its use somewhat better.
      07-May-2002  TJF  An occurance of multiple attempts to park a fibre
                        has been seen.  Modify CouldNotMove_MustPark() to
                        report more details and prompt the user to use
                        the 2dfsave command to send the details to tjf.
      09-Apr-2003  TJF  Invesitage and fix double park problem.  Several changes
                        and lots of debugging added.
                        Drop NO_ORDER_CHECK flag (now just an error return);
      {@change entry@}
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
 *  Invoke if we find we can move a fibre directly to it's desired location.
 */
static void CanMoveDirect(
    const unsigned      curPivot,
    const SdsIdType     cmdFileId,
    const unsigned      numPivots,
    tdFinterim          * const current,
    tdFtarget           * const target,
    const tdFconstants  * const constants,
    tdFcrosses          * const crosses,
    int                 * const pivotsLeft,
    int                 * const didMove,
    unsigned            * const lineNumber,
    unsigned            * const numParks,
    unsigned            * const numMoves,
    short               * const numUnParkedNotMovedLeft,
    int                 * const pivotsMoved,
    float               * const lastUpdate,
    short               alreadyParked[],
    short               alreadyMoved[],
    StatusType          * const status)
{
    double cosT,sinT;
    unsigned j;
    FibreCross *ptmp;
    int ParkMayCollide;

    if (curPivot == 325) {
        fprintf(stderr,"My Fibre\n");
    }
    /*
     * Last chance check
     */
    checkMoveOk(curPivot, current, crosses, status);

    if (*status != STATUS__OK) return;

    /*
     * We need to determine if we have to check for collisions against
     * parked fibres.
     */
    ParkMayCollide = FpilParkMayCollide(tdFdeltaFpilInst());
    ParkMayCollide = 0;   /* Override since we can't correctly
                             handle it and the 6dF bend angles have
	                     be modified to prevent it */


    /*
     *  Update numUnParkedNotMovedLeft.
     */
    if (current->park[curPivot] == NO)
    {
#ifdef DEBUG_DELTA
        if ((curPivot == 189)||(curPivot == 192))
            fprintf(stderr,"CanMoveDirect:Decrementing UPNM, for fibre %d\n", curPivot+1);
#endif

        (*numUnParkedNotMovedLeft)--;
    }
#ifdef DEBUG_DELTA
    else
    {
        if ((curPivot == 189)||(curPivot == 192))
            fprintf(stderr,"CanMoveDirect:moving parked fibre %d\n", curPivot+1);
    }
#endif
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
    (*pivotsMoved)++;
    (*didMove) = YES;

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
        if ((current->park[j] == YES)&&(!ParkMayCollide)) continue;
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

    /*
     *  Add move to command file.
     */
    if (target->park[curPivot] == YES) {    /* To park */
#ifdef DEBUG_DELTA
        /*if ((curPivot == 189)||(curPivot == 192))*/
        fprintf(stderr,"Park fibre %d (1)\n", curPivot+1);
#endif
        if (alreadyParked[curPivot])
        {
            *status = TDFDELTA__DELTAERR;
            ErsRep(0,status,
                   "Error generating command file - attempted to park fibre %d 2 times",
               curPivot+1);
            ErsRep(0, status, "within:CanMoveDirect");
            ErsRep(0, status, "Please use the \"2dfsave\" command from the terminal window to send details of this error to support");
            ErsRep(0, status, 
                   "To get going again, first try parking fibres %d through %d (from engineering interface).", 
                   curPivot-20, curPivot+20);
            ErsRep(0, status, "Then try field again.");
            return;
            
        }
        ++alreadyParked[curPivot];
        tdFdeltaCFaddCmd (status,
                          cmdFileId,(*lineNumber)++,
                          "PF",curPivot+1);
        (*numParks)++;
    } else {                             /* From plate */
#ifdef DEBUG_DELTA
        /*if ((curPivot == 189)||(curPivot == 192))*/
            fprintf(stderr,"Move fibre %d\n", curPivot+1);
#endif
        if (alreadyMoved[curPivot])
        {
            *status = TDFDELTA__DELTAERR;
            ErsRep(0,status,
                   "Error generating command file - attempted to move fibre %d 2 times",
               curPivot+1);
            ErsRep(0, status, "Please use the \"2dfsave\" command from the terminal window to send details of this error to support");
            ErsRep(0, status, 
                   "To get going again, first try parking fibres %d through %d (from engineering interface).", 
                   curPivot-20, curPivot+20);
            ErsRep(0, status, "Then try field again.");
            return;
            
        }
        ++alreadyMoved[curPivot];
        tdFdeltaCFaddCmd (status,
                          cmdFileId,(*lineNumber)++,
                          "MF",curPivot+1,
                          target->xf[curPivot],
                          target->yf[curPivot],
                          target->theta[curPivot]);
        (*numMoves)++;
    }

    /*
     *  Set the DELTA_PROG parameter - this indicates the progress of the
     *  ordering process.
     */
    DisplayProgress(*numMoves, *numParks, *pivotsLeft, 
                        lastUpdate, status);
    if (*status != STATUS__OK) {
        ErsRep(0,status,"Error updating interim field details - %s",
               DitsErrorText(*status));
        return;
    }

}
/*
 *  We can park a fibre,  Record move and update field details.
 */
static void CanPark_RecordMoveUpdate(
    const short         parkFibre,
    const SdsIdType     cmdFileId,
    tdFinterim          * const current,
    tdFtarget           * const target,
    const tdFconstants  * const constants,
    tdFcrosses          * const crosses,
    int                 * const pivotsLeft,
    int                 * const didMove,
    unsigned            * const lineNumber,
    unsigned            * const numParks,
    short               * const numUnParkedNotMovedLeft,
    short               alreadyParked[],
    short               * const extraParks,
    StatusType          * const status)
{
    FibreCross *ptmp;

    /*
     * Last chance check
     */
    checkMoveOk(parkFibre, current, crosses, status);


    if (*status != STATUS__OK) return;
    /*
     *  Update field details.
     */
    if (target->mustMove[parkFibre] == NO) {
        target->mustMove[parkFibre] = YES;
        
        (*pivotsLeft)++;
#ifdef DELTA_DEBUG
        if ((parkFibre == 189)||(parkFibre == 192))
            fprintf(stderr,
              "CanPark_Record:Fibre %s which should not moved to be parked\n", 
                parkFibre+1);
#endif
        ++(*extraParks);
        
    } else if (target->mustMove[parkFibre] == IF_NEEDED) {
        target->mustMove[parkFibre] = NO;
#ifdef DELTA_DEBUG
        if ((parkFibre == 189)||(parkFibre == 192))
            fprintf(stderr,
             "CanPark_Record:Fibre %d which is move if needed to be parked\n", 
                parkFibre+1);
#endif
        ++(*extraParks);
    } else {
#ifdef DEBUG_DELTA
        if ((parkFibre == 189)||(parkFibre == 192))
            fprintf(stderr,"CanPark_Record:Decrementing UPNM, for fibre %d\n", 
                    parkFibre+1);
#endif
        (*numUnParkedNotMovedLeft)--;
    }
    (*didMove) = YES;
#ifdef DEBUG_DELTA
    if ((parkFibre == 189)||(parkFibre == 192))
        fprintf(stderr,
                "CanPark_Record:Incrementing alreadyParked for fibre %d\n", 
                parkFibre+1);
#endif
    ++alreadyParked[parkFibre];
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
#ifdef DEBUG_DELTA
    /*if ((parkFibre == 189)||(parkFibre == 192))*/
        fprintf(stderr,"Park fibre %d (2)\n", parkFibre+1);
#endif
    tdFdeltaCFaddCmd (status,
                      cmdFileId,(*lineNumber)++,
                      "PF",parkFibre+1);
    (*numParks)++;

}


/*
 *  We could not move a fibre.  We must park one.
 */
static void CouldNotMove_MustPark(
    const int           numMoves,
    const SdsIdType     cmdFileId,
    tdFinterim          * const current,
    tdFtarget           * const target,
    const tdFconstants  * const constants,
    tdFcrosses          * const crosses,
    int                 * const pivotsLeft,
    int                 * const didMove,
    unsigned            * const lineNumber,
    unsigned            * const numParks,
    short               * const numUnParkedNotMovedLeft,
    float               * const lastUpdate,
    short               alreadyParked[],
    short               numMovesPrevented[],
    short               * const extraParks,
    StatusType          * const status)
{
    short parkFibre;

    if (*status != STATUS__OK) return;
    /*
     *  Choose optimal fibre to park.
     */
    parkFibre = tdFdelta___DeltaChoosePark (current,
                                            crosses,
                                            target,
                                            pivotsLeft,
                                            numMovesPrevented,
                                            numUnParkedNotMovedLeft,
                                            status);

#ifdef DEBUG_DELTA
    if ((parkFibre == 189)||(parkFibre == 192))
        fprintf(stderr,"Chose to park pivot %d\n", parkFibre);
#endif

    parkFibre--;  /* array index = piv#-1 */
    if (*status != STATUS__OK) {
        ErsRep(0,status,"Error choosig fibre to park - %s",
               DitsErrorText(*status));
        return;
    } else if (alreadyParked[parkFibre] > MAX_PARKS) {
#ifdef DEBUG_DELTA
        fprintf(stderr,"ERROR, %d already parked\n", parkFibre+1);
#endif
        *status = TDFDELTA__DELTAERR;
        ErsRep(0,status,
          "Error generating command file - attempted to park fibre %d %d times",
               parkFibre+1, MAX_PARKS+1);
        ErsRep(0, status, "Number of moves prevented by this fibre = %d",
               numMovesPrevented[parkFibre]);
        ErsRep(0, status, "Number of crosses = %d, %s",
	       current->nAbove[parkFibre], 
               (crosses->above ? "list exists": "list empty"));
        ErsRep(0, status, "Please use the \"2dfsave\" command from the terminal window to send details of this error to support");
        ErsRep(0, status, 
               "To get going again, first try parking fibres %d through %d (from engineering interface).", 
               parkFibre-20, parkFibre+20);
        ErsRep(0, status, "Then try field again.");
        return;
    }

    /*
     *  Can park button - record move and update field details.
     */
    else {
        CanPark_RecordMoveUpdate(parkFibre,
                                 cmdFileId,
                                 current,
                                 target,
                                 constants,
                                 crosses,
                                 pivotsLeft,
                                 didMove,
                                 lineNumber,
                                 numParks,
                                 numUnParkedNotMovedLeft,
                                 alreadyParked,
                                 extraParks,
                                 status);

        DisplayProgress(numMoves, *numParks, *pivotsLeft, 
                        lastUpdate, status);
        if (*status != STATUS__OK) {
            ErsRep(0,status,
                   "Error updating interim field details - %s",
                   DitsErrorText(*status));
            return;
        }           
    }
}

/*
 *  Search for a fibre to move.
 */
static void SearchForMove(
    const unsigned      numPivots,
    const SdsIdType     cmdFileId,
    tdFdeltaType        * const data,
    int                 * const numMoves,
    int                 * const pivotsLeft,
    int                 * const didMove,
    int                 * const pivotsMoved,
    unsigned            * const lineNumber,
    unsigned            * const numParks,
    short               * const numUnParkedNotMovedLeft,
    float               * const lastUpdate,
    short               numMovesPrevented[],
    short               alreadyParked[],
    short               alreadyMoved[],
    StatusType          * const status)
{
    register unsigned i;
    if (*status != STATUS__OK) return;
    /*
     *  Reset the park candidate list and didMove flag.
     */
    for (i=0; i< numPivots; i++) numMovesPrevented[i] = 0;
    (*didMove) = NO;

    /*
     *  Sequentially check each fibre to see if it can be moved directly
     *  to its target position.
     */
    for (i=0; i< numPivots; i++) {

        short offendingPivot;
        /*
         *  Only check fibres that need to be moved.
         */
        if (data->target.mustMove[i] != YES)  
        {
#ifdef DEBUG_DELTA2
            if ((i == 189)||(i == 192))
                fprintf(stderr,"fibre %d, mustMove != YES\n", i+1);
#endif
            continue;
        }

        /*
         *  Make sure that all fibres that are not in their park positions and
         *  and need to be moved are moved before any fibres in their park
         *  positions are.
         */
        if ((data->current.park[i] == YES) && 
            ((*numUnParkedNotMovedLeft) > 0)) 
        {
#ifdef DEBUG_DELTA
            if ((i == 189)||(i == 192))
                fprintf(stderr,
                        "fibre %d, park = YES, && numUnParkedNotMoved\n", 
                        i+1);
#endif
            continue;
        }

        /*
         *  If button can be moved directly from current to target position,
         *  record the move and update the field.  If it can't, record the number
         *  of the pivot that prevented it from moving.
         */
        offendingPivot = tdFdelta___DeltaDirectMove (&data->current,
                                                     &data->crosses,
                                                     &data->target,
                                                     &data->constants,
                                                     data->butClearG,
                                                     data->butClearO,
                                                     data->fibClearG,
                                                     data->fibClearO,
                                                     i, status);
        if (offendingPivot)
        {
#ifdef DEBUG_DELTA
            if ((i == 189)||(i == 192))
                fprintf(stderr,"Pivot %d prevents %d being moved (%s)\n", 
                        offendingPivot, i+1,
                     (data->current.nAbove[i] > 0 ? "crossed" : "collision" ));
#endif            
            numMovesPrevented[offendingPivot-1]++;  /* array index = piv#-1 */
        }
        else if (*status != STATUS__OK) {
            SdsDelete (cmdFileId,status);
            SdsFreeId (cmdFileId,status);
            free((void *)data);
            return;
        }

        /*
         *  Can move button directly to target position.
         */
        else {
#ifdef DEBUG_DELTA
            if ((i == 189)||(i == 192))
                fprintf(stderr, "Can move %d directly\n", i+1);
#endif
            /*
             *  First check that pivot needs to be moved.
	     *
             *  THIS CODE MAY BE WRONG BUT IS PROBABLY NOT BEING TRIGGERED
             *  as the tdFpt task will only mark a fibre as requiring a move
             *  if it is not already in position within tolerance.  This code
             *  does not decrement notPakesMoves which is may need to do - we
             *  are unclear on this.
             */
            if ((data->current.theta[i] == data->target.theta[i]) &&
                (data->current.xf[i]    == data->target.xf[i]) &&
                (data->current.yf[i]    == data->target.yf[i])) {
                (*didMove) = YES;
                data->target.mustMove[i] = NO;
                (*pivotsLeft)--;
                continue;
            }

            CanMoveDirect(i,
                          cmdFileId,
                          numPivots,
                          &data->current,
                          &data->target,
                          &data->constants,
                          &data->crosses,
                          pivotsLeft,
                          didMove,
                          lineNumber,
                          numParks,
                          numMoves,
                          numUnParkedNotMovedLeft,
                          pivotsMoved,
                          lastUpdate,
                          alreadyParked,
                          alreadyMoved,
                          status);

            if (*status != STATUS__OK) {
                SdsDelete (cmdFileId,status);
                SdsFreeId (cmdFileId,status);
                free((void *)data);
                return;
            }

        }
    }
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
    short               * const numUnParkedNotMovedLeft,
    short               alreadyParked[],
    short               numMovesPrevented[],
    short               alreadyMoved[],
    StatusType * const status)
{
    register unsigned i;

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
    } else if (data->check & NO_ORDER_CHECK) {
        *status = TDFDELTA__INVARG;
        ErsRep(0, status, "Delta no longer supports NO_ORDER_CHECK flag");
        goto ERROR_RETURN;
    } else
        MsgOut(status,"Performing delta (ordering) process...");

    /*
     *  Open a new command file.
     */
#ifdef DEBUG_DELTA
    fprintf(stderr,"\n**************************\n");
    fprintf(stderr,"Delta starting, command file name %s\n", data->name);
#endif
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
#ifdef DEBUG_DELTA
        fprintf(stderr, "Pivot %.3d, mustMove=%s, currentlyParked=%s",
                i+1,
                (data->target.mustMove[i] == YES ? "Yes" :
                    (data->target.mustMove[i] == IF_NEEDED ? "IFN" : "No ")),
                (data->current.park[i] == YES ? "Yes" : "No "));
        if (data->current.nAbove[i] > 0)
        {
            fprintf(stderr, " crossed by %.2d, being ", 
                    data->current.nAbove[i]);
            
            if (data->crosses.above[i])
            {
                FibreCross *ptmp = data->crosses.above[i];
                while (ptmp)
                {
                    fprintf(stderr, "%.3d ", ptmp->piv);
                    ptmp = ptmp->next;
                }
            }
            else
            {
                fprintf(stderr, "INVALID CROSS LIST");
            }
        }
        fprintf(stderr,"\n");

#endif
        if (data->target.mustMove[i] == YES) {
            if (data->current.park[i] == NO)
                (*numUnParkedNotMovedLeft)++;
            (*pivotsLeft)++;
        }
        numMovesPrevented[i] = 0;
        alreadyParked[i] = 0;
        alreadyMoved[i] = 0;
    }
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

TDFDELTA_INTERNAL void  tdFdeltaSequencer (
        StatusType  *status)
{
    tdFdeltaType  *data = DitsGetActData();/* Function parameters            */
    SdsIdType     cmdFileId;        /* Command file Id (Sds structure id)    */
    time_t        tStart, tEnd;     /* Used for timing this function         */
    static float  lastUpdate;       /* DELTA_PROG at last update             */
    int           pivotsLeft = 0,   /* Number of pivots left to move         */
                  pivotsMoved = 0,  /* Number of pivots moved to target pos. */
                  didMove = YES;    /* Flag indicating that fibre was moved  */
    unsigned      lineNumber = 1,   /* Counters                              */
                  numMoves = 0,     /* Final number of pivots to be moved    */
                  numParks = 0;     /* Final number of pivots to be parked   */
    short         extraParks = 0;  
    /*
     * This array holds for each fibre, the number of times it has
     * prevented another fibre from being moved.
     */

    short numMovesPrevented[FPIL_MAXPIVOTS];

    short alreadyMoved[FPIL_MAXPIVOTS];
    short alreadyParked[FPIL_MAXPIVOTS];/*Flag each pivot when parked*/
    short numUnParkedNotMovedLeft = 0; /* Number of un-parked pivots not 
                                          moved  */
    unsigned numPivots;                /* Number of pivots          */

    if (*status != STATUS__OK) return;

    /*
     * Initialise this function's variables etc.
     */
    if (!SequencerInit(data,&numPivots, &tStart, &lastUpdate, &cmdFileId,
                       &pivotsLeft, &numUnParkedNotMovedLeft, alreadyParked,
                       numMovesPrevented, alreadyMoved, status))
        return;

    /*
     *  Begin main loop.
     *
     *  Loop consists of two parts:-
     *  A - Sequentially check each of the numPivots fibres to see if they 
     *      can be moved directly from their current to target position.   
     *      If possible - record the move and update the interim field 
     *      details to reflect the move.
     *
     *      Repeat this process until either all the fibres have been moved, 
     *      or it is not possible to move any fibres (see part B).
     *
     *  B - Choose the optimum fibre to return to its park position, and then
     *      repeat part A.
     *
     *  Assuming that the target field is a valid configuration, this will 
     *  always produce a sequence of moves to change from the current 
     *  configuration to the target configuration.  The number of moves in 
     *  this sequence can be minimised by choosing the optimum fibre to park 
     *  (and perhaps later by also optimising the gripper gantry movement 
     *  between pivot moves).
     */
    while (pivotsLeft) {

#ifdef DEBUG_DELTA
    fprintf(stderr,"\n-------- N e x t    P a s s -----------------\n");
    fprintf(stderr,"Pivots Left = %d, didMove = %s, UPNM = %d EP = %d\n", 
            pivotsLeft,
            (didMove ? "YES" : "NO"), numUnParkedNotMovedLeft, extraParks);
#endif
        /*
         *  Search for pivot to move directly from current to target position.
         */
        if (didMove) {
            SearchForMove(numPivots,
                          cmdFileId,
                          data,
                          &numMoves,
                          &pivotsLeft,
                          &didMove,
                          &pivotsMoved,
                          &lineNumber,
                          &numParks,
                          &numUnParkedNotMovedLeft,
                          &lastUpdate,
                          numMovesPrevented,
                          alreadyParked,
                          alreadyMoved,
                          status);
            if (*status != STATUS__OK)
                return;
         } /* didMove*/
        /*
         *  Could not move any fibre directly to target pos - must park a fibre.
         */
        else {

            CouldNotMove_MustPark(numMoves,
                                  cmdFileId,
                                  &data->current,
                                  &data->target,
                                  &data->constants,
                                  &data->crosses,
                                  &pivotsLeft,
                                  &didMove,
                                  &lineNumber,
                                  &numParks,
                                  &numUnParkedNotMovedLeft,
                                  &lastUpdate,
                                  alreadyParked,
                                  numMovesPrevented,
                                  &extraParks,
                                  status);
            
            if (*status != STATUS__OK) {
                SdsDelete (cmdFileId,status);
                SdsFreeId (cmdFileId,status);
                free((void *)data);
                return;
            }
                
            

        } /* !didMove */
    } /* while pivotsLeft */
#ifdef DEBUG_DELTA
    fprintf(stderr,"Delta Complete, moves = %d, parks = %d\n",
            numMoves, numParks);
    fprintf(stderr,"=====================================\n");
    {
        unsigned i;
        for (i = 0; i < numPivots ; ++i)
        {
            if (alreadyMoved[i] > 1) 
            {
                fprintf(stderr,"Fibre %d moved twice\n", i+1);
            }
        }       
    }
#endif
    /*
     *  Record the number of moves and parks in the command file.
     */
    tdFdeltaCFaddMoves (cmdFileId,(long int)numMoves,(long int)numParks,status);

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
