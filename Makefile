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

all: $(EXE)
debug: CC_FLAGS += -DDEBUG -g
debug: $(EXE)
tests: $(TESTS)

$(EXE) : $(OBJS)
	@echo
	$(LD) -o$(EXE) $(OBJS) $(LD_FLAGS) -L./$(LIBS) -L./$(EXT_LIBS) $(LINKLIBS)
	@echo
$(OBJS): $(OBJLIBS) 
	@echo
	$(CC) -c $(CC_FLAGS) $(LD_FLAGS) -I./$(HEADERS) $(EXT_REC_HEADERS) -L ./$(LIBS) -L ./$(EXT_LIBS) Main.cpp
$(TESTS):
	@echo
	cd tests
	$(MAKE) $(MFLAGS)
	cd ..
$(OBJLIBS) :
	@echo looking into utils : $(MAKE) $(MFLAGS)
	cd utils; $(MAKE) $(MFLAGS)
	cd ..

	@echo
	@echo looking into orderMatcher : $(MAKE) $(MFLAGS)
	cd orderMatcher; $(MAKE) $(MFLAGS)
	cd ..
clean:
	@echo cleaning up in .
	-rm -f $(EXE) $(TESTS) $(EXE_DEBUG) $(OBJS) $(OBJLIBS)
	-for d in $(SUBDIRS); do (cd $$d; $(MAKE) clean ); done
	@echo
