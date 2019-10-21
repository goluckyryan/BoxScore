CC=g++

DEPLIBS=-lCAENDigitizer

all: GUIDict GUIBoxScore

GUIDict: GUIBoxScore.h GUILinkDef.h 
	rootcling -f GUIDict.cxx -c GUIBoxScore.h GUILinkDef.h 

GUIBoxScore: GUIBoxScore.cpp GUIBoxScore.h DigitizerClass.h
	$(CC) -o GUIBoxScore GUIBoxScore.cpp GUIDict.cxx -lCAENDigitizer `root-config --cflags --glibs`
