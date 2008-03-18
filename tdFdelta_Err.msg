! "@(#) $Id: ACMM:2dFdelta/tdFdelta_Err.msg,v 3.12 18-Mar-2008 13:05:03+11 tjf $"
.TITLE   TDFDELTA_ERR  2dF Delta Configuration Task
.FACILITY TDFDELTA,1844/PREFIX=TDFDELTA__
.SEVERITY ERROR
.BASE 2
MALLOCERR "Error allocating memory"
SPRINTF "Format string exceeds available string length (ErsSPrintf)"
INVARG "Invalid argument value"
INITIALISED "Initialise command received when task is already initialised"
NOTINIT "Task not initialised"
NOPARAM "Invalid parameter name"
DELTAERR "DELTA - Unable to generate command file"
DELTAPARKERR "DELTA - Unable to find fibre that can be legally parked"
CF_NOCMD "No such (command file) command available"
FIBRETYPEERR "Actual fibre type does not match expected fibre type"
SDSTOC "Error while converting SDS structure to C structure"
CROSSESERR "Inconsistancy with crossover lists"
OUTOFRANGE "Number(s) not within valid range"
INVFIELD "Invalid field configuration detected"
UNKNOWN_ERR "Reason for error unknown"
.END
