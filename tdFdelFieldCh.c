/*+                T D F D E L T A

 *  Module name:
      tdFdeltaFieldCheck

 *  Function:
      Check the validity of a field configuration.

 *  Description:
      Before any moves are performed, their validity must be checked.  This 
      is the module that performs that checking.

      This module can be bypassed by including the NO_FIELD_CHECK flag in 
      the action parameter structure.

 *  Language:
      C

 *  Support: James Wilcox, AAO

 *-

 *  History:
      01-Jul-1994  JW   Original version
      01-Nov-2000  TJF  Remove ability to check FPIL against old
                        tdFcollision routines.
      23-Mar-2006  TJF  MsgOut messages on fibre collisions etc now
                         have WARNING prefixed to they are shown in
                         yellow on 2dF interface.
      {@change entry@}


 *     @(#) $Id: ACMM:2dFdelta/tdFdelFieldCh.c,v 3.15 10-Sep-2013 08:31:15+10 tjf $
 */

/*
 *  Include files
 */


static char *rcsId="@(#) $Id: ACMM:2dFdelta/tdFdelFieldCh.c,v 3.15 10-Sep-2013 08:31:15+10 tjf $";
static void *use_rcsId = (0 ? (void *)(&use_rcsId) : (void *) &rcsId);


#include "DitsTypes.h"       /* Basic dits types            */
#include "Dits_Err.h"        /* Dits error codes            */
#include "DitsSys.h"         /* For PutActionHandlers       */
#include "DitsFix.h"         /* For various Dits routines   */
#include "DitsMsgOut.h"      /* For MsgOut                  */
#include "DitsParam.h"       /* For DitsGetParId            */
#include "DitsPeek.h"        /* For DitsPeek                */
#include "DitsUtil.h"        /* For DitsErrorText           */
#include "mess.h"            /* For MessPutFacility         */
#include "sds.h"             /* For SDS_ macros             */
#include "arg.h"             /* For ARG_ macros             */
#include "Ers.h"
#include "status.h"          /* STATUS__OK definition       */

#include "tdFdelta.h"
#include "tdFdelta_Err.h"

#include <stdio.h>
#include <string.h>
#include <math.h>


