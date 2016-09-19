CC = g++
CC_FLAGS = -w -std=c++11 -Wall
LD = g++
LD_FLAGS = -O3 
MAKE = make
MFLAGS = -s
ECHO = echo
AR = ar
ARFLAGS = rcv
RANLIB = ranlib
ROOT = $(shell pwd)
EXT_HEADER = /ExternalLibraries/include
PREFIX = 
EXT_REC_HEADERS = $(addprefix -I,$(shell find $(PREFIX)$(EXT_HEADER) -type d -print))
EXT_LIBS = ExternalLibraries/lib
HEADERS = Include
LIBS = Lib
