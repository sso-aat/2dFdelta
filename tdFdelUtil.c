/*+                T D F D E L T A

 *  Module name:
      tdFdeltaUtil

 *  Function:
      Utility functions for the tdFdelta task.

 *  Description:

 *  Language:
      C

 *  Support: James Wilcox, AAO

 *-

 *  History:
      30-Jun-1994  JW   Original version
      {@change entry@}

 *  @(#) $Id: ACMM:2dFdelta/tdFdelUtil.c,v 3.5 18-Feb-2005 17:31:36+11 tjf $ (mm/dd/yy)
 */

/*
 *  Include files
 */


static char *rcsId="@(#) $Id: ACMM:2dFdelta/tdFdelUtil.c,v 3.5 18-Feb-2005 17:31:36+11 tjf $";
static void *use_rcsId = ((void)&use_rcsId,(void *) &rcsId);


#include "DitsTypes.h"       /* Basic dits types            */
#include "Dits_Err.h"        /* Dits error codes            */
#include "DitsSys.h"         /* For PutActionHandlers       */
#include "DitsFix.h"         /* For various Dits routines   */
#include "DitsMsgOut.h"      /* For MsgOut                  */
#include "DitsParam.h"       /* For DitsGetParId            */
#include "DitsUtil.h"        /* For DitsErrorText           */
#include "Dmon.h"            /* To DmonCommand              */
#include "arg.h"             /* For ARG_ macros             */
#include "sds.h"             /* For SDS_ macros             */
#include "Sdp.h"             /* Sdp routines                */
#include "mess.h"            /* For MessPutFacility         */
#include "Git.h"             /* Git routines                */
#include "Git_Err.h"         /* GIT__ codes                 */
#include "Ers.h"
#include "status.h"          /* STATUS__OK definition       */

#include "tdFdelta.h"
#include "tdFdelta_Err.h"

#include <stdlib.h>


/*+        T D F D E L T A U T I L

 *  Function name:
      tdFdeltaGetFlag

 *  Function:
      Returns the value of optional flags that may accompany an action.

 *  Description:
        This function searches the SDS action argument structure for the presence
        of the specified character string.  If the exact string is found YES is
        returned, otherwise NO is returned.

 *  Language:
      C

 *  Call:
      (void) = tdFdeltaGetFlag (actArgId,stringName,found,status)

 *  Parameters:  (">" input, "!" modified, "W" workspace, "<" output)
      (>) actArgId    (SdsIdType)       Action argument structure id.
      (>) stringName  (char *)          Character string to search for.
      (<) found       (short *)         Flag indicating if string was found.
      (!) status      (StatusType *)    Modified status.

 *  Proir Requirements:

 *  Support: James Wilcox, AAO

 *-

 *  History:
      30-Jun-1994  JW   Original version
      {@change entry@}

 */
TDFDELTA_INTERNAL void  tdFdeltaGetFlag (
        SdsIdType   actArgId,
        char        *stringName,
        short       *found,
        StatusType  *status)
{
    long int       i = 1;
    SdsIdType      tmpId;
    SdsCodeType    sdsCode;
    unsigned long  dims;
    long int       ndims;
    char           name[20],
                   thisString[30];

    if (*status != STATUS__OK) return;

    *found = NO;

    while (1) {
        SdsIndex(actArgId,i++,&tmpId,status);
        if (*status == SDS__NOMEM)
            return;
        else if (*status != STATUS__OK) {
            *status = STATUS__OK;
            return;
        }
        else {
            SdsInfo(tmpId,name,&sdsCode,&ndims,&dims,status);
            if (sdsCode == SDS_CHAR) {
                ArgGetString(actArgId,name,sizeof(thisString),thisString,status);
                if (*status != STATUS__OK)
                    *status = STATUS__OK;
                else if (strcmp(thisString,stringName) == 0) {
                    *found = YES;
                    return;
                }
            }
        }
    }
}


/*+        T D F D E L T A U T I L

 *  Function name:
      tdFdeltaFlagCheck

 *  Function:
      Searchs the argument structure for the specified flags.

 *  Description:

 *  Language:
      C

 *  Call:
      (void) = tdFdeltaFlagCheck(paramId,checkFor,argFlags,status)

 *  Parameters:  (">" input, "!" modified, "W" workspace, "<" output)
      (>) paramId     (SdsIdType)       The parameter structure id.
      (>) checkFor    (short)           The flags we are interested in.
      (<) argFlags    (short *)         The resulting check bit word.
      (!) status      (StatusType *)    Modified status.

 *  Proir Requirements:

 *  Support: James Wilcox, AAO

 *-

 *  History:
      30-Jun-1994  JW   Original version
      01-Nov-2000  TJF  Support SPECIAL flag.
      {@change entry@}
 */
