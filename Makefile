# importing commons
include Makefile.inc

# Declaration of variables
# File names
EXE = run
EXE_DEBUG = rundebug 
TESTS = runtests
SUBDIRS = orderMatcher utils
OBJLIBS = -L./$(LIBS)
LINKLIBS	= -lorder_matcher -llogger -lpthread 
OBJS = Main.o

# PREFIX is mandatory variable to point headerfiles recursively
# PREFIX is the relative path of External Include Libraries
PREFIX = .

# Add fmt library support (properly defined to avoid duplication)
CC_FLAGS += -I$(EXT_HEADER)
LD_FLAGS += -L$(EXT_LIBS) -lfmt

# Added comments to explain the purpose of each target in the Makefile
# clean: Removes all compiled files and logs.
# all: Compiles the entire project.
# run: Executes the compiled binary.
# test: Runs the unit tests.

all: $(EXE)
debug: CC_FLAGS += -DDEBUG -g
debug: $(EXE)
tests: $(TESTS)

# Main executable target
$(EXE) : $(OBJS)
	@echo
	$(LD) -o$(EXE) $(OBJS) $(LD_FLAGS) -L./$(LIBS) -L$(EXT_LIBS) $(LINKLIBS)
	@echo

# Object files target
$(OBJS): $(OBJLIBS) 
	@echo
	$(CC) -c $(CC_FLAGS) -I./$(HEADERS) $(EXT_REC_HEADERS) -L./$(LIBS) -L$(EXT_LIBS) Main.cpp

# Test executable target
$(TESTS):
	@echo
	cd tests
	$(MAKE) $(MFLAGS)
	cd ..

# Subdirectory libraries target
$(OBJLIBS) :
	@echo looking into utils : $(MAKE) $(MFLAGS)
	cd utils; $(MAKE) $(MFLAGS)
	cd ..

	@echo
	@echo looking into orderMatcher : $(MAKE) $(MFLAGS)
	cd orderMatcher; $(MAKE) $(MFLAGS)
	cd ..

# Clean target to remove generated files
clean:
	@echo cleaning up in .
	-rm -f $(EXE) $(TESTS) $(EXE_DEBUG) $(OBJS) $(OBJLIBS)
	-for d in $(SUBDIRS); do (cd $$d; $(MAKE) clean ); done
	@echo
