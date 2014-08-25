/*+                 T D F D E L T A

 *  Module name:
      tdFdelta

 *  Function:
      Action definitions/handlers and parameter structure definition for 
      2dF/6dF Delta Configuration Task (TDFDELTA).

 *  Description:

 *  Language:
      C

 *  Support: James Wilcox, AAO

 *-

 *  History:
      30-Jun-1994  JW   Original version
      10-Jun-1998  TJF  Set the ENQ_VER_NUM and ENQ_VER_DATE parameters
      28-Jan-2000  TJF  Add the tdFdeltaFpilInst() function and
                        the tdFdeltaInstrument variable.
      24-Feb-2000  TJF  Warn about version compilation combinations.
      01-Nov-2000  TJF  Remove ability to check FPIL against old
                        tdFcollision routines.
      {@change entry@}

 *     @(#) $Id: ACMM:2dFdelta/tdFdelta.c,v 3.17 25-Aug-2014 14:38:02+10 tjf $
 */

/*
 *  Include files
 */


static char *rcsId="@(#) $Id: ACMM:2dFdelta/tdFdelta.c,v 3.17 25-Aug-2014 14:38:02+10 tjf $";
static void *use_rcsId = (0 ? (void *)(&use_rcsId) : (void *) &rcsId);


#include "DitsTypes.h"       /* Basic dits types              */
#include "Dits_Err.h"        /* Dits error codes              */
#include "DitsSys.h"         /* For PutActionHandlers         */
#include "DitsFix.h"         /* For various Dits routines     */
#include "DitsMsgOut.h"      /* For MsgOut                    */
#include "DitsParam.h"       /* For DitsGetParId              */
#include "DitsSignal.h"      /* For isr                       */
#include "DitsInteraction.h" /* For NotInterested & Obey      */
#include "DitsUtil.h"        /* For DitsErrorText             */
#include "Dmon.h"            /* To DmonCommand                */
#include "arg.h"             /* For ARG_ macros               */
#include "sds.h"             /* For SDS_ macros               */
#include "Sdp.h"             /* Sdp routines                  */
#include "mess.h"            /* For MessPutFacility           */
#include "Git.h"             /* Git routines                  */
#include "Git_Err.h"         /* GIT__ codes                   */
#include "Ers.h"

#define  TDFDELTA_MODULE
#include "tdFdelta.h"
#include "tdFdelta_Err.h"
#include "tdFdelta_Err_msgt.h"

#include <string.h>
/*
 *  Include headers of routines which allow us to support 6dF and 2dF with
 *  the FPIL library.  The NO_SIX_DF and NO_TWO_DF macros allow us to
 *  disable support for these instruments.
 */
#ifndef NO_SIX_DF
#include "sixdffpil.h"
#warning "Compiling with 6dF support"
#endif
#ifndef NO_TWO_DF
#include "tdffpil.h"
#warning "Compiling with 2dF support"
#endif

/*
 *  tdFdeltaVersion and tdFdeltaDate are defined in the module tdFdelVersion.c
 */
extern const char * const tdFdeltaVersion;
extern const char * const tdFdeltaDate;

/*
 *  Instrumment description variable. 
 */
static FpilType tdFdeltaInstrument;


/*
 *  Function prototypes - ACTIONS
 */
TDFDELTA_PRIVATE void  tdFdeltaInitialise(StatusType *status);
TDFDELTA_PRIVATE void  tdFdeltaReset(StatusType *status);
TDFDELTA_PRIVATE void  tdFdeltaExit(StatusType *status);
TDFDELTA_PRIVATE void  tdFdeltaGenerate(StatusType *status);

/*
 *  Action definitions
 */
static DitsActionMapType tdFdeltaMap[] = {
    {tdFdeltaInitialise, 0,            0, "INITIALISE"},
    {tdFdeltaReset,      0,            0, "RESET"     },
    {tdFdeltaExit,       0,            0, "EXIT"      },
    {tdFdeltaGenerate,   tdFdeltaKick, 0, "GENERATE"  },
    };
int tdFdeltaMapSize = sizeof(tdFdeltaMap)/sizeof(DitsActionMapType);

/*
 *  Parameter Initialisation variables.
 */
float  deltaProg = 0.0;

/*
 *  Parameter array
 */
static SdpParDefType tdFdeltaParams[] = {
    /*
     *  Override the following Git parameters.
     */
    {"ENQ_DEV_DESCR", "2dF Delta Configuration Task", ARG_STRING},
    {"ENQ_VER_NUM",   "r1_0",                         ARG_STRING},
    {"ENQ_VER_DATE",  "May-1996",                     ARG_STRING},
    /*
     *  Progress parameter.
     */
    {"DELTA_PROG",    &deltaProg,                     SDS_FLOAT }
    };