TDFDELTA_INTERNAL void  tdFdeltaFlagCheck (
        SdsIdType   paramId,
        short       checkFor,
        short       *argFlags,
        StatusType  *status)
{
    short  flag;

    if (*status != STATUS__OK) return;

    /*
     *  Reset word.
     */
    *argFlags = CHECK_ALL;

    /*
     *  If there is no parameter structure return.
     */
    if (!paramId)
        return;

    /*
     *  Check for DEBUG if requested.
     */
    if (checkFor & _DEBUG) {
        tdFdeltaGetFlag(paramId,"DEBUG",&flag,status);
        if (flag == YES) {
            *argFlags += _DEBUG;
            MsgOut(status,"DEBUG flag set");
        }
    }

    /*
     *  Check for DISPLAY if requested.
     */
    if (checkFor & DISPLAY) {
        tdFdeltaGetFlag(paramId,"DISPLAY",&flag,status);
        if (flag == YES) {
            *argFlags += DISPLAY;
            if (*argFlags & _DEBUG)
                MsgOut(status,"DISPLAY flag set");
        }
    }

    /*
     *  Check for NO_FIELD_CHECK if requested.
     */
    if (checkFor & NO_FIELD_CHECK) {
        tdFdeltaGetFlag(paramId,"NO_FIELD_CHECK",&flag,status);
        if (flag == YES) {
            *argFlags += NO_FIELD_CHECK;
            if (*argFlags & _DEBUG)
                MsgOut(status,"NO_FIELD_CHECK flag set");
        }
    }

    /*
     *  Check for NO_ORDER_CHECK if requested.
     */
    if (checkFor & NO_ORDER_CHECK) {
        tdFdeltaGetFlag(paramId,"NO_ORDER_CHECK",&flag,status);
        if (flag == YES) {
            *argFlags += NO_ORDER_CHECK;
            if (*argFlags & _DEBUG)
                MsgOut(status,"NO_ORDER_CHECK flag set");
        }
    }

    /*
     *  Check for NO_DELTA if requested.
     */
    if (checkFor & NO_DELTA) {
        tdFdeltaGetFlag(paramId,"NO_DELTA",&flag,status);
        if (flag == YES) {
            *argFlags += NO_DELTA;
            if (*argFlags & _DEBUG)
                MsgOut(status,"NO_DELTA flag set");
        }
    }

    /*
     *  Check for CHECK_FULL_FIELD if requested.
     */
    if (checkFor & CHECK_FULL_FIELD) {
        tdFdeltaGetFlag(paramId,"CHECK_FULL_FIELD",&flag,status);
        if (flag == YES) {
            *argFlags += CHECK_FULL_FIELD;
            if (*argFlags & _DEBUG)
                MsgOut(status,"CHECK_FULL_FIELD flag set");
        }
    }
   /*
     *  Check for SPECIAL if requested.
     */
    if (checkFor & SPECIAL) {
        tdFdeltaGetFlag(paramId,"SPECIAL",&flag,status);
        if (flag == YES) {
            *argFlags += SPECIAL;
            if (*argFlags & _DEBUG)
                MsgOut(status,"SPECIAL flag set");
        }
    }

}


/*+        T D F D E L T A U T I L

 *  Function name:
      tdFdeltaActionName

 *  Function:
      Display the action name.

 *  Description:

 *  Language:
      C

 *  Call:
      (char *) = tdFdeltaActionName ()

 *  Parameters:  (">" input, "!" modified, "W" workspace, "<" output)

 *  Proir Requirements:

 *  Support: James Wilcox, AAO

 *-

 *  History:
      30-Jun-1994  JW   Original version
      {@change entry@}
 */
TDFDELTA_INTERNAL char  *tdFdeltaActionName ()
{
    StatusType   ignore = STATUS__OK;
    static char  actionName[DITS_C_NAMELEN];

    DitsGetName(sizeof(actionName),actionName,&ignore);
    return(actionName);
}


/*+        T D F D E L T A U T I L

 *  Function name:
      tdFdeltaKick

 *  Function:
      General purpose kick handler for tdFdelta task.

 *  Description:
      All kick actions will result in the aborting of the action.

 *  Language:
      C

 *  Call:
      (void) = tdFdeltaKick (status)

 *  Parameters:  (">" input, "!" modified, "W" workspace, "<" output)
      (!) status      (StatusType *)    Modified status.

 *  Proir Requirements:

 *  Support: James Wilcox, AAO

 *-

 *  History:
      30-Jun-1994  JW   Original version
      {@change entry@}
 */
TDFDELTA_INTERNAL void  tdFdeltaKick (
        StatusType  *status)
{
    free((void *)DitsGetActData());
    MsgOut(status,"%s action terminated",tdFdeltaActionName());
    DitsPutRequest(DITS_REQ_END,status);
}