/*+        T D F D E L T A F I E L D C H E C K

 *  Function name:
      tdFdeltaFieldCheck

 *  Function:
      Checks the validity of a new field configuration, ie, it checks the 
      validity of all proposed moves.  Buttons that do not have to be moved 
      are not checked (if they are not being moved they must already be at 
      their target positions and therefore these must be valid locations!).

 *  Description:
      The checks that are performed are -

        - no button/button collisions
        - no button/fibre collisions
        - maximum fibre extension is not exceeded
        - maximum button/fibre bend angle is not exceeded
        - maximum pivot/fibre bend angle is not exceeded
        - position is a within field plate limits
        - button is not positioned on screw hole

      These checks are only performed on buttons that are flagged as needing 
      to be moved (as if a button does not need to be moved, it must 
      already be there, hence its position must be a valid one!).

 *  Language:
      C

 *  Call:
      (void) = tdFdeltaFieldCheck (status)

 *  Parameters:  (">" input, "!" modified, "W" workspace, "<" output)
      (!) status      (StatusType *)    Modified status.

 *  Proir Requirements:
      DitsGetActData() must return a pointer to a structure of type 
      tdFdeltaType (see tdFdelta.h for structure details).

 *  Support: James Wilcox, AAO

 *-

 *  History:
      01-Jul-1994  JW   Original version
      22-Nov-1996  TJF  Final show of fiducials status was incorrectly checking
			for out of use fiducials. 
      26-Feb-1998  TJF  Was not correctly checking all cases of button/fibre
                        and button/button collision, in particular, when
                        pivot being moved was greater in number then pivot
                        we are checking against, and pivot we are checking
                        against was not being moved.
      19-May-1998  TJF  Missed one "continue" statement in the above fix.
      17-Jan-2000  TJF  Comment out DitsPeek calls to avoid a DRAMA bug,
			as we don't really need to do it anyway.
      13-Jan-2000  TJF  Comment out variables used only by DitsPeek.
      31-Jan-2000  TJF  Use FpilCol* instead of tdFcollisionFib* to
                        all us to support other instruments (see
                        tdFdeltaActivate()).  For the moment, we keep the
                        call to tdFcollisoin* as well as a check on
                        the Fpil code, but only if the Macro TDF_CHK_FPIL
                        is defined.
      01-Feb-2000  TJF  Instead of using NUM_PIVOTS macro, use the new
                        variable numPivots, which is initialised to the
                        value returned by FpilGetNumPivots().
      04-Feb-2000  TJF  Break into a bunch of small functions to make
                        it more readable.  
                        
                        Remove all the peak releated code.  

                        We now need three unobstructed fiduicals, so
                        that a survey (rather then a shift-coeffs) can be
                        performed.   

                        Remove fieldOK variable, the numErrors variable will 
                        do that job.

                        Rename the i and j variables throughout with more
                        descriptive names.
      
     
      07-Feb-2000  TJF  The IF_NEEDED value for the flag mustMove was not
                        being considered correctly when deciding if we check
                        a fibre and the default was to consider it as per
                        YES, when we should have been considering it as
                        per the NO value.  Change all occurances of

                            (target->mustMove[firstPivot] == NO)) 
                        to
                            (target->mustMove[firstPivot] != YES)) 

                        And occurances of
                             ((target->mustMove[otherPivot] == YES)||

                        to

                             ((target->mustMove[otherPivot] == YES)||
                             (target->mustMove[otherPivot] == IF_NEEDED)||
                

      17-Mar-2000  TJF  Use FPIL to get the number of fiducials rather then
                        the macro NUM_FIDUCIALS.  Use FPIL_MAXFIDS for
                        static array declarations.

      26-Apr-2000  TJF  FpilColInvPos needs the plate number (which we don't
                        have available at the moment).  Dummy it to use
                        plate 0 all the time, which is safe for 2dF and
                        6dF. (OzPoz will have to set it).
      26-Apr-2000  TJF  In 6dF, a fibre may collide with the part position
                        of another fibre.  We use FpilParkMayCollide() to
                        determine if we are dealing with such an instrument
                        and in these cases, will check against park positions.
      03-May-2000 TJF   CheckBendAngles should not check the bend angles
                        at the button unless it may collide.
      20-Oct-2000 TJF   Before deleting the tdFdeltaType data item, we
                        must now free the above Sds item within it.
      01-Nov-2000 TJF   Remove ability to check FPIL against old
                        tdFcollision routines. 
      01-Nov-2000 TJF   Handle the SPECIAL flag to invoke 
                        tdFdeltaSequencerSpecial() rather then 
                        tdFdeltaSequencer()
      13-Feb-2001 TJF   Maximum fibre extension is now fibre specific (for
                        6dF case).
      19-Sep-2002 TJF   Insert of using the park theta as the line taken
                        from the fibre to the center of the plate, calculate
                        it - to ensure it is right.  Add FibreAngle function.
      20-Aug-2009 TJF   Add Fibre Type argument to  FpilColInvPos() call, 
                         but we don't need provide any real value for 2dF/6dF 
                         and it is just set to zero.
      10-Sep-2013 TJF  Robot contants file is now named tdFconstantsDF.sds.
      {@change entry@}
 */



/*
 *  Check for collisions between buttons.
 */
