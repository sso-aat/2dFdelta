/*+                T D F D E L T A

 *  Module name:
      tdFdeltaConvert

 *  Function:
      Converts data structures from Sds format to C format.

 *  Description:


 *  Language:
      C

 *  Support: James Wilcox, AAO

 *-

 *  History:
      30-Jun-1994  JW   Original version
      {@change entry@}

 *      @(#) $Id: ACMM:2dFdelta/tdFdelConvert.c,v 3.3 09-Apr-2003 21:11:07+10 tjf $ */

/*
 *  Include files
 */


static char *rcsId="@(#) $Id: ACMM:2dFdelta/tdFdelConvert.c,v 3.3 09-Apr-2003 21:11:07+10 tjf $";
static void *use_rcsId = ((void)&use_rcsId,(void *) &rcsId);


#include "DitsTypes.h"       /* Basic dits types            */
#include "DitsMsgOut.h"      /* For MsgOut                  */
#include "DitsUtil.h"        /* For DitsErrorText           */
#include "sds.h"             /* For SDS_ macros             */
#include "arg.h"             /* For ARG_ macros             */
#include "Ers.h"
#include "mess.h"            /* For MessPutFacility         */
#include "status.h"          /* STATUS__OK definition       */
#include "Git.h"

#include "tdFdelta.h"
#include "tdFdelta_Err.h"

#ifdef unix
#   include <malloc.h>
#else
#   include <stdlib.h>
#endif


/*
 *  Internal Function, name:
      tdFdelta___SdsToCrosses

 *  Description:
      Convert the `above' array from the Sds `current' structure to the
      dynamically linked above and below crossover lists.

 *  History:
      30-Jun-1994  JW   Original version
      01-Feb-2000  TJF  Replace NUM_PIVOTS by FPIL_MAXPIVOTS.
      {@change entry@}
 */
TDFDELTA_PRIVATE void  tdFdelta___SdsToCrosses (
        int          size,    /* (>) number of enteries in the `above' array */
        short        above[], /* (>) `above' array containing crossover details */
        tdFcrosses   *cCrosses,/* (<) dynamically linked crossover lists     */
        short        nAbove[],/* (<) number of fibres crossing above each fibre */
        short        nBelow[],/* (<) number of fibres crossing below each fibre */
        StatusType   *status)
{
    int  i, piv;

    if (*status != STATUS__OK) return;

    /*
     *  Initialise crossover lists (Note, it does not matter if we initialise
     *  this list to more elements (FPIL_MAXPIVOTS) then we actuall will
     *  use (FpilGetNumPivots()).
     */
    for (i=0; i< FPIL_MAXPIVOTS; i++) {
        cCrosses->above[i] = cCrosses->below[i] = NULL;
        nAbove[i] = nBelow[i] = 0;
    }

    /*
     *  If there are no crossovers just return.
     */
    if (size == 1)
        return;

    /*
     *  Convert `above' array into dynamically linked crossover list.
     */
    i = 0;
    while (i < size) {

        /*
         *  This array entry is the pivot that has crossovers above it.
         */
        piv = above[i++];

        /*
         *  The enteries following this entry (upto the zero) are the numbers of
         *  fibres crossing above the fibre.  Add them to the appropiate crossover
         *  lists.
         */
        while (above[i] != 0) {
            tdFdeltaAddCross(above[i],&cCrosses->above[piv-1],status);
            tdFdeltaAddCross(piv,&cCrosses->below[above[i]-1],status);
            nBelow[above[i]-1]++;
            nAbove[piv-1]++;
            i++;
        }

        /*
         *  Skip over the zero.
         */
        i++;
    }
}


/*+        T D F D E L T A C O N V E R T

 *  Function name:
      tdFdeltaConvertCurToC

 *  Function:
      Converts the Sds 'current' structure to a C format for the specified field.

 *  Description:

 *  Language:
      C

 *  Call:
      (void) = tdFdeltaConvertCurToC (curId,cField,crosses,status)

 *  Parameters:  (">" input, "!" modified, "W" workspace, "<" output)
      (>) curId       (SdsIdType)       Sds current field details structure.
      (<) cField      (tdFinterim *)    Current field details.
      (<) crosses     (tdFcrosses *)    Crossover list for current field
      (<) aboveID     (SdsIdType *)     The above item from the
                                        SDS structure is copied here..
      (>) check       (short)           Check flags.
      (!) status      (StatusType *)    Modified status.

 *  Proir Requirements:

 *  Support: James Wilcox, AAO

 *-

 *  History:
      30-Jun-1994  JW   Original version
      01-Feb-2000  TJF  Instead of using NUM_PIVOTS macro, use the new
                        variable numPivots, which is initialised to the
                        value returned by FpilGetNumPivots().
      20-Jul-2000  TJF  Copy the above SDS item to the new aboveID item.
      {@change entry@}
 */
