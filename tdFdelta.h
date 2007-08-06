/*+            T D F D E L T A

 *  Module name:
      tdFdelta.h

 *  Function:
      2dF Delta Configuration Task include file.
    
 *  Description:
      Include file for the 2dF Delta Configuration Task.

 *  Language:
      C

 *  Support: James Wilcox, AAO

 *-

 *  History:
      01-Jul-1994  JW   Original version
      10-Aug-1998  TJF  Add graspX/graspY to tdFconstants
      29-Jan-2000  TJF  Add tdFdeltaFpilInst protoyptes and include fpil.h
                        Add task name macros.
      01-Feb-2000  TJF  Drop NUM_PIVOTS macro definition.  For static
                        array definitons, use FPIL_MAXPIVOTS.  For actual
                        numbers of pivots, have to use FpilGetNumPivots().
      17-Mar-2000  TJF  Drop NUM_FIDUCIALS.  For static array definitions,
                        use FPIL_MAXFIDS, for actual number of fiducials,
                        have to use FpilGetNumFiducials()
      20-Oct-2000  TJF  Add above item to tdFdeltaType, allowing us to
                        keep a copy of the above structure from the
                        current details, so that it may be added to
                        the command file later.
      01-Nov-2000  TJF  Add SPECIAL flag.
      13-Feb-2001  TJF  Add support for a maximum fibre extension on a
                        fibre specific basis - to the constants structure.
      25-Mar-2001  TJF  Add extSpringOut item to tdFdeltaType structure.
      22-Sep-2002  TJF  Add tdFdeltaCFaddSpringOutParks() function.
      
      {@change entry@}

 *  @(#) $Id: ACMM:2dFdelta/tdFdelta.h,v 3.10 06-Aug-2007 11:40:12+10 tjf $ (mm/dd/yy)
 */

#ifndef __TDFDELTA_H__
#define __TDFDELTA_H__

#include "DitsTypes.h"
#include "sds.h"
#include "Git.h"
#include "status.h"
#include "fpil.h"

#define SIXDF "SIXDF"
#define SIXDF_TAKSNAME "SIXDFDELTA"
#define TWODF_TASKNAME "TDFDELTA"
/*
 *  Use to declare constant arguments.
 *  Under GNU may get some warnings when compiling TDFDELTA itself due to C
 *  routines not being declared correctly.
 */
#define TDFDELTA_CONST_ARGVAL  const   /* Use for constant argument values     */
#define TDFDELTA_CONST_ARGPNT  const   /* Use for constant argument pointers   */
#define TDFDELTA_CONST_VAR     const   /* Use for constant variables           */

/*
 *  Used to declare routines.
 */
#define TDFDELTA_PUBLIC   extern       /* routines available to other packages */
#define TDFDELTA_INTERNAL extern       /* routines available thoughout tdFpt   */
#define TDFDELTA_PRIVATE  static       /* routines internal to one module      */

/*
 *  2dF Delta Configuration Task definitions.
 */
#define PI             3.1415926535

#define NO                        0
#define YES                       1
#define IF_NEEDED                 2

#define GUIDE                     0    /* Fibre types                          */
#define OBJECT                    1

#define FILENAME_LENGTH         200    /* Max "Command File" name length       */
#define CMD_NAME_LENGTH          10    /* Command buffer length                */
#define COMMENT_LENGTH          100    /* Comment buffer length                */
#define CMDLINE_LENGTH          500    /* Command line maximum length          */

#define RESOLUTION              3.0    /* Only update the DELTA_PROG param ... */
                                       /* ... for % changes greater than this  */
                                       /* Note - must be float value           */

#define TDFDELTA_MSG_BUFFER  250000    /* Size of message buffer for TDFDELTA  */

/*
 *  Used to set check word that is passed between most functions.
 */
#define CHECK_ALL                 0    /* Reset check mask                     */
#define _DEBUG               (1<<0)    /* Display debugging infomation         */
#define DISPLAY              (1<<1)    /* Display useful infomation            */
#define SHOW     (_DEBUG | DISPLAY)    /* Debug or display flag specified      */
#define CHECK_FULL_FIELD     (1<<2)    /* Check validity of ALL fibres         */
#define NO_DELTA             (1<<3)    /* Do not perform delta process         */
#define NO_ORDER_CHECK       (1<<4)    /* Bypass delta checks                  */
#define NO_FIELD_CHECK       (1<<5)    /* Do not perform field validity checks */

#define SPECIAL              (1<<6)    /* Run the special mode delta for 6dF */

/*
 *  Macro's
 */
#define SQRD(x)           ((x)*(x))    /* Returns the square of the number     */
#define MAXMIN(a,b,c,d) ((c=a)>(d=b)?(c):(c-=d,d+=c,c=d-c))  /* JOS macro to.. */
                                       /* take a and b, and put max in c and.. */
                                       /* min in d.                            */


/*
 *  Interim details - these are the details that will be continually changing
 *  during the generation of command files. All units are either microns
 *  or radians.
 */