static void CheckForButButCollisions(
    const FpilType      inst,
    const unsigned      actionFlags,
    const unsigned      numPivots,
    const long int      butClearO,
    const long int      butClearG,
    const short         type[],
    const tdFtarget     * const target,
    const tdFconstants  * const constants,
    unsigned            * const numErrors,
    StatusType * const status)
{
    register unsigned firstPivot;
    int ParkMayCollide;

    if (*status != STATUS__OK) return;

    if (actionFlags & SHOW)
        MsgOut(status,"...checking for button/button collisions");
    /*
     * We need to determine if we have to check for collisions against
     * parked fibres.
     */
    ParkMayCollide = FpilParkMayCollide(inst);

    for (firstPivot=0; firstPivot < numPivots; firstPivot++) {

        register unsigned otherPivot;
        int firstPivotX;
        int firstPivotY;
        double firstPivotTheta;
        /*
         *  Only check buttons that have to move unless otherwise specified.
         */
        if (!(actionFlags & CHECK_FULL_FIELD) &&
            (target->mustMove[firstPivot] != YES)) continue;

        /*
         * For non-parked fibres, check against target position.
         * 
         * If park positions may collide, check against parked positions.
         *
         * Otherwise, park position is always save, no need to check it.
         */
        
        if (target->park[firstPivot] != YES) {
            firstPivotX     = target->xf[firstPivot];
            firstPivotY     = target->yf[firstPivot];
            firstPivotTheta = target->theta[firstPivot];
        } else if (ParkMayCollide) {
            firstPivotX     = constants->xPark[firstPivot];
            firstPivotY     = constants->yPark[firstPivot];
            firstPivotTheta = constants->tPark[firstPivot];
        } else {
            continue;
        }
            

        /*
         *  Check against each of the other buttons.
         */
        for (otherPivot=0; otherPivot < numPivots; otherPivot++) {

            int flag;
            long int buttonClear;
            int otherPivotX;
            int otherPivotY;
            double otherPivotTheta;            
            /*
             *  Do not check a button against itself or a button that it 
             *  has already been checked against. 
             *  Note, we have only checked against buttons which must be
             *  moved, unless CHECK_FULL_FIELD is enabled.
             */
            if (firstPivot == otherPivot) 
                continue;
            else if ((firstPivot > otherPivot )&&
                     ((target->mustMove[otherPivot] == YES)||
                      (target->mustMove[otherPivot] == IF_NEEDED)||
                      (actionFlags & CHECK_FULL_FIELD)))
                continue;

            /*
             * For non-parked fibres, check against target position.
             * 
             * If park positions may collide, check against parked positions.
             *
             * Otherwise, park position is always save, no need to check it.
             */
            if (target->park[otherPivot] != YES) {
                otherPivotX     = target->xf[otherPivot];
                otherPivotY     = target->yf[otherPivot];
                otherPivotTheta = target->theta[otherPivot];
            } else if (ParkMayCollide) {
                otherPivotX     = constants->xPark[otherPivot];
                otherPivotY     = constants->yPark[otherPivot];
                otherPivotTheta = constants->tPark[otherPivot];
            } else {
                continue;
            }
            
            /*
             *  Check button firstPivot against button otherPivot for collision.
             */
            if ((type[firstPivot] == GUIDE) || 
                (type[otherPivot] == GUIDE))
                buttonClear = butClearG;
            else
                buttonClear = butClearO;

            FpilSetButClear(inst, buttonClear);
            flag = FpilColButBut(inst,
                                 firstPivotX, firstPivotY, firstPivotTheta,
                                 otherPivotX, otherPivotY, otherPivotTheta);

            if (flag == YES) {
                MsgOut(status,
           "WARNING:Button/button collision detected in target field (but=%d,%d)",
                       firstPivot+1,otherPivot+1);
                (*numErrors)++;
            }
        }
    } 
   
}

/*
 *  Check for collisions between buttons and fibres.
 */