static int tdFdeltaParamCnt = sizeof(tdFdeltaParams)/sizeof(SdpParDefType);


/*
 *+           T D F D E L T A

 *  Function name:
      tdFdeltaActivate

 *  Function:
      Activate the 2df Delta Configuration Task action handlers.

 *  Description:
      Actions handlers are put using DitsPutActionHandlers and the 
      parameters are created using SdpCreate.

      Initialise the instrument model (as returned by tdFdeltaFpilInst())
      for either 2dF or 6dF based on the task name.  If the task name
      starts with SIXDF, then we are the 6dF delta task instead of the
      2dF delta task.

 *  Language:
      C

 *  Call:
      (void) = tdFdeltaActivate (parsysid,status)

 *  Parameters:   (">" input, "!" modified, "W" workspace, "<" output)
      (>) parsysid   (SdsIdType)    The parameter system id.
      (!) status     (StatusType *) Modified status. 

 *  Prior requirements:
      DitsInit and SdpInit should have been called.  Should not be called
      from a Dits action handler routine.

 *  Support: James Wilcox, AAO

 *-

 *  History:
      30-Jun-1994  JW   Original version
      10-Jun-1998  TJF  Set the ENQ_VER_NUM and ENQ_VER_DATE parameters.
      28-Jan-2000  TJF  Convert to using FPIL module to allow support
                        of 6dF as well as 2dF, based on task name.
      {@change entry@}
 */
TDFDELTA_PUBLIC void  tdFdeltaActivate (
        SdsIdType   parsysid,
        StatusType  *status)
{
    char name[DITS_C_NAMELEN];
    MessPutFacility(&MessFac_TDFDELTA);
    DitsPutActionHandlers(tdFdeltaMapSize,tdFdeltaMap,status);
    if (parsysid)
        SdpCreate(parsysid,tdFdeltaParamCnt,tdFdeltaParams,status);

   /*
    * Put the version number and date parameter values
    */
    SdpPutString ("ENQ_VER_NUM",tdFdeltaVersion,status);
    SdpPutString ("ENQ_VER_DATE",tdFdeltaDate,status);

    /*
     * Are we the 2dF or 6dF task.  If our name starts with SIX, then
     * we are the 6dF task
     */
    DitsGetTaskName(sizeof(name), name, status);
    if (strncmp(name,SIXDF,sizeof(SIXDF)-1) == 0)
    {
#       ifndef NO_SIX_DF
        {
            printf("%s:Activating as 6dF delta task\n",name);
            sixdfFpilMinInit(&tdFdeltaInstrument);
        }
#       else
        {
            fprintf(stderr,"Delta task can't run with 6dF support as it was not compiled with 6dF support\n");
            exit(1);
        }
#       endif
    } 
    else
    {
#       ifndef NO_TWO_DF
        {
            printf("%s:Activating as 2dF delta task\n",name);
            TdfFpilMinInit(&tdFdeltaInstrument);
        }
#       else
        {
            fprintf(stderr,"Delta task can't run with 2dF support as it was not compiled with 2dF support\n");
            exit(1);
        }
#       endif
    }       
}


/*
 *  Internal Function, name:
      tdFdeltaInitialise

 *  Description:
      Initialise 2dF Delta Configuration Task.  (Note that this does nothing - task
      does not need initialising, the action is only included so task conforms to GIT
      standards.

 *  History:
      30-Jun-1994  JW   Original version
      28-Jan-2000  TJF  Get instrument and telescope name into message 
		 	using FPIL.
      04-Feb-2000  TJF  Get task name into message using DitsGetTaskName.
      {@change entry@}
 */
TDFDELTA_PRIVATE void  tdFdeltaInitialise (
        StatusType  *status)
{
    char name[DITS_C_NAMELEN];
    DitsGetTaskName(sizeof(name), name, status);
    MsgOut(status,"%s Delta Configuration Task (%s) for %s Initialised",
           FpilGetInstName(tdFdeltaFpilInst()),
	   name,
           FpilGetTelescope(tdFdeltaFpilInst()));
}


/*
 *  Internal Function, name:
      tdFdeltaReset

 *  Description:
      Reset 2dF Delta Configuration Task.  (Note that this does nothing - task
      does not need initialising, the action is only included so task conforms to GIT
      standards.

 *  History:
      30-Jun-1994  JW   Original version
      {@change entry@}
 */
TDFDELTA_PRIVATE void  tdFdeltaReset (
        StatusType  *status)
{
    MsgOut(status,"2dF Delta Configuration Task (TDFDELTA) Reset");
}


