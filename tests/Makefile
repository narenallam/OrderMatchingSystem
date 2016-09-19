include ../Makefile.inc

# File names
EXE = ../runtests
OBJS = AllTests.o
LINKLIBS = -lboost_unit_test_framework -lorder_matcher -llogger -lpthread	

# PREFIX is mandatory variable to point headerfiles recursively
# PREFIX is the relative path of External Include Libraries
PREFIX = ..

all:$(EXE)
	
$(EXE): $(OBJS)
	@echo
	$(LD) -o$(EXE) $(OBJS) -L ../$(LIBS)  $(LINKLIBS)
	ln -s ../runtests ./runtests
	@echo created softlink ./runtests

$(OBJS):
	@echo
	$(CC) -c $(CC_FLAGS) -I ../$(HEADERS) -L ../$(LIBS)  $(EXT_REC_HEADERS) AllTests.cpp
	@echo
# To remove generated files
clean:
	@echo
	@echo removing $(EXE) $(OBJS) and softlinks
	rm -f $(EXE)
	rm -f $(OBJS)
	rm -f ./runtests
	@echo done!
	@echo