typedef struct tdFinterim {
      double    theta[FPIL_MAXPIVOTS]; /* Button handle orientation          */
      double    fibreLength[FPIL_MAXPIVOTS];/* Pivot-button distance         */
      long int  fvpX[FPIL_MAXPIVOTS];  /* Fibre virtual pivot point - x ord. */
      long int  fvpY[FPIL_MAXPIVOTS];  /* Fibre virtual pivot point - y ord. */
      long int  xf[FPIL_MAXPIVOTS];    /* Fibre-end x coordinate             */
      long int  yf[FPIL_MAXPIVOTS];    /* Fibre-end y coordinate             */
      long int  xb[FPIL_MAXPIVOTS];    /* Button x coordinate                */
      long int  yb[FPIL_MAXPIVOTS];    /* Button y coordinate                */
      short     park[FPIL_MAXPIVOTS];  /* Flag indicating button is parked   */
      short     nAbove[FPIL_MAXPIVOTS];/* Number of fibres crossing above    */
      short     nBelow[FPIL_MAXPIVOTS];/* Number of fibres crossing below    */
      } tdFinterim;

/*
 *  Positioning offsets.  This is the offset that occurs when a button
 *  is placed.  There are three offsets for each button, one for moves
 *  from the park position , one for moves to the park position and
 *  one for all other moves (plate to plate)
 */
typedef struct tdFoffsets {
      short  xOffPL[FPIL_MAXPIVOTS]; /* Positioning offset - x, plate to plate  */
      short  yOffPL[FPIL_MAXPIVOTS]; /* Positioning offset - y, plate to plate  */
      short  xOffFrPK[FPIL_MAXPIVOTS]; /* Positioning offset - x, park  to plate  */
      short  yOffFrPK[FPIL_MAXPIVOTS]; /* Positioning offset - y, park  to plate  */
      short  xOffToPK[FPIL_MAXPIVOTS]; /* Positioning offset - x, plate to park */
      short  yOffToPK[FPIL_MAXPIVOTS]; /* Positioning offset - y, plate to park */
      } tdFoffsets;

/*
 *  Fidcuial locations.
 */
typedef struct tdFfidcuials {
      long int  fidX[FPIL_MAXFIDS];   /* Fiducial mark X ordinate (mic)     */
      long int  fidY[FPIL_MAXFIDS];   /* Fiducial mark Y ordinate (mic)     */
      short     inUse[FPIL_MAXFIDS];  /* Is the fiducial currently in use   */
      } tdFfiducials;

/*
 *  Target structure - details of target field.
 */
typedef struct tdFtarget {
      double    theta[FPIL_MAXPIVOTS]; /* Button handle orientation          */
      double    fibreLength[FPIL_MAXPIVOTS]; /* Pivot-button distance        */
      long int  fvpX[FPIL_MAXPIVOTS];  /* Fibre virtual pivot point - x ord. */
      long int  fvpY[FPIL_MAXPIVOTS];  /* Fibre virtual pivot point - y ord. */
      long int  xf[FPIL_MAXPIVOTS];    /* Fibre-end x coordinate             */
      long int  yf[FPIL_MAXPIVOTS];    /* Fibre-end y coordinate             */
      short     mustMove[FPIL_MAXPIVOTS];/* Flag indicating button must move */
      short     park[FPIL_MAXPIVOTS];  /* Flag indicating button is parked   */
      } tdFtarget;

/*
 *  Field constants - should be constant during observations.
 */
typedef struct tdFconstants {
      double    tPark[FPIL_MAXPIVOTS]; /* Park position - theta coordinate   */
      long int  xPark[FPIL_MAXPIVOTS]; /*               - x coordinate       */
      long int  yPark[FPIL_MAXPIVOTS]; /*               - y coordinate       */
      long int  xPiv[FPIL_MAXPIVOTS];  /* Pivot location - x coordinate      */
      long int  yPiv[FPIL_MAXPIVOTS];  /*                - y coordinate      */
      short     type[FPIL_MAXPIVOTS];  /* Type of pivot - GUIDE/OBJECT       */
      short     inUse[FPIL_MAXPIVOTS]; /* Flag indicating if pivot is broken */
      long int  graspX[FPIL_MAXPIVOTS];/* Fibre-end location during...       */
      long int  graspY[FPIL_MAXPIVOTS];/*                   ...button grasp  */
      unsigned long maxExt[FPIL_MAXPIVOTS]; /* Maximum fibre extension */
      } tdFconstants;

/*
 *  Crossover details - details of the crossovers between the fibres.
 *  When a crossover is detected, the level number is used to determine
 *  which fibre crosses above the other. This information is then used to
 *  construct a linked list of the numbers of fibres crossing above, and the
 *  numbers of the fibres crossing below each fibre.
 */
typedef struct FibreCross {
      short             piv;              /* Number of the fibre crossing    */
      struct FibreCross *next;            /* Pointer to next pivot number    */
} FibreCross;

