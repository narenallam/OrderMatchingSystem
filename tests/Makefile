include ../Makefile.inc

# File names
EXE = ../runtests
OBJS = AllTests.o
LINKLIBS = -lboost_unit_test_framework -lorder_matcher -llogger -lpthread -lfmt

# Add fmt include path and lib path
FMT_INCLUDE_PATH := /opt/homebrew/include
FMT_LIB_PATH := /opt/homebrew/lib

# Update compiler flags to include fmt
CC_FLAGS += -I$(FMT_INCLUDE_PATH)
LD_FLAGS += -L$(FMT_LIB_PATH)	

# PREFIX is mandatory variable to point headerfiles recursively
# PREFIX is the relative path of External Include Libraries
PREFIX = ..

all:$(EXE)
	
$(EXE): $(OBJS)
	@echo
	$(LD) -o$(EXE) $(OBJS) $(LD_FLAGS) -L ../$(LIBS) -L $(FMT_LIB_PATH) $(LINKLIBS)
	ln -s ../runtests ./runtests
	@echo created softlink ./runtests

$(OBJS):
	@echo
	$(CC) -c $(CC_FLAGS) -I ../$(HEADERS) -I $(FMT_INCLUDE_PATH) -L ../$(LIBS) -L $(FMT_LIB_PATH) $(EXT_REC_HEADERS) AllTests.cpp
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