static void CheckForButFibCollisions(
    const FpilType      inst,
    const unsigned      actionFlags,
    const unsigned      numPivots,
    const long int      fibClearO,
    const long int      fibClearG,
    const tdFconstants  * const constants,
    const tdFtarget     * const target,
    unsigned            * const numErrors,
    StatusType * const status)
{
    register unsigned firstPivot;
    int ParkMayCollide;

    if (*status != STATUS__OK) return;

    /*
     * We need to determine if we have to check for collisions against
     * parked fibres.
     */
    ParkMayCollide = FpilParkMayCollide(inst);


    if (actionFlags & SHOW)
        MsgOut(status,"...checking for button/fibre collisions");
    for (firstPivot=0; firstPivot < numPivots; firstPivot++) {

        register unsigned otherPivot;
        int firstPivotX;
        int firstPivotY;
        double firstPivotTheta;

        /*
         *  Only check buttons that have to move unless otherwise specified.
         */
        if (!(actionFlags & CHECK_FULL_FIELD) &&
            (target->mustMove[firstPivot] != YES)) continue;


        /*
         * For non-parked fibres, check against target position.
         * 
         * If park positions may collide, check against parked positions.
         *
         * Otherwise, park position is always safe, no need to check it.
         */
        
        if (target->park[firstPivot] != YES) {
            firstPivotX     = target->xf[firstPivot];
            firstPivotY     = target->yf[firstPivot];
            firstPivotTheta = target->theta[firstPivot];
        } else if (ParkMayCollide) {
            firstPivotX     = constants->xPark[firstPivot];
            firstPivotY     = constants->yPark[firstPivot];
            firstPivotTheta = constants->tPark[firstPivot];
        } else {
            continue;
        }

        /*
         *  Check against all other buttons.
         */
        for (otherPivot=0; otherPivot < numPivots; otherPivot++) {

            int flag;
            unsigned  fibreClear;
            int otherPivotX;
            int otherPivotY;
            double otherPivotTheta;            
            /*
             *  Do not check a button against itself or a button that it 
             *  has already been checked against. 
             *  Note, we have only checked against buttons which must be
             *  moved, unless CHECK_FULL_FIELD is enabled.
             */
            if (firstPivot == otherPivot) 
                continue;
            else if ((firstPivot > otherPivot )&&
                     ((target->mustMove[otherPivot] == YES)||
                      (target->mustMove[otherPivot] == IF_NEEDED)||
                      (actionFlags & CHECK_FULL_FIELD)))
                continue;
 

            /*
             * For non-parked fibres, check against target position.
             * 
             * If park positions may collide, check against parked positions.
             *
             * Otherwise, park position is always save, no need to check it.
             */
            if (target->park[otherPivot] != YES) {
                otherPivotX     = target->xf[otherPivot];
                otherPivotY     = target->yf[otherPivot];
                otherPivotTheta = target->theta[otherPivot];
            } else if (ParkMayCollide) {
                otherPivotX     = constants->xPark[otherPivot];
                otherPivotY     = constants->yPark[otherPivot];
                otherPivotTheta = constants->tPark[otherPivot];
            } else {
                continue;
            }

            /*
             *  Check button firstPivot against fibre otherPivot for collision.
             */
            if (constants->type[otherPivot] == GUIDE)
                fibreClear = fibClearG;
            else
                fibreClear = fibClearO;
            
            FpilSetFibClear(inst, fibreClear);

            flag = FpilColButFib(inst,
                                 firstPivotX, firstPivotY, firstPivotTheta,
                                 (double)target->fvpX[otherPivot],
                                 (double)target->fvpY[otherPivot],
                                 (double)constants->xPiv[otherPivot],
                                 (double)constants->yPiv[otherPivot]);


            if (flag == YES) {
                MsgOut(status,
            "WARNING:Button/Fibre collision detected in target field (but=%d,fib=%d)",
                       firstPivot+1,otherPivot+1);
                (*numErrors)++;
            }

            /*
             *  Check button otherPivot against fibre firstPivot for collision.
             */
            if (constants->type[firstPivot] == GUIDE)
                fibreClear = fibClearG;
            else
                fibreClear = fibClearO;
            FpilSetFibClear(inst, fibreClear);
            
            flag = FpilColButFib(inst,
                                 otherPivotX, otherPivotY, otherPivotTheta,
                                 (double)target->fvpX[firstPivot],
                                 (double)target->fvpY[firstPivot],
                                 (double)constants->xPiv[firstPivot],
                                 (double)constants->yPiv[firstPivot]);


            if (flag == YES) {
                MsgOut(status,
         "WARNING:Button/fibre collision detected in target field (but=%d,fib=%d)",
                       otherPivot+1,firstPivot+1);
                (*numErrors)++;
            }
        }
    }


}