TDFDELTA_INTERNAL void  tdFdeltaConvertCurToC (
        SdsIdType    curId,
        tdFinterim   *cField,
        tdFcrosses   *crosses,
        SdsIdType    *aboveID,
        short        check,
        StatusType   *status)
{
    SdsCodeType    code;
    SdsIdType      tmpId;
    unsigned long  actlen,
                   dims[7];
    unsigned       numPivots;
    long int       ndims;
    short          *above;
    char           name[16];

    if (*status != STATUS__OK) return;
    /*
     *  Get the number of pivots in this instrument
     */
    numPivots = FpilGetNumPivots(tdFdeltaFpilInst());
    /*
     *  Display message if requested.
     */
    if (check & SHOW)
        MsgOut(status,"Converting CURRENT field details from SDS to C format...");

    /*
     *  Read field details into C structure.
     */
    SdsFind(curId,"theta",&tmpId,status);
    SdsGet(tmpId,sizeof(double)*numPivots,0,cField->theta,&actlen,status);
    SdsFreeId(tmpId,status);
    SdsFind(curId,"fibreLength",&tmpId,status);
    SdsGet(tmpId,sizeof(double)*numPivots,0,cField->fibreLength,&actlen,status);
    SdsFreeId(tmpId,status);
    SdsFind(curId,"fvpX",&tmpId,status);
    SdsGet(tmpId,sizeof(long int)*numPivots,0,cField->fvpX,&actlen,status);
    SdsFreeId(tmpId,status);
    SdsFind(curId,"fvpY",&tmpId,status);
    SdsGet(tmpId,sizeof(long int)*numPivots,0,cField->fvpY,&actlen,status);
    SdsFreeId(tmpId,status);
    SdsFind(curId,"xf",&tmpId,status);
    SdsGet(tmpId,sizeof(long int)*numPivots,0,cField->xf,&actlen,status);
    SdsFreeId(tmpId,status);
    SdsFind(curId,"yf",&tmpId,status);
    SdsGet(tmpId,sizeof(long int)*numPivots,0,cField->yf,&actlen,status);
    SdsFreeId(tmpId,status);
    SdsFind(curId,"xb",&tmpId,status);
    SdsGet(tmpId,sizeof(long int)*numPivots,0,cField->xb,&actlen,status);
    SdsFreeId(tmpId,status);
    SdsFind(curId,"yb",&tmpId,status);
    SdsGet(tmpId,sizeof(long int)*numPivots,0,cField->yb,&actlen,status);
    SdsFreeId(tmpId,status);
    SdsFind(curId,"park",&tmpId,status);
    SdsGet(tmpId,sizeof(short)*numPivots,0,cField->park,&actlen,status);
    SdsFreeId(tmpId,status);
    SdsFind(curId,"above",&tmpId,status);
    SdsInfo(tmpId,name,&code,&ndims,dims,status);
    if ((above = (short *)malloc(sizeof(short)*dims[0])) == NULL) {
        *status = TDFDELTA__MALLOCERR;
        return;
    }
    SdsCopy(tmpId, aboveID, status);
    SdsGet(tmpId,sizeof(short)*dims[0],0,above,&actlen,status);
    SdsFreeId(tmpId,status);

    /*
     *  Construct crossover lists.
     */
    tdFdelta___SdsToCrosses((int)dims[0],
                            above,
                            crosses,
                            cField->nAbove,
                            cField->nBelow,
                            status);

    /*
     *  Clean up.
     */
    free((void *)above);
    SdsFreeId(curId,status);
    if (*status != STATUS__OK)
        ErsRep(0,status,"Error reading or converting current field details - %s",
               DitsErrorText(*status));
}


