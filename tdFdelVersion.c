/*+                     T D F D E L V E R S I O N

 *  Module name:
      TDFDELVERSION

 *  Function:
      Version module for TDFDELTA - 2dF Delta task.

 *  Description:
      This module is compiled into an object of the name
        
           tdFfpi_VER.o

      where VER is the release version number.  All this module
      does it allow the version number and date to be set from
      the make file.  Other modules reference the variables
      tdFdeltaVersion and tdFdeltaDate to get the version number and version
      date.

 *  Language:
      C

 *  Support: Tony Farrell, AAO

 *-

 *  History:
     10-Jun-1998 - TJF - Original version
 


 *     @(#) $Id: ACMM:2dFdelta/tdFdelVersion.c,v 3.8 19-Jun-2007 09:32:34+10 tjf $

 */
const char * const tdFdeltaVersion= TDFDELTA_VER;
const char * const tdFdeltaDate   = TDFDELTA_DATE;

