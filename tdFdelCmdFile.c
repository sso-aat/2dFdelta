/*+           T D F D E L T A

 *  Module name:
      tdFdeltaCmdFile

 *  Function:
      Contains functions to enable the creation of 2dF Command Files.

 *  Description:

 *  Language:
      C

 *  Support: James Wilcox, AAO

 *-

 *  History:
      01-Jul-1994  JW    Original version
      {@change entry@}

 *  @(#) $Id: ACMM:2dFdelta/tdFdelCmdFile.c,v 3.8 19-Jun-2007 09:32:34+10 tjf $ (mm/dd/yy)
 */

/*
 *  Include files.
 */


static char *rcsId="@(#) $Id: ACMM:2dFdelta/tdFdelCmdFile.c,v 3.8 19-Jun-2007 09:32:34+10 tjf $";
static void *use_rcsId = (0 ? (void *)(&use_rcsId) : (void *) &rcsId);


#include "DitsTypes.h"       /* Basic dits types            */
#include "Dits_Err.h"        /* Dits error codes            */
#include "DitsSys.h"         /* For PutActionHandlers       */
#include "DitsFix.h"         /* For various Dits routines   */
#include "DitsMsgOut.h"      /* For MsgOut                  */
#include "DitsParam.h"       /* For DitsGetParId            */
#include "DitsUtil.h"        /* For DitsErrorText           */
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

#include <stdio.h>
#include <string.h>

#ifdef DSTDARG_OK
#   include <stdarg.h>       /* For ANSI C                  */
#else
#   include <varargs.h>      /* For old style unix          */
#endif


/*
 *+           T D F D E L T A C M D F I L E

 *  Function name:
      tdFdeltaCFnew

 *  Function:
      Creates a new command file.

 *  Description:
      This function creates a new command file and records the current x, y, and theta
      coordinates for each button in that field. This allow the position of each button
      to be checked at the time of command file execution to confirm command file
      validity.

      The function returns the SdsIdType for the generated command file.

 *  Language:
      C

 *  Call:
      (SdsIdType) = tdFdeltaCFnew (name,currDetails,status)

 *  Parameters:   (">" input, "!" modified, "W" workspace, "<" output)
      (>) name         (char *)        Name of the command file to be generated.
      (>) currDetails  (tdFinterim *)  Pointer to current field details.
      (>) above        (SdsIdType)    The SDS id of the above item from
                                      the original plate current details
                                      structure.  This is just copied
                                      to the output file.
      (!) status       (StatusType *)  Modified status.

 *  Prior requirements:

 *  Support: James Wilcox, AAO

 *-

 *  History:
      01-Jul-1994  JW    Original version
      01-Feb-2000  TJF   Change intial value of dims from NUM_PIVOTS to
                         the value returned by FpilGetNumPivots().
      20-Oct-2000  TJF   Add the new "above" item to the output so that
                         we can reconstruct the conditions before the
                         delta.
      {@change entry@}
 */
TDFDELTA_INTERNAL SdsIdType  tdFdeltaCFnew (
        char        *name,
        tdFinterim  *currDetails,
        SdsIdType   *above,
        StatusType  *status)
{
    SdsIdType          newCmdFile,
                       xId, yId,
                       thetaId;
    unsigned long int  dims = FpilGetNumPivots(tdFdeltaFpilInst());

    if (*status != STATUS__OK) return (0);

    /*
     *  Create new command file (Sds structure).
     */
    SdsNew(0,name,0,NULL,SDS_STRUCT,0,NULL,&newCmdFile,status);
    SdsNew(newCmdFile,"xf",0,NULL,SDS_INT,1,&dims,&xId,status);
    SdsNew(newCmdFile,"yf",0,NULL,SDS_INT,1,&dims,&yId,status);
    SdsNew(newCmdFile,"theta",0,NULL,SDS_DOUBLE,1,&dims,&thetaId,status);
    /*
     * If the SDS above structure exists, insert it into the
     * command file and then free the id and make.
     */
    if (*above) {
        SdsInsert(newCmdFile, *above, status);
        SdsFreeId(*above, status);
        *above = 0;
    }

    /*
     *  Add x,y,theta for current field.
     */
    SdsPut(xId,sizeof(long int)*dims,0,(void *)currDetails->xf,status);
    SdsPut(yId,sizeof(long int)*dims,0,(void *)currDetails->yf,status);
    SdsPut(thetaId,sizeof(double)*dims,0,(void *)currDetails->theta,status);

    /*
     *  Free sds id's.
     */
    SdsFreeId(xId,status);
    SdsFreeId(yId,status);
    SdsFreeId(thetaId,status);

    /*
     *  Return SdsIdType for command file.
     */
    return (newCmdFile);
}