/*+        T D F D E L T A C O N V E R T

 *  Function name:
      tdFdeltaConvertConToC

 *  Function:
      Converts the Sds 'constant' structure to a C format for the specified field.

 *  Description:

 *  Language:
      C

 *  Call:
      (void) = tdFdeltaConvertConToC (conId,con,check,status)

 *  Parameters:  (">" input, "!" modified, "W" workspace, "<" output)
      (>) conId       (SdsIdType)       Sds constant field details structure.
      (>) maxExt      (unsigned long)   If non-zero, then this is the 
                                        maximum fibre extent for all
                                        fibres.  IF zero, then read the
                                        maximum fibre extent from
                                        the constants item maxExt
      (<) con         (tdFconstants *)  The constant field C structure.
      (>) check       (short)           Check flags.
      (!) status      (StatusType *)    Modified status.

 *  Proir Requirements:

 *  Support: James Wilcox, AAO

 *-

 *  History:
      30-Jun-1994  JW   Original version
      10-Aug-1998  TJF  Add fetching graspX/graspY
      01-Feb-2000  TJF  Instead of using NUM_PIVOTS macro, use the new
                        variable numPivots, which is initialised to the
                        value returned by FpilGetNumPivots().
      13-Feb-2001  TJF  Support varaible fibre extent, if the new argument
                        maxExt is zero.  This allows 6dF to have different
                        fibre extents whilst being compatible with 2dF.
      {@change entry@}
 */
TDFDELTA_INTERNAL void  tdFdeltaConvertConToC (
        SdsIdType     conId,
        unsigned long maxExt,
        tdFconstants  *con,
        short         check,
        StatusType    *status)
{
    SdsIdType      tmpId;
    unsigned long  actlen;
    unsigned       numPivots;

    if (*status != STATUS__OK) return;

    /*
     *  Get the number of pivots in this instrument
     */
    numPivots = FpilGetNumPivots(tdFdeltaFpilInst());
    /*
     *  Display message if requested.
     */
    if (check & SHOW)
        MsgOut(status,"Converting CONSTANT field details from SDS to C format...");

    /*
     *  Read field details into C structure.
     */
    SdsFind(conId,"tPark",&tmpId,status);
    SdsGet(tmpId,sizeof(double)*numPivots,0,con->tPark,&actlen,status);
    SdsFreeId(tmpId,status);
    SdsFind(conId,"xPark",&tmpId,status);
    SdsGet(tmpId,sizeof(long int)*numPivots,0,con->xPark,&actlen,status);
    SdsFreeId(tmpId,status);
    SdsFind(conId,"yPark",&tmpId,status);
    SdsGet(tmpId,sizeof(long int)*numPivots,0,con->yPark,&actlen,status);
    SdsFreeId(tmpId,status);
    SdsFind(conId,"xPiv",&tmpId,status);
    SdsGet(tmpId,sizeof(long int)*numPivots,0,con->xPiv,&actlen,status);
    SdsFreeId(tmpId,status);
    SdsFind(conId,"yPiv",&tmpId,status);
    SdsGet(tmpId,sizeof(long int)*numPivots,0,con->yPiv,&actlen,status);
    SdsFreeId(tmpId,status);
    SdsFind(conId,"type",&tmpId,status);
    SdsGet(tmpId,sizeof(short)*numPivots,0,con->type,&actlen,status);
    SdsFreeId(tmpId,status);
    SdsFind(conId,"inUse",&tmpId,status);
    SdsGet(tmpId,sizeof(short)*numPivots,0,con->inUse,&actlen,status);
    SdsFreeId(tmpId,status);

    SdsFind(conId,"graspX",&tmpId,status);
    SdsGet(tmpId,sizeof(long int)*numPivots,0,con->graspX,&actlen,status);
    SdsFreeId(tmpId,status);
    SdsFind(conId,"graspY",&tmpId,status);
    SdsGet(tmpId,sizeof(long int)*numPivots,0,con->graspY,&actlen,status);
    SdsFreeId(tmpId,status);

    /*
     * If maxExt argument is non-zero, supply this value for all pivots.  
     * Otherwise, maxExt is fibre specific and can be picked up from
     * the constants structure.  (6dF requires variable length maximum
     * fibre extents - but we must remain compatible with existing
     * 2dF.
     */
    if (maxExt) {
        register unsigned i;
        MsgOut(status, "Using a fixed max extension of %ld", maxExt);
        for (i = 0; i < numPivots ;  ++i) {
            con->maxExt[i] = maxExt;
        }
    } else {
        MsgOut(status, "Using fibre specific maximum fibre extension");
        SdsFind(conId,"maxExt",&tmpId,status);
        SdsGet(tmpId,sizeof(unsigned long)*numPivots,0,
               con->maxExt,&actlen,status);
        SdsFreeId(tmpId,status);
    }

    /*
     *  Clean up.
     */
    SdsFreeId(conId,status);
    if (*status != STATUS__OK)
        ErsRep(0,status,"Error reading or converting constant field details - %s",
               DitsErrorText(*status));
}


