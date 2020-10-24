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

OUT2     =    DetectDigitizer

CC	=	g++

COPTS	=	-fPIC -DLINUX -w

DEPLIBS	=	-lCAENDigitizer

INCLUDEDIR =	-I./include

OBJS1	=	src/keyb.o src/Functions.o 
OBJS2   =   src/keyb.o src/DetectDigitizer.o 

INCLUDES =	./include/*

ROOTLIBS = `root-config --cflags --glibs`

#########################################################################

all	:	$(OUT2) CutsCreator BoxScore BoxScoreReader

clean	:
		/bin/rm -f $(OBJS1) $(OBJS2) $(OUT2)

$(OUT2)	:	$(OBJS2)
		$(CC) -o $(OUT2) $(OBJS2) $(DEPLIBS)

$(OBJS2)	:	$(INCLUDES) Makefile

%.o	:	%.c
		$(CC) $(COPTS) $(INCLUDEDIR) -c -o $@ $<

CutsCreator:	$(OBJS3) src/CutsCreator.c
		g++ -std=c++11 -pthread src/CutsCreator.c -o CutsCreator $(ROOTLIBS)

BoxScore	: src/BoxScore.c Class/DigitizerClass.h Class/FileIO.h Class/GenericPlane.h Class/HelioTarget.h Class/IsoDetect.h Class/HelioArray.h Class/MCPClass.h
		g++ -std=c++11 -pthread src/BoxScore.c -o BoxScore  $(DEPLIBS) $(ROOTLIBS)

BoxScoreReader: src/BoxScoreReader.c Class/GenericPlane.h Class/HelioTarget.h Class/IsoDetect.h Class/HelioArray.h
		g++ -std=c++11 src/BoxScoreReader.c -o BoxScoreReader $(ROOTLIBS)