/*
 *  Check the fibre lenghts.
 */
static void CheckFibreExtension(
    const tdFconstants  * const constants,
    const unsigned      actionFlags,
    const unsigned      numPivots,
    const tdFtarget     * const target,
    unsigned            * const numErrors,
    StatusType * const status)
{
    register unsigned pivot;

    if (actionFlags & SHOW)
        MsgOut(status,"...checking fibre extensions");

    for (pivot=0; pivot < numPivots; pivot++) {

        /*
         *  Only check buttons that have to move unless otherwise specified.
         */
        if (!(actionFlags & CHECK_FULL_FIELD) &&
            (target->mustMove[pivot] != YES)) continue;

        /*
         *  Park position is always safe - no need to check it.
         */
        if (target->park[pivot] == YES) continue;

        /*
         *  Check fibre extensions.
         */
        if (target->fibreLength[pivot] > constants->maxExt[pivot]) {
            MsgOut(status,
                   "WARNING:Maximum fibre length exceeded (%ld) in target field (piv=%d, proposed length = %ld)",
                   constants->maxExt[pivot],
                   pivot+1,
                   (long)target->fibreLength[pivot]);
            (*numErrors)++;
        }
    }

}
/*
 * Calculate the ngle of a fibre about the pivot point - in the 2dF style.
 *
 * theta=0 @ 12o'clock, increasing anti-clockwise).
 *
 */
static double FibreAngle(
    const long TargetX,
    const long TargetY,
    const long PivotX,
    const long PivotY)
{
    double thetaFib;
    double dx;
    double dy;
    double tmpdx;
    double tmpdy;

    dx = TargetX - PivotX;
    dy = TargetY - PivotY;
 
    tmpdx = (dx < 0)? -dx: dx;
    tmpdy = (dy < 0)? -dy: dy;

    if (dx>0 && dy==0)
        thetaFib = 3*PI/2;
    else if (dx>0 && dy>0)
        thetaFib = 3*PI/2 + atan((double)tmpdy/(double)tmpdx);
    else if (dx==0 && dy>0)
        thetaFib = 0;
    else if (dx<0 && dy>0)
        thetaFib = PI/2 - atan((double)tmpdy/(double)tmpdx);
    else if (dx<0 && dy==0)
        thetaFib = PI/2;
    else if (dx<0 && dy<0)
        thetaFib = PI/2 + atan((double)tmpdy/(double)tmpdx);
    else if (dx==0 && dy<0)
        thetaFib = PI;
    else /* (dx>0 && dy<0) */
        thetaFib = 3*PI/2 - atan((double)tmpdy/(double)tmpdx);

    return thetaFib;
}

/*
 *  Check the fibre bend angles.
 */
