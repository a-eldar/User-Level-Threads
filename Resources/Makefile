CC=g++
CXX=g++

CODESRC= uthreads.cpp
EXESRC= $(CODESRC)
EXEOBJ= libuthreads.a
CODEOBJ = uthreads.o

INCS=-I.
CFLAGS = -Wall -std=c++11 -O3 $(INCS) -c
CXXFLAGS = -Wall -std=c++11 -O3 $(INCS) -c

TARGETS = $(EXEOBJ)

TAR=tar
TARFLAGS=-cvf
TARNAME=ex2.tar
TARSRCS=$(CODESRC) Makefile README

AR = ar
ARFLAGS = rvs

all: $(TARGETS)

$(TARGETS): $(EXESRC)
	$(CXX) $(CXXFLAGS) $^
	$(AR) $(ARFLAGS) $@ $(CODEOBJ)

clean:
	$(RM) $(TARGETS) $(CODEOBJ)

depend:
	makedepend -- $(CFLAGS) -- $(SRC) $(EXESRC)

tar:
	$(TAR) $(TARFLAGS) $(TARNAME) $(TARSRCS)