/*
 *+           T D F D E L T A C M D F I L E

 *  Function name:
      tdFdeltaCFaddMoves

 *  Function:
      Append the number of moves to be performed to the command file.

 *  Description:
      The number of moves and the number of parks are added to the command file.

 *  Language:
      C

 *  Call:
      (void) = tdFdeltaCFaddMoves (cmdFileId,numMoves,numParks,status)

 *  Parameters:   (">" input, "!" modified, "W" workspace, "<" output)
      (>) cmdFileId    (SdsIdType)     Name of the command file.
      (>) numMoves     (long int)      Number of moves to be performed.
      (>) numParks     (long int)      Number of parks to be performed.
      (!) status       (StatusType *)  Modified status.

 *  Prior requirements:

 *  Support: James Wilcox, AAO

 *-

 *  History:
      01-Jul-1994  JW   Original version
      {@change entry@}
 */
TDFDELTA_INTERNAL void  tdFdeltaCFaddMoves (
        SdsIdType   cmdFileId,
        long int    numMoves,
        long int    numParks,
        StatusType  *status)
{
    /*
     *  Append the number of moves and parks to the command file.
     */
    ArgPuti(cmdFileId,"numMoves",numMoves,status);
    ArgPuti(cmdFileId,"numParks",numParks,status);
}


/*
 *+           T D F D E L T A C M D F I L E

 *  Function name:
      tdFdeltaCFaddCmd

 *  Function:
      Adds a command to the command file.

 *  Description:
      Adds a structure containing the command and appropiate parameters to the
      command file (the SdsIdType).

 *  Language:
      C

 *  Call:
      (void) = tdFdeltaCFaddCmd (status,cmdFileId,lineNo,cmd,param1,...)

 *  Parameters:   (">" input, "!" modified, "W" workspace, "<" output)
      (!) status        (StatusType *)  Modified status.
      (>) cmdFileId     (SdsIdType)     The Sds Id for the command file.
      (>) lineNo        (int)           The number of the line to be added.
      (>) cmd           (char *)        The command to be added.
      (>) param1,...    (...)           The accompanying parameters (see 
                                        individual command description for 
                                        parameters and types).

 *  Prior requirements:

 *  Support: James Wilcox, AAO

 *-

 *  History:
      01-Jul-1994  JW   Original version
      10-Aug-1998  TJF  Drop offsets from output file
      {@change entry@}
 */
#ifdef DSTDARG_OK
    TDFDELTA_INTERNAL void  tdFdeltaCFaddCmd (
            StatusType   *status,
            SdsIdType    cmdFileId,
            int          lineNo,
            char         *cmd,
            ...)
#else
    TDFDELTA_INTERNAL void tdFdeltaCFaddCmd (va_alist)
            va_dcl
