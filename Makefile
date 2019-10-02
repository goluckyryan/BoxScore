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

OUT1     =    BoxScore
OUT2     =    DetectDigitizer

CC	=	g++

COPTS	=	-fPIC -DLINUX -w

DEPLIBS	=	-lCAENDigitizer

INCLUDEDIR =	-I./include

OBJS	=	src/BoxScore.o 
OBJS1	=	src/keyb.o src/Functions.o 
OBJS2   =   src/keyb.o src/DetectDigitizer.o 

INCLUDES =	./include/*

ROOTLIBS = `root-config --cflags --glibs`

#########################################################################

all	:	$(OUT1) $(OUT2) CutsCreator BoxScoreXY

clean	:
		/bin/rm -f $(OBJS) $(OBJS1) $(OBJS2) $(OUT1) $(OUT2)

$(OUT1)	:	$(OBJS) $(OBJS1)
		$(CC) -o $(OUT1) $(OBJS) $(OBJS1) $(DEPLIBS) $(ROOTLIBS)

$(OUT2)	:	$(OBJS2)
		$(CC) -o $(OUT2) $(OBJS2) $(DEPLIBS)

$(OBJS)	:	src/BoxScore.c
		$(CC) $(FLAGS) $(INCLUDEDIR) -c -o $(OBJS) src/BoxScore.c $(ROOTLIBS)

$(OBJS1)	:	$(INCLUDES) Makefile

$(OBJS2)	:	$(INCLUDES) Makefile

%.o	:	%.c
		$(CC) $(COPTS) $(INCLUDEDIR) -c -o $@ $<

CutsCreator:	$(OBJS3) src/CutsCreator.c
		g++ -std=c++11 -pthread src/CutsCreator.c -o CutsCreator $(ROOTLIBS)

BoxScoreXY	: src/BoxScoreXY.c GUI/DigitizerClass.h
		g++ -std=c++11 -pthread src/BoxScoreXY.c -o BoxScoreXY  $(DEPLIBS) $(ROOTLIBS)