/*
 *  Internal Function, name:
      tdFdeltaExit

 *  Description:
      Shut down the 2dF Delta Configuration Task.

 *  History:
      28-Jul-1994  JW   Original version
      {@change entry@}
 */
TDFDELTA_PRIVATE void  tdFdeltaExit (
        StatusType  *status)
{
    MsgOut(status,"2dF Delta Configuration Task (TDFDELTA) shut down");
    DitsPutRequest(DITS_REQ_EXIT,status);
}


/*
 *  Internal Function, name:
      tdFdeltaGenerate

 *  Action:  GENERATE  maxFibExt maxButAngG maxPivAngG maxButAngO maxPivAngO
                       butClearG fibClearG butClearO fibClearO
                       tdFtarget tdFconstants tdFoffsets tdFfiducials
                       [tdFcurrent name] [flag]

 *  Parameters:
      maxFibExt    - SDS_INT    - Maximum allowable fibre extension (microns)
      maxButAngX   - SDS_DOUBLE - Maximum allowable bend angle between fibre and button
                                  (radians, G = guide probes, O = object probes)
      maxPivAngX   - SDS_DOUBLE - Maximum allowable bend angle between fibre and pivot
                                  (radians, G = guide probes, O = object probes)
      butClearX    - SDS_INT    - Clearence around button during collision checking
                                  (microns, G = guide probes, O = object probes)
      fibClearX    - SDS_INT    - Clearence around fibre during collision checking
                                  (microns, G = guide probes, O = object probes)
      tdFtarget    - SDS_STRUCT - Target field details (see tdFdelta doc)
      tdFconstants - SDS_STRUCT - Constant field details (see tdFdelta doc)
      tdFoffsets   - SDS_STRUCT - Button/fibre offset details (see tdFdelta doc)
      tdFfiducials - SDS_STRUCT - Fiducials details (see tdFdelta doc)
      [tdFcurrent] - SDS_STRUCT - Current field details (only required when NO_DELTA flag
                                  is not specified, see tdFdelta doc)
      [name]       - ARG_STRING - Name of command file to be generated (only required
                                  when NO_DELTA flag is not specified, see tdFdelta doc)
      [extSpringOut] - SDS_INT    The extension when the 6dF spring is starting
                                  to rise. Only needed if SPECIAL flag is
                                  supplied.
      [flag]       - ARG_STRING - DISPLAY
                                - DEBUG
                                - NO_DELTA
                                - NO_ORDER_CHECK
                                - NO_FIELD_CHECK
                                - CHECK_FULL_FIELD
                                - SPECIAL (for 6dF)

 *  Description:
      Check the target field validity and generate a command file containing the
      required sequence of moves to change from the current to target configurations.

 *  History:
      30-Jun-1994  JW   Original version
      28-Jul-1998  TJF  data->offsets renamed to data->offsets_
      20-Oct-2000  TJF  Initialise new above item in tdFdeltaType and pass
                        its address to tdFdeltaConvertCurToC.
      01-Nov-2000  TJF  Support SPECIAL flag.
      13-Feb-2001  TJF  Max fibre extend may now be fibre specific.  It is
                        still supplied as an argument, but if 0, then 
                        an extra item is supplied in the constants structure
                        to specify it on a fibre specific basis.
      {@change entry@}
 */