typedef struct tdFcrosses {
      FibreCross   *above[FPIL_MAXPIVOTS];/* Numbers of fibres crossing above*/
      FibreCross   *below[FPIL_MAXPIVOTS];/* Numbers of fibres crossing above*/
} tdFcrosses;


/*
 *  Action structs (used with DitsPutActData and DitsGetActData).
 */
typedef struct tdFdeltaType {
      tdFinterim      current;
      tdFcrosses      crosses;
      tdFconstants    constants;
      tdFoffsets      offsets_;  /* Underscore was only to have compiler pick 
                                    up occurances of this variable */
      tdFfiducials    fids;
      tdFtarget       target;
      double          maxButAngG;
      double          maxButAngO;
      double          maxPivAngG;
      double          maxPivAngO;
      long int        butClearG;
      long int        butClearO;
      long int        fibClearG;
      long int        fibClearO;
      long int        extSpringOut;
      short           check;
      char            name[FILENAME_LENGTH];

      SdsIdType       above;   /* Sds ID of a copy of the above item from
                                  the intial current details structdure 
                               */
      }  tdFdeltaType;


/*
 *  Function prototypes.
 */

/*
 *  MODULE = tdFdelta
 */
TDFDELTA_PUBLIC void  tdFdeltaActivate (
        SdsIdType   parsysid,
        StatusType  *status);
/*
 *  MODULE = tdFdeltaUtil
 */
TDFDELTA_INTERNAL void  tdFdeltaGetFlag (
        SdsIdType   actArgId,
        char        *stringName,
        short       *found,
        StatusType  *status);
TDFDELTA_INTERNAL char  *tdFdeltaActionName (
        void);
TDFDELTA_INTERNAL void  tdFdeltaFlagCheck (
        SdsIdType   paramId,
        short       checkFor,
        short       *argFlags,
        StatusType  *status);
TDFDELTA_INTERNAL void  tdFdeltaKick (
        StatusType  *status);
/*
 *  MODULE = tdFdeltaConvert
 */
TDFDELTA_INTERNAL void  tdFdeltaConvertConToC (
        SdsIdType     conId,
        unsigned long maxExt,
        tdFconstants  *con,
        short         check,
        StatusType    *status);
TDFDELTA_INTERNAL void  tdFdeltaConvertCurToC (
        SdsIdType   curId,
        tdFinterim  *cur,
        tdFcrosses  *crosses,
        SdsIdType   *above,
        short       check,
        StatusType  *status);
TDFDELTA_INTERNAL void  tdFdeltaConvertOffToC (
        SdsIdType   offId,
        tdFoffsets  *off,
        short       check,
        StatusType  *status);
TDFDELTA_INTERNAL void  tdFdeltaConvertFidToC (
        SdsIdType     fidId,
        tdFfiducials  *fid,
        short         check,
        StatusType    *status);
TDFDELTA_INTERNAL void  tdFdeltaConvertTarToC (
        SdsIdType   tarId,
        tdFtarget   *tar,
        short       check,
        StatusType  *status);
/*
 *  MODULE = tdFdeltaFieldCheck
 */
TDFDELTA_INTERNAL void  tdFdeltaFieldCheck (
        StatusType  *status);
/*
 *  MODULE = tdFdeltaCrosses
 */
TDFDELTA_INTERNAL void  tdFdeltaAddCross (
        int         newCross,
        FibreCross  **start,
        StatusType  *status);
TDFDELTA_INTERNAL void  tdFdeltaDeleteCross (
        int         cross,
        FibreCross  **start,
        StatusType  *status);
TDFDELTA_INTERNAL FibreCross  *tdFdeltaFindCross(
        int         cross,
        FibreCross  **start,
        StatusType  *status);
/*
 *  MODULE = tdFdeltaSequencer
 */
TDFDELTA_INTERNAL void  tdFdeltaSequencer (
        StatusType  *status);
/*
 *  MODULE = tdFdeltaSeqSp
 */
TDFDELTA_INTERNAL void  tdFdeltaSequencerSpecial (
        StatusType  *status);/*
 *  MODULE = tdFdeltaCmdFile
 */
TDFDELTA_INTERNAL SdsIdType  tdFdeltaCFnew (
        char        *name,
        tdFinterim  *currDetails,
        SdsIdType   *above,
        StatusType  *status);
TDFDELTA_INTERNAL void  tdFdeltaCFaddMoves (
        SdsIdType   cmdFileId,
        long int    numMoves,
        long int    numParks,
        StatusType  *status);
TDFDELTA_INTERNAL void  tdFdeltaCFaddSpringOutParks (
        SdsIdType   cmdFileId,
        long int    numSpringOutParks,
        StatusType  *status);

#ifdef DSTDARG_OK
    TDFDELTA_INTERNAL void  tdFdeltaCFaddCmd (
            StatusType  *status,
            SdsIdType   cmdFileId,
            int         lineNo,
            char        *cmd,
            ...);
#else
    TDFDELTA_INTERNAL void  tdFdeltaCFaddCmd ();
#endif



TDFDELTA_INTERNAL FpilType tdFdeltaFpilInst();
#endif