#endif
{
    va_list    args;                     /* Argument pointer list */
    double     tarTheta;
    long int   tarXf, tarYf;             /* Command parameters    */
    int        piv;
    char       lineName[10],             /* Name for new entry    */
               cmdLine[CMDLINE_LENGTH],  /* Command line contents */
               *comment;

#   ifdef DSTDARG_OK
        va_start(args,cmd);
#   else
        StatusType   *status;
        SdsIdType    cmdFileId;
        int          lineNo;
        char         *cmd;

        va_start(args);
        status    = va_arg(args, StatusType *);
        cmdFileId = va_arg(args, SdsIdType);
        lineNo    = va_arg(args, int);
        cmd       = va_arg(args, char *);
#   endif

    if (*status != STATUS__OK) return;

    /*
     *  Name new structure `lineX', where X is the line number.
     */
    sprintf(lineName,"line%d",lineNo);

    /*
     *  Command is: MOVE FIBRE (MF).
     */
    if (strcmp("MF",cmd) == 0) {

        /*
         *  Get MF args (must be in correct order).
         */
        piv      = va_arg(args, int);
        tarXf    = va_arg(args, long int);
        tarYf    = va_arg(args, long int);
        tarTheta = va_arg(args, double);

        /*
         *  Add new line to command file containing move details.
         */
        if (ErsSPrintf(sizeof(cmdLine),cmdLine,"%s %d %ld %ld %f",
                        cmd,piv,tarXf,tarYf,tarTheta) == EOF)
            *status = TDFDELTA__SPRINTF;
        else
            ArgPutString(cmdFileId,lineName,cmdLine,status);
        va_end(args);
        return;
    }

    /*
     *  Command is: PARK FIBRE (PF).
     */
    else if (strcmp("PF",cmd) == 0) {

        /*
         *  Get PF args.
         */
        piv  = va_arg(args, int);

        /*
         *  Add new line to command file containing park details.
         */
        if (ErsSPrintf(sizeof(cmdLine),cmdLine,"%s %d",cmd,piv) == EOF)
            *status = TDFDELTA__SPRINTF;
        else
            ArgPutString(cmdFileId,lineName,cmdLine,status);
        va_end(args);
        return;
    }

    /*
     *  Command is: COMMENT (no echo) (!).
     */
    else if (strcmp("!",cmd) == 0) {

        /*
         *  Get ! arg.
         */
        comment = va_arg(args, char *);

        /*
         *  Add new line to command file containing comment.
         */
        if (ErsSPrintf(sizeof(cmdLine),cmdLine,"%s %s",cmd,comment) == EOF)
            *status = TDFDELTA__SPRINTF;
        else
            ArgPutString(cmdFileId,lineName,cmdLine,status);
        va_end(args);
        return;
    }

    /*
     *  Command is: COMMENT (echo) (*).
     */
    else if (strcmp("*",cmd) == 0) {

        /*
         *  Get * arg.
         */
        comment = va_arg(args, char *);

        /*
         *  Add new line to command file containing comment.
         */
        if (ErsSPrintf(sizeof(cmdLine),cmdLine,"%s %s",cmd,comment) == EOF)
            *status = TDFDELTA__SPRINTF;
        else
            ArgPutString(cmdFileId,lineName,cmdLine,status);
        va_end(args);
        return;
    }

    /*
     *  No such command.
     */
    else {
        *status = TDFDELTA__CF_NOCMD;
        va_end(args);
        return;
    }
}
/*
 *+           T D F D E L T A C M D F I L E

 *  Function name:
      tdFdeltaCFaddNumSpringOutParks

 *  Function:
      Append the number of moves to be performed to the command file.

 *  Description:
      Add the number of spring out parks (6dF special delta) to the
      command file.

 *  Language:
      C

 *  Call:
      (void) = tdFdeltaCFaddSpringOutParks (cmdFileId,numSpringOutParks, 
      status)

 *  Parameters:   (">" input, "!" modified, "W" workspace, "<" output)
      (>) cmdFileId    (SdsIdType)     Name of the command file.
      (>) numSpringOutParks (long int)      Number of spring out parks.
      (!) status       (StatusType *)  Modified status.

 *  Prior requirements:

 *  Support: James Wilcox, AAO

 *-

 *  History:
      22-Sep-2002  TJF  Original version
      {@change entry@}
 */
TDFDELTA_INTERNAL void  tdFdeltaCFaddSpringOutParks (
        SdsIdType   cmdFileId,
        long int    numSpringOutParks,
        StatusType  *status)
{
    /*
     *  Append the number of sprint out parks to the file..
     */
    ArgPuti(cmdFileId,"springOutParks",numSpringOutParks,status);
}