static void CheckBendAngles(
    const FpilType      inst,
    const unsigned      actionFlags,
    const unsigned      numPivots,
    const double        maxButAngO,
    const double        maxButAngG,
    const double        maxPivAngO,
    const double        maxPivAngG,
    const tdFconstants  * const constants,
    const tdFtarget     * const target,
    unsigned            * const numErrors,
    StatusType * const status)
{
    register unsigned pivot;
    if (*status != STATUS__OK) return;

    if (actionFlags & SHOW)
        MsgOut(status,"...checking button/fibre and pivot/fibre bend angles");
    for (pivot=0; pivot< numPivots ; pivot++) {

        double thetaButFib;            /* Angle between button and fibre    */
        double thetaPivFib;            /* Angle between pivot and fibre     */
        double thetaFib;               /* Angle of fibre about piv point    */
        double maxButFibAngle;         /* Maximum allowed but/fib bend ang  */
        double maxPivFibAngle;         /* Maximum allowed piv/fib bend ang  */

        /*
         *  Only check buttons that have to move unless otherwise specified.
         */
        if (!(actionFlags & CHECK_FULL_FIELD) &&
            (target->mustMove[pivot] != YES)) continue;

        /*
         *  Park position is always safe - no need to check it.
         */
        if (target->park[pivot] == YES) continue;

        /*
         * Get the fibre bend angle.
         */
        thetaFib = FibreAngle(target->fvpX[pivot],
                              target->fvpY[pivot],
                              constants->xPiv[pivot],
                              constants->yPiv[pivot]);
        
        /*
         *  Calulate fibre/button fibre/pivot angles.
         */
        thetaButFib = thetaFib - target->theta[pivot] - PI;
        thetaPivFib = thetaFib -  FibreAngle(0,
                                             0,
                                             constants->xPiv[pivot],
                                             constants->yPiv[pivot]);

        /*
         *  Get absolute value of angles, between 0 and 2PI.
         */
        thetaButFib = thetaButFib<0 ? -thetaButFib: thetaButFib;
        while (thetaButFib > PI) {
            thetaButFib -= (2*PI);
            thetaButFib = thetaButFib<0 ? -thetaButFib: thetaButFib;
        }
        thetaPivFib = thetaPivFib<0 ? -thetaPivFib: thetaPivFib;
        while (thetaPivFib > PI) {
            thetaPivFib -= (2*PI);
            thetaPivFib = thetaPivFib<0 ? -thetaPivFib: thetaPivFib;
        }

        /*
         *  Compare bend angles against maximum values.
         */
        if (constants->type[pivot] == GUIDE) {
            maxButFibAngle = maxButAngG;
            maxPivFibAngle = maxPivAngG;
        } else {
            maxButFibAngle = maxButAngO;
            maxPivFibAngle = maxPivAngO;
        }
        /*
         * Only bother doing this check if the button angle may vary.
         * (I am not sure why this was picking up a problem in
         *  6dF when it was not there, it suggests the above 
         *  calculation is not right  TJF, 3-May-2000)
         */
        if (FpilGetFibAngVar(inst)) {
            if (thetaButFib > maxButFibAngle) {
                MsgOut(status,
           "WARNING:Out of range %s angle detected in target field (piv=%d,ang=%.3f)",
                       "button/fibre",pivot+1,thetaButFib*180/PI);
                (*numErrors)++;
            }
        }
        if (thetaPivFib > maxPivFibAngle) {
            MsgOut(status,
            "WARNING:Out or range %s angle detected in target field (piv=%d,ang=%.3f)",
                   "pivot/fibre",pivot+1,thetaPivFib*180/PI);
            (*numErrors)++;
        } 
    }

}


/*
 *  Check fibres are in valid positions.
 */
static void CheckValidFieldPosition(
    const FpilType      inst,
    const unsigned      actionFlags,
    const unsigned      numPivots,
    const tdFtarget     * const target,
    unsigned            * const numErrors,
    StatusType * const status)
{
    register unsigned pivot;
    if (*status != STATUS__OK) return;

    if (actionFlags & SHOW)
        MsgOut(status,
               "...checking if target location is valid field plate position");

    for (pivot=0; pivot < numPivots; pivot++) {
        int  obstructed;

        /*
         *  Only check buttons that have to move unless otherwise specified.
         */
        if (!(actionFlags & CHECK_FULL_FIELD) &&
            (target->mustMove[pivot] != YES)) continue;

        /*
         *  Park position is always safe - no need to check it.
         */
        if (target->park[pivot] == YES) continue;

        /*
         *  Currently just check if within field radius.
         */
        if (!FpilOnField(inst,
                         target->xf[pivot],
                         target->yf[pivot])) {
            MsgOut(status,
              "WARNING:Button outside usable field plate area and not parked (piv=%d)",
               pivot+1);
            (*numErrors)++;
        }

        /*
         *  Check that the button is not on one of the screws.
         *  WARNING. ASSUMING PLATE IS ZERO and Fibre type is ZERO. 
         *    MUST BE FIXED IN LONG TERM
         *     but does not affect 2dF or 6dF.
         */
        obstructed = FpilColInvPos(inst, 0, 0,
                                   target->xf[pivot],
                                   target->yf[pivot],
                                   target->theta[pivot]);

        
        if (obstructed == YES) {
            MsgOut(status,
               "WARNING:Button/screw-hole collision detected in target field (but=%d)",
               pivot+1);
            (*numErrors)++;
        }
    }



}



