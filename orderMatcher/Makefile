include ../Makefile.inc

# File names
OBJECTS = OrderMatcher.o
OBJLIBS = ../$(LIBS)/liborder_matcher.a
# PREFIX is mandatory variable to point headerfiles recursively
# PREFIX is the relative path of External Include Libraries
PREFIX = ..

all: $(OBJLIBS)

$(OBJLIBS): $(OBJECTS)
	@echo
	$(AR) $(ARFLAGS) $(OBJLIBS) $?
	$(RANLIB) $(OBJLIBS)
	@echo
# To obtain object files
$(OBJECTS):
	@echo
	$(CC) -c $(CC_FLAGS) $(LD_FLAGS)-I../$(HEADERS) -L../$(LIBS) $(EXT_REC_HEADERS) -L../$(EXT_LIBS) OrderMatcher.cpp
# To remove generated files
clean:
	@echo
	@echo removing $(OBJECTS) and $(OBJECLIBS)
	rm -f $(OBJECTS)
	rm -f $(OBJLIBS)
	@echo done!
	@echo
