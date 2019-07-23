CC=g++

DEPLIBS=-lCAENDigitizer

#all: testDict test testClass GUIDict GUIBoxScore
all: GUIDict GUIBoxScore


testDict: test.h testLinkDef.h
	rootcling -f testDict.cxx -c test.h testLinkDef.h

test: test.cpp test.h
	$(CC) -o test test.cpp testDict.cxx `root-config --cflags --glibs`

testClass: testClass.cpp DigitizerClass.h
	$(CC) -o testClass testClass.cpp -lCAENDigitizer



GUIDict: GUIBoxScore.h GUILinkDef.h DigitizerClass.h
	rootcling -f GUIDict.cxx -c GUIBoxScore.h DigitizerClass.h GUILinkDef.h 

GUIBoxScore: GUIBoxScore.cpp GUIBoxScore.h DigitizerClass.h
	$(CC) -o GUIBoxScore GUIBoxScore.cpp GUIDict.cxx -lCAENDigitizer `root-config --cflags --glibs`
