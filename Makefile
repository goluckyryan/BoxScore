########################################################################
#
##              --- CAEN SpA - Computing Division ---
#
##   CAENDigitizer Software Project
#
##   Created  :  October    2009      (Rel. 1.0)
#
##   Auth: A. Lucchesi
#
#########################################################################
ARCH	=	`uname -m`

OUTDIR  =    	.

OUTNAME1 =    	RealTimeReading
OUT1     =    	$(OUTDIR)/$(OUTNAME1)

OUTNAME2 =    	DetectDigitizer
OUT2     =    	$(OUTDIR)/$(OUTNAME2)

CC	=	gcc

COPTS	=	-fPIC -DLINUX -O2

#FLAGS	=	-soname -s
#FLAGS	=       -Wall,-soname -s
#FLAGS	=	-Wall,-soname -nostartfiles -s
FLAGS	=	-w

DEPLIBS	=	-lCAENDigitizer

LIBS	=	-L..

INCLUDEDIR =	-I./include

OBJS1	=	src/keyb.o src/Functions.o src/RealTimeReading.o 

OBJS2   =   src/keyb.o src/DetectDigitizer.o 

INCLUDES =	./include/*

#########################################################################

all	:	$(OUT1) $(OUT2)

clean	:
		/bin/rm -f $(OBJS1) $(OBJS2) $(OUT1) $(OUT2)

$(OUT1)	:	$(OBJS1)
		$(CC) $(FLAGS) -o $(OUT1) $(OBJS1) $(DEPLIBS)

$(OUT2)	:	$(OBJS2)
		$(CC) $(FLAGS) -o $(OUT2) $(OBJS2) $(DEPLIBS)

$(OBJS1)	:	$(INCLUDES) Makefile

$(OBJS2)	:	$(INCLUDES) Makefile

%.o	:	%.c
		$(CC) $(COPTS) $(INCLUDEDIR) -c -o $@ $<

