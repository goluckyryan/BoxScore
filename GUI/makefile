CC=g++

all: testDict test

testDict: test.h testLinkDef.h
	rootcling -f testDict.cxx -c test.h testLinkDef.h

test: test.cpp test.h
	$(CC) -o test test.cpp testDict.cxx `root-config --cflags --glibs`
