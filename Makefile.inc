# Compiler and flags for building the project
# CC: Compiler to use
CC = g++
# CC_FLAGS: Compiler flags for warnings and standards
CC_FLAGS = -w -std=c++23 -Wall
# LD: Linker to use
LD = g++
# LD_FLAGS: Linker flags for optimization
LD_FLAGS = -O3
# MAKE: Make command
MAKE = make
# MFLAGS: Make command flags
MFLAGS = -s
# ECHO: Command to print messages
ECHO = echo
# AR: Archiver for static libraries
AR = ar
# ARFLAGS: Flags for the archiver
ARFLAGS = rcv
# RANLIB: Command to generate index for static libraries
RANLIB = ranlib
# ROOT: Root directory of the project
ROOT = $(shell pwd)
# EXT_HEADER: Path to external library headers
EXT_HEADER = /opt/homebrew/include
# PREFIX: Prefix for external headers (removed since we're using absolute paths)
PREFIX = 
# EXT_REC_HEADERS: Include paths for external headers
# Modified to use direct includes instead of find
EXT_REC_HEADERS = -I/opt/homebrew/include
# EXT_LIBS: Path to external libraries
EXT_LIBS = /opt/homebrew/lib
# HEADERS: Path to project headers
HEADERS = Include
# LIBS: Path to project libraries
LIBS = Lib

# Added explicit Boost configuration - using system installed Boost 1.87
BOOST_VERSION = system
BOOST_LIBS = -lboost_unit_test_framework
