CC=g++
CXX=g++

CODESRC= uthreads.cpp
EXESRC= $(CODESRC) uthreads.h
EXEOBJ= libuthreads.a

INCS=-I.
CFLAGS = -Wall -std=c++11 -O3 $(INCS) -o
CXXFLAGS = -Wall -std=c++11 -O3 $(INCS) -o

TARGETS = $(EXEOBJ)

TAR=tar
TARFLAGS=-cvf
TARNAME=ex2.tar
TARSRCS=$(CODESRC) Makefile README

all: $(TARGETS)

$(TARGETS): $(EXESRC)
	$(CXX) $(CXXFLAGS) $@ $^

clean:
	$(RM) $(TARGETS)

depend:
	makedepend -- $(CFLAGS) -- $(SRC) $(EXESRC)

tar:
	$(TAR) $(TARFLAGS) $(TARNAME) $(TARSRCS)