/*
 *  Check that we have at least three unobscured fiducials
 */ 
static void CheckFiducials(
    const FpilType      inst,
    const unsigned      actionFlags,
    const unsigned      numPivots,
    const tdFconstants  * const constants,
    const tdFtarget     * const target,
    const tdFfiducials  * fids,
    unsigned            * const numErrors,
    StatusType * const status)
{
    register unsigned fiducial;
    long int  fidx, fidy;
    short numUnObstructed = 0;
    short fidFlags[FPIL_MAXFIDS];
    unsigned NumFids = FpilGetNumFiducials(inst);

 
    if (*status != STATUS__OK) return;

    if (actionFlags & SHOW)
        MsgOut(status,
               "...checking that not all fiducial marks will be obstructed");
 
    for (fiducial=0; fiducial < NumFids; fiducial++) {
        int    obstructed;
        unsigned pivot;

        if (fids->inUse[fiducial] == 0)  continue;

        fidx = fids->fidX[fiducial];
        fidy = fids->fidY[fiducial];

        /*
         *  Set the flag for this fiducial to unobstructed (if the fiducial is
         *  found to be obstructed then this flag will be set to the number of
         *  the offending fibre/button).
         */
        fidFlags[fiducial] = 0;

        /*
         *  Check fiducial against each button/fibre.
         */
        for (pivot=0; pivot < numPivots; pivot++) {

            /*
             *  Button in park position can not obstruct fiducial.
             */
            if (target->park[pivot] == YES) continue;

            /*
             *  Check for fibre/fiducial collision.
             */
            obstructed = FpilColFiducial(inst,
                                         target->xf[pivot],
                                         target->yf[pivot],
                                         target->theta[pivot],
                                         constants->xPiv[pivot],
                                         constants->yPiv[pivot],
                                         target->fvpX[pivot],
                                         target->fvpY[pivot],
                                         fidx,fidy);

            if (obstructed) {
                fidFlags[fiducial] = pivot+1;
                break;
            }
        }

        /*
         *  Check if the fiducial was obstructed.
         */
        if (fidFlags[fiducial] == 0)  numUnObstructed++;
    }

    if (numUnObstructed <= 2) {
        if (numUnObstructed == 0) {
            MsgOut(status, "WARNING:All fiducials are obstructed in target field");
            MsgOut(status, "  We must have three unobstructed fiducials");
        } else {
            MsgOut(status,
      "WARNING:Target field does not have enough unobstructed fiducials for a survey");
            MsgOut(status, "  We have %d of the three needed for a survey",
                   numUnObstructed);
        }
        (*numErrors)++;
    }

    if (actionFlags & SHOW) {
        for (fiducial=0; fiducial < NumFids; fiducial++) {
            if (fids->inUse[fiducial] == 0)
                MsgOut(status,"Fiducial %d NOT IN USE",fiducial+1);
            else if (fidFlags[fiducial] == 0)
                MsgOut(status,"Fiducial %d not obstructed",fiducial+1);
            else
                MsgOut(status,"Fiducial %d OBSTRUCTED by button/fibre %d",
                       fiducial+1,fidFlags[fiducial]);
        }
    }


}