/*+        T D F D E L T A C O N V E R T

 *  Function name:
      tdFdeltaConvertOffToC

 *  Function:
      Converts the Sds 'offsets' structure to a C format for the specified field.

 *  Description:

 *  Language:
      C

 *  Call:
      (void) = tdFdeltaConvertOffToC (offId,off,check,status)

 *  Parameters:  (">" input, "!" modified, "W" workspace, "<" output)
      (>) offId       (SdsIdType)       Sds offset field details structure.
      (<) off         (tdFoffsets *)    Button/fibre offset details.
      (>) check       (short)           Check flags.
      (!) status      (StatusType *)    Modified status.

 *  Proir Requirements:

 *  Support: James Wilcox, AAO

 *-

 *  History:
      30-Jun-1994  JW   Original version
      09-Nov-1998  TJF  Support to park offsets.
      01-Feb-2000  TJF  Instead of using NUM_PIVOTS macro, use the new
                        variable numPivots, which is initialised to the
                        value returned by FpilGetNumPivots().
      {@change entry@}
 */
TDFDELTA_INTERNAL void  tdFdeltaConvertOffToC (
        SdsIdType    offId,
        tdFoffsets   *off,
        short        check,
        StatusType   *status)
{
    SdsIdType      tmpId;
    unsigned long  actlen;
    unsigned       numPivots;


    if (*status != STATUS__OK) return;
    /*
     *  Get the number of pivots in this instrument
     */
    numPivots = FpilGetNumPivots(tdFdeltaFpilInst());

    /*
     *  Display message if requested.
     */
    if (check & DISPLAY)
        MsgOut(status,"Converting OFFSET field details from SDS to C format...");

    /*
     *  Read offset details into C structure.
     */
    SdsFind(offId,"xOffPL",&tmpId,status);
    SdsGet(tmpId,sizeof(short)*numPivots,0,off->xOffPL,&actlen,status);
    SdsFreeId(tmpId,status);
    SdsFind(offId,"yOffPL",&tmpId,status);
    SdsGet(tmpId,sizeof(short)*numPivots,0,off->yOffPL,&actlen,status);
    SdsFreeId(tmpId,status);

    SdsFind(offId,"xOffFrPK",&tmpId,status);
    SdsGet(tmpId,sizeof(short)*numPivots,0,off->xOffFrPK,&actlen,status);
    SdsFreeId(tmpId,status);
    SdsFind(offId,"yOffFrPK",&tmpId,status);
    SdsGet(tmpId,sizeof(short)*numPivots,0,off->yOffFrPK,&actlen,status);
    SdsFreeId(tmpId,status);

    SdsFind(offId,"xOffToPK",&tmpId,status);
    SdsGet(tmpId,sizeof(short)*numPivots,0,off->xOffToPK,&actlen,status);
    SdsFreeId(tmpId,status);
    SdsFind(offId,"yOffToPK",&tmpId,status);
    SdsGet(tmpId,sizeof(short)*numPivots,0,off->yOffToPK,&actlen,status);
    SdsFreeId(tmpId,status);

    /*
     *  Clean up.
     */
    SdsFreeId(offId,status);
    if (*status != STATUS__OK)
        ErsRep(0,status,"Error reading or converting offset field details - %s",
               DitsErrorText(*status));
}


/*+        T D F D E L T A C O N V E R T

 *  Function name:
      tdFdeltaConvertFidToC

 *  Function:
      Converts the Sds 'fiducials' structure to a C format for the specified field.

 *  Description:

 *  Language:
      C

 *  Call:
      (void) = tdFdeltaConvertFidToC (fidId,fid,check,status)

 *  Parameters:  (">" input, "!" modified, "W" workspace, "<" output)
      (>) fidId       (SdsIdType)       Sds fiducials field details structure.
      (<) fid         (tdFfiducials *)  Fiducial details.
      (>) check       (short)           Check flags.
      (!) status      (StatusType *)    Modified status.

 *  Proir Requirements:

 *  Support: James Wilcox, AAO

 *-

 *  History:
      15-Mar-1996  JW   Original version
      17-Mar-2000  TJF  Use FPIL to get the number of fiducials rather then
                        the macro NUM_FIDUCIALS.
      {@change entry@}
 */
