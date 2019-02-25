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

CC	=	g++

COPTS	=	-fPIC -DLINUX -w

#FLAGS	=	-soname -s
#FLAGS	=       -Wall,-soname -s
#FLAGS	=	-Wall,-soname -nostartfiles -s
#FLAGS	=	-w

DEPLIBS	=	-lCAENDigitizer

LIBS	=	-L..

INCLUDEDIR =	-I./include

OBJS	=	src/RealTimeReading.o 

OBJS1	=	src/keyb.o src/Functions.o 

OBJS2   =   src/keyb.o src/DetectDigitizer.o 

INCLUDES =	./include/*

#########################################################################

all	:	$(OUT1) $(OUT2) CutsCreator

clean	:
		/bin/rm -f $(OBJS) $(OBJS1) $(OBJS2) $(OUT1) $(OUT2)

$(OUT1)	:	$(OBJS) $(OBJS1)
		$(CC) $(FLAGS) -o $(OUT1) $(OBJS) $(OBJS1) $(DEPLIBS) `root-config --cflags --glibs`

$(OUT2)	:	$(OBJS2)
		$(CC) $(FLAGS) -o $(OUT2) $(OBJS2) $(DEPLIBS)
		
CutsCreator:	$(OBJS3) src/CutsCreator.c
		g++ -std=c++11 -pthread src/CutsCreator.c -o CutsCreator `root-config --cflags --glibs`

$(OBJS)	:	src/RealTimeReading.c
		$(CC) $(FLAGS) $(INCLUDEDIR) -c -o $(OBJS) src/RealTimeReading.c `root-config --cflags --glibs`

$(OBJS1)	:	$(INCLUDES) Makefile

$(OBJS2)	:	$(INCLUDES) Makefile


%.o	:	%.c
		$(CC) $(COPTS) $(INCLUDEDIR) -c -o $@ $<