TDFDELTA_INTERNAL void  tdFdeltaFieldCheck (
        StatusType  *status)
{
    tdFdeltaType  *data = DitsGetActData();/* Function parameters            */
    unsigned      numErrors = 0;           /* Total number of error detected */
    unsigned      numPivots;               /* Number of pivots               */
    FpilType      inst;                    /* Instrument description         */

    if (*status != STATUS__OK) return;


    /*
     *  Get instrument description.  We use this a lot
     */
    inst = tdFdeltaFpilInst();
    /*
     *  Get the number of pivots in this instrument
     */
    numPivots = FpilGetNumPivots(inst);

    /*
     *  Bypass checking if requested.
     */
    if (data->check & NO_FIELD_CHECK) {
        MsgOut(status,"WARNING:Target field validity checking NOT performed - %s",
               "NO_FIELD_CHECK specified");
        if (data->check & SPECIAL) 
            DitsPutHandler(tdFdeltaSequencerSpecial,status);
        else
            DitsPutHandler(tdFdeltaSequencer,status);
            
        DitsPutRequest(DITS_REQ_STAGE,status);
        return;
    }
    else
        MsgOut(status,"Checking target field validity...");

    /*
     *  Check for button/button collisions.
     */
    CheckForButButCollisions(inst,
                             data->check,
                             numPivots,
                             data->butClearO,
                             data->butClearG,
                             data->constants.type,
                             &data->target,
                             &data->constants,
                             &numErrors,
                             status);

    /*
     *  Check for button/fibre collisions.
     */
    CheckForButFibCollisions(inst,
                             data->check,
                             numPivots,
                             data->fibClearO,
                             data->fibClearG,
                             &data->constants,
                             &data->target,
                             &numErrors,
                             status);


    /*
     *  Check fibre extension is within limits.
     */
    CheckFibreExtension(&data->constants,
                        data->check,
                        numPivots,
                        &data->target,
                        &numErrors,
                        status);



    /*
     *  Check that fibre/button angle and pivot/fibre angle are within limits.
     */


    CheckBendAngles(inst,
                    data->check,
                    numPivots,
                    data->maxButAngO,
                    data->maxButAngG,
                    data->maxPivAngO,
                    data->maxPivAngG,
                    &data->constants,
                    &data->target,
                    &numErrors,
                    status);


    /*
     *  Check that button is placed in valid field position (ie, not outside 
     I  field limits or on the 2 screws).
     */
    CheckValidFieldPosition(inst,
                            data->check,
                            numPivots,
                            &data->target,
                            &numErrors,
                            status);

    /*
     *  Check that at least three fiducials will not be obstructed when the
     *  target field is configured (otherwise we can not perform the 
     *  SURVEY action to determine the position of the field plate relative 
     *  to the gantry after tumbling).
     */
    CheckFiducials(inst,
                   data->check,
                   numPivots,
                   &data->constants,
                   &data->target,
                   &data->fids,
                   &numErrors,
                   status);



    /*
     *  Summarise field check findings.
     */
    if (numErrors == 0) {
        if (data->check & SHOW)
            MsgOut(status,"Target field configuration is VALID");
    } else {
        *status = TDFDELTA__INVFIELD;
        ErsRep(0,status,
               "Target field configuration is INVALID - %d %s DETECTED - see scrolling message area for details.",
               numErrors,
               (numErrors == 1?  "ERROR": "ERRORS"));
        ErsRep(0,status,
               "Common causes are - wrong time compared to configuration time (HA) and robot status file (tdFconstantsDF.sds) has changed since fibre allocation");
        ErsRep(0, status,
               "Try tweaking for \"proposal date\" (and aborting once it starts).  If that works, then the original configuration time is likely to be wrong compared to the observing (tweak) time.");
    }

    /*
     *  Reschedule delta process if OK, otherwise complete action.
     */
    if (*status == STATUS__OK) {
        if (data->check & SPECIAL) 
            DitsPutHandler(tdFdeltaSequencerSpecial,status);
        else
            DitsPutHandler(tdFdeltaSequencer,status);
        DitsPutRequest(DITS_REQ_STAGE,status);
    }
    else
    {
        if (data->above)
        {
            StatusType ignore = STATUS__OK;
            SdsDelete(data->above,&ignore);
            SdsFreeId(data->above,&ignore);
            data->above = 0;
        }
        free((void *)data);
    }
}