TDFDELTA_INTERNAL void  tdFdeltaConvertFidToC (
        SdsIdType     fidId,
        tdFfiducials  *fid,
        short         check,
        StatusType    *status)
{
    SdsIdType      tmpId;
    unsigned long  actlen;
    unsigned long  bytesINT32;
    unsigned long  bytesShort;

    /*
     *  Display message if requested.
     */
    if (check & DISPLAY)
        MsgOut(status,"Converting FIDUCIAL details from SDS to C format...");

    /*
     *  Work out the size in bytes of the xf, yf and inUse arrays, which
     *  depend on the number of fiducials.
     */
    bytesINT32 = FpilGetNumFiducials(tdFdeltaFpilInst()) * sizeof(INT32);
    bytesShort = FpilGetNumFiducials(tdFdeltaFpilInst()) * sizeof(INT32);

    /*
     *  Read offset details into C structure.
     */
    SdsFind(fidId,"xf",&tmpId,status);
    SdsGet(tmpId,bytesINT32,0,fid->fidX,&actlen,status);
    SdsFreeId(tmpId,status);
    SdsFind(fidId,"yf",&tmpId,status);
    SdsGet(tmpId,bytesINT32,0,fid->fidY,&actlen,status);
    SdsFreeId(tmpId,status);
    SdsFind(fidId,"inUse",&tmpId,status);
    SdsGet(tmpId,bytesShort,0,fid->inUse,&actlen,status);
    SdsFreeId(tmpId,status);

    /*
     *  Clean up.
     */
    SdsFreeId(fidId,status);
    if (*status != STATUS__OK)
        ErsRep(0,status,"Error reading or converting fiducial details - %s",
               DitsErrorText(*status));
}


/*+        T D F D E L T A C O N V E R T

 *  Function name:
      tdFdeltaConvertTarToC

 *  Function:
      Converts the Sds 'target' structure to a C format.

 *  Description:

 *  Language:
      C

 *  Call:
      (void) = tdFdeltaConvertConToC (tarId,tar,check,status)

 *  Parameters:  (">" input, "!" modified, "W" workspace, "<" output)
      (>) tarId       (SdsIdType)       Sds target field details structure.
      (<) tar         (tdFtarget *)     The constant field C structure.
      (>) check       (short)           Check flags.
      (!) status      (StatusType *)    Modified status.

 *  Proir Requirements:

 *  Support: James Wilcox, AAO

 *-

 *  History:
      30-Jun-1994  JW   Original version
      01-Feb-2000  TJF  Instead of using NUM_PIVOTS macro, use the new
                        variable numPivots, which is initialised to the
                        value returned by FpilGetNumPivots().
      {@change entry@}
 */
TDFDELTA_INTERNAL void  tdFdeltaConvertTarToC (
        SdsIdType     tarId,
        tdFtarget     *tar,
        short         check,
        StatusType    *status)
{
    SdsIdType      tmpId;
    unsigned long  actlen;
    unsigned       numPivots;

    if (*status != STATUS__OK) return;

    /*
     *  Get the number of pivots in this instrument
     */
    numPivots = FpilGetNumPivots(tdFdeltaFpilInst());

    /*
     *  Display message if requested.
     */
    if (check & DISPLAY)
        MsgOut(status,"Converting TARGET field details from SDS to C format...");

    /*
     *  Read field details into C structure.
     */
    SdsFind(tarId,"theta",&tmpId,status);
    SdsGet(tmpId,sizeof(double)*numPivots,0,tar->theta,&actlen,status);
    SdsFreeId(tmpId,status);
    SdsFind(tarId,"fibreLength",&tmpId,status);
    SdsGet(tmpId,sizeof(double)*numPivots,0,tar->fibreLength,&actlen,status);
    SdsFreeId(tmpId,status);
    SdsFind(tarId,"fvpX",&tmpId,status);
    SdsGet(tmpId,sizeof(long int)*numPivots,0,tar->fvpX,&actlen,status);
    SdsFreeId(tmpId,status);
    SdsFind(tarId,"fvpY",&tmpId,status);
    SdsGet(tmpId,sizeof(long int)*numPivots,0,tar->fvpY,&actlen,status);
    SdsFreeId(tmpId,status);
    SdsFind(tarId,"xf",&tmpId,status);
    SdsGet(tmpId,sizeof(long int)*numPivots,0,tar->xf,&actlen,status);
    SdsFreeId(tmpId,status);
    SdsFind(tarId,"yf",&tmpId,status);
    SdsGet(tmpId,sizeof(long int)*numPivots,0,tar->yf,&actlen,status);
    SdsFreeId(tmpId,status);
    SdsFind(tarId,"mustMove",&tmpId,status);
    SdsGet(tmpId,sizeof(short)*numPivots,0,tar->mustMove,&actlen,status);
    SdsFreeId(tmpId,status);
    SdsFind(tarId,"park",&tmpId,status);
    SdsGet(tmpId,sizeof(short)*numPivots,0,tar->park,&actlen,status);
    SdsFreeId(tmpId,status);

    /*
     *  Clean up.
     */
    SdsFreeId(tarId,status);
    if (*status != STATUS__OK)
        ErsRep(0,status,"Error reading or converting target field details - %s",
               DitsErrorText(*status));
}
