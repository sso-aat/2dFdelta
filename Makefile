# SCCS Makefile for the tdFdelta system
#
#
# These macros define the default Release, Man and Documentation directories.
#	Override on the make command line to change the directories
REL_DIR=.
MAN_DIR=.
DOC_DIR=.
# Default class name and remark for the "newclass" target.  By being null,
# newclass won't run and you will have to specify them.
CLASS=
REMARK=
# Location ofthe dmkmf command, used by some targets
DMKMF=$$CONFIG_DIR/dmkmf
# Location of $(WFLMAN)
WFLMAN=wflman
# The sccs command used to get files from the SCCS directory
.SCCS_GET:
	acmmFileCopy 2dFdelta -s $@ -G$(REL_DIR)/$@

# Various SCCS Groups.
RELEASE = dmakefile tdFdelta.c tdFdelta.h tdFdelCmdFile.c \
tdFdelConvert.c tdFdelCrosses.c tdFdelFieldCh.c tdFdelMain.c \
tdFdelSeq.c tdFdelUtil.c tdFdelta_Err.msg tdFdelVersion.c \
tdFdelSeqSp.c
# Fetch the group RELEASE
release : $(RELEASE)

# Targets to fetch the group RELEASE and run dmkmf
vaxvms_release : $(RELEASE)
	(cd $(REL_DIR) ; $(DMKMF) -t vaxvms -f)

sun4_release : $(RELEASE)
	(cd $(REL_DIR) ; $(DMKMF) -t sun4)

decstation_release : $(RELEASE)
	(cd $(REL_DIR) ; $(DMKMF) -t decstation)

vw68k_release : $(RELEASE)
	(cd $(REL_DIR) ; $(DMKMF) -t vw68k)


# Generate a new class
newclass :
	@ if [ "$(CLASS)" = "" ]; then echo No class name supplied; exit 1;fi
	@ if [ "$(REMARK)" = "" ]; then echo No remark supplied; exit 1; fi
	@ if [ -f SCCS/s.class-$(CLASS) ]; then echo  class $(CLASS) already exists ; exit 1 ; fi
	@ echo "#Class $(CLASS) created by $$USER at "`date` >class-$(CLASS)
	@ echo "#$(REMARK) " >> class-$(CLASS)
	@ echo "# " >> class-$(CLASS)
	sccs prs -d"acmmFileCopy 2dFdelta :M: -r:I:" $(RELEASE) >> class-$(CLASS)
	acmmFileCopy 2dFdelta -e -p dmakefile |  sed 's/RELEASE=.*/RELEASE=$(CLASS)/' >dmakefile
	sccs delta dmakefile -y"$(REMARK)"
	sccs create class-$(CLASS)
	sccs clean	
	rm -f ,class-$(CLASS)