TDFDELTA_PRIVATE void  tdFdeltaGenerate (
        StatusType  *status)
{
    tdFdeltaType  *data;                   /* Will contain all action parameters   */
    SdsIdType     tarId,  curId,           /* SDS structure identifiers            */
                  conId,  offId,
                  fidId;
    char          name[FILENAME_LENGTH];   /* Name of command file to be generated */
    double        maxButAngG, maxPivAngG,
                  maxButAngO, maxPivAngO;
    long int      maxFibExt,
                  butClearG,  fibClearG,
                  butClearO,  fibClearO;
    long int      extSpringOut = 0;
    int           index;
    short         check;

    /*
     *  Get action arguments.
     */
    tdFdeltaFlagCheck(DitsGetArgument(),
                      _DEBUG | DISPLAY | CHECK_FULL_FIELD | NO_FIELD_CHECK |
                      NO_ORDER_CHECK | NO_DELTA | SPECIAL,
                      &check,
                      status);

    GitArgGetI(DitsGetArgument(),"maxFibExt",1,0,0,GIT_M_ARG_KEEPERR,&maxFibExt,status);
    GitArgGetD(DitsGetArgument(),"maxButAngG",2,0,0,GIT_M_ARG_KEEPERR,&maxButAngG,status);
    GitArgGetD(DitsGetArgument(),"maxPivAngG",3,0,0,GIT_M_ARG_KEEPERR,&maxPivAngG,status);
    GitArgGetD(DitsGetArgument(),"maxButAngO",4,0,0,GIT_M_ARG_KEEPERR,&maxButAngO,status);
    GitArgGetD(DitsGetArgument(),"maxPivAngO",5,0,0,GIT_M_ARG_KEEPERR,&maxPivAngO,status);
    GitArgGetI(DitsGetArgument(),"butClearG",6,0,0,GIT_M_ARG_KEEPERR,&butClearG,status);
    GitArgGetI(DitsGetArgument(),"fibClearG",7,0,0,GIT_M_ARG_KEEPERR,&fibClearG,status);
    GitArgGetI(DitsGetArgument(),"butClearO",8,0,0,GIT_M_ARG_KEEPERR,&butClearO,status);
    GitArgGetI(DitsGetArgument(),"fibClearO",9,0,0,GIT_M_ARG_KEEPERR,&fibClearO,status);
    GitArgNamePos(DitsGetArgument(),"tdFtarget",10,&tarId,status);
    GitArgNamePos(DitsGetArgument(),"tdFconstants",11,&conId,status);
    GitArgNamePos(DitsGetArgument(),"tdFoffsets",12,&offId,status);
    GitArgNamePos(DitsGetArgument(),"tdFfiducials",13,&fidId,status);
    if (check & NO_DELTA)  /* Do not need current field details if NO_DELTA specified */
        sprintf(name,"blank");
    else {
        GitArgNamePos(DitsGetArgument(),"tdFcurrent",14,&curId,status);
        GitArgGetS(DitsGetArgument(),"name",15,0,0,GIT_M_ARG_KEEPERR,
                   sizeof(name),name,&index,status);
    }
    /*
     * If special flag is set, need to extSpringOut value.
     */
    if (check & SPECIAL) {
        GitArgGetI(DitsGetArgument(),"extSpringOut",16,0,0,
                   GIT_M_ARG_KEEPERR,&extSpringOut,status);
    }
    if (*status != STATUS__OK) {
        ErsRep(0,status,"Error getting %s argument(s) - %s",
               tdFdeltaActionName(),DitsErrorText(*status));
        return;
    }


    /*
     *  Create parameter structure to contain all action parameters.
     */
    if ((data = (tdFdeltaType *)malloc(sizeof(tdFdeltaType))) == NULL) {
        *status = TDFDELTA__MALLOCERR;
        return;
    }
    else {
        data->check = check;
        data->maxButAngG = maxButAngG;
        data->maxPivAngG = maxPivAngG;
        data->maxButAngO = maxButAngO;
        data->maxPivAngO = maxPivAngO;
        data->butClearG = butClearG;
        data->fibClearG = fibClearG;
        data->butClearO = butClearO;
        data->fibClearO = fibClearO;
        data->extSpringOut = extSpringOut;
        data->above     = 0;
        if (ErsSPrintf(sizeof(data->name),data->name,"%s",name) == EOF) {
            *status = TDFDELTA__SPRINTF;
            free((void *)data);
            return;
        }
    }

    /*
     *  Convert SDS structures to C structures.
     */
    tdFdeltaConvertConToC(conId,maxFibExt, &data->constants,check,status);
    tdFdeltaConvertOffToC(offId,&data->offsets_,check,status);
    tdFdeltaConvertTarToC(tarId,&data->target,check,status);
    tdFdeltaConvertFidToC(fidId,&data->fids,check,status);
    if (!(check & NO_DELTA))
        tdFdeltaConvertCurToC(curId,&data->current,&data->crosses,
                              &data->above, check,status);
    if (*status != STATUS__OK) {
        free((void *)data);
        return;
    }

    /*
     *  Reschedule the field validity checking.
     */
    DitsPutActData(data,status);
    DitsPutHandler(tdFdeltaFieldCheck,status);
    DitsPutRequest(DITS_REQ_STAGE,status);
}



/*
 *+           T D F D E L T A

 *  Function name:
      tdFdeltaFpilInst

 *  Function:
      Returns the FPIL instrument variable

 *  Description:
      Returns the address of the variable initialised by tdFdeltaActivate
	      using FpilInit()

	 *  Language:
	      C

	 *  Call:
	      (FpilType *) = tdFdeltaFpilInst ()


	 *  Prior requirements:
	      tdFdeltaActivate() must have been invoked.

 *  Support: Tony Farrell, AAO

 *-

 *  History:
      28-Jan-2000 TJF Original version
      {@change entry@}
 */
TDFDELTA_INTERNAL FpilType tdFdeltaFpilInst()
{
    return tdFdeltaInstrument;
}
