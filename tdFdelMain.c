/*+               T D F D E L T A

 *  Module name: 
      tdFdelta

 *  Function:
      2dF Delta Configuration Task main module.

 *  Description:
      This module is a main routine which uses the 2df Delta Configuration
      task module (tdFdelta) and the Generic Instrument Task module (GIT)
      to create the 2dF Delta Configuration Task.

 *  Language:
      C

 *  Support: James Wilcox, AAO

 *-

 *  History:
      01-Jul-1994  JW   Original version
      31-Jan-2000  TJF  Call FpilFree after DitsMainLoop() has exited.
      {@change entry@}

 *  @(#) $Id: ACMM:2dFdelta/tdFdelMain.c,v 3.5 18-Feb-2005 17:31:35+11 tjf $ (mm/dd/yy)
 */



static char *rcsId="@(#) $Id: ACMM:2dFdelta/tdFdelMain.c,v 3.5 18-Feb-2005 17:31:35+11 tjf $";
static void *use_rcsId = ((void)&use_rcsId,(void *) &rcsId);


#include "DitsTypes.h"          /* Basic dits types               */
#include "Dits_Err.h"           /* Dits error codes               */
#include "DitsSys.h"            /* Dits System function           */
#include "DitsParam.h"          /* Dits Parameter system stuff    */  
#include "DitsFix.h"            /* For PutUserData                */
#include "Sdp.h"                /* Simple dits parameter system   */
#include "Git.h"                /* Generic Dits instrument task   */
#include "mess.h"               /* For MessGetMsg                 */
#include "status.h"             /* STATUS__OK definition          */

#include "tdFdelta.h"           /* TDFDLETA def'ns and structs    */
#include "tdFdelta_Err.h"       /* TDFDLETA Errors                */

#include <stdlib.h>             /* Vms definition of malloc       */




/*
 *  tdFdeltaMain
 */
TDFDELTA_PUBLIC int  tdFdeltaMain (
        char  *name)
{
    StatusType  status  = STATUS__OK;        /* Status variable         */
    SdsIdType   parsysid  = 0;               /* For parameter system id */
    char        *taskName = TWODF_TASKNAME;  /* Task name               */

    /*
     *  If a task name is supplied, use it.
     */
    if (name != 0)
        taskName = name;

    /*
     *  Initialise the parameter system and dits.
     */
    DitsInit(taskName,TDFDELTA_MSG_BUFFER,0,&status);
    SdpInit(&parsysid,&status);
    GitTpiInit(&status);

    /*
     *  Activate the two modules we use. This enables action handlers and creates
     *  parameters
     */
    GitActivate(parsysid,&status);
    tdFdeltaActivate(parsysid,&status);

    /*
     *  Loop receiving messages.
     */
    DitsMainLoop(&status);


    /*
     *  Since we don't have a tdFdeltaDeActivate routine, we must call
     *  this at this point.
     */ 
    FpilFree(tdFdeltaFpilInst());
    /*
     *  Exit - shutdown dits and exit.
     */
    return(DitsStop(taskName,&status));
}

#ifdef DITS_MAIN_NEEDED
    TDFDELTA_PUBLIC int  main(
            int   argc,
            char  *argv[])
    {
        if (argc >= 2)
            return(tdFdeltaMain(argv[1]));
        else 
            return(tdFdeltaMain(0));
    }
#endif
