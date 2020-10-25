# ------------------------------------------------
# PCSKY
#
# Author: David Basler
# Date  : 2020-10-21
#
# Changelog :
#   2020-10-21 - first version
# ------------------------------------------------

# project name
TARGET_SKY   = pcsky
TARGET_RASTER   = pcrasterize

CC       = g++
# compiling flags here
CFLAGS   =  -I. #-Wall

LINKER   = g++
# linking flags here
LFLAGS   =  -I. -lm -L/usr/include/x86_64-linux-gnu/tiffio.h -ltiff #-Wall


# directories
SRCDIR   = src
OBJDIR   = obj
BINDIR   = bin
TESTDIR  = test

SOURCES  := $(SRCDIR)/pcsky.cpp $(SRCDIR)/pcrasterize.cpp  $(SRCDIR)/miniply.cpp $(SRCDIR)/hemisphere.cpp $(SRCDIR)/io.cpp
INCLUDES := $(wildcard $(SRCDIR)/*.h)
OBJECTS  := $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
OBJECTS_SKY    := $(OBJDIR)/pcsky.o  $(OBJDIR)/miniply.o $(OBJDIR)/hemisphere.o $(OBJDIR)/io.o
OBJECTS_RASTER := $(OBJDIR)/pcrasterize.o  $(OBJDIR)/miniply.o $(OBJDIR)/io.o
rm       = rm -rf

all: $(BINDIR)/$(TARGET_SKY) $(BINDIR)/$(TARGET_RASTER)

$(BINDIR)/$(TARGET_SKY): $(OBJECTS_SKY)
	@mkdir -p $(@D)
	@$(LINKER) $(OBJECTS_SKY) $(LFLAGS) -o $@
	@echo "Linking pcsky complete!"

$(BINDIR)/$(TARGET_RASTER): $(OBJECTS_RASTER)
	@mkdir -p $(@D)
	@$(LINKER) $(OBJECTS_RASTER) $(LFLAGS) -o $@
	@echo "Linking pcrasterize complete!"

$(OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.cpp 
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compiled "$<" successfully!"

.PHONY: clean
clean:
	@$(rm) $(OBJDIR)
	@echo "Cleanup complete!"
.PHONY: remove
remove: clean
	@$(rm) $(BINDIR)/$(TARGET_SKY)
	@$(rm) $(BINDIR)/$(TARGET_RASTER)
	@$(rm) $(TESTDIR)/tests
	@echo "Executable removed!"



TESTOBJECTS := $(OBJDIR)/tests.o $(OBJDIR)/miniply.o $(OBJDIR)/hemisphere.o $(OBJDIR)/io.o
tests: $(OBJECTS) $(TESTDIR)/tests
$(TESTDIR)/tests: $(OBJDIR)/tests.o
	@mkdir -p $(@D)
	@$(LINKER) $(TESTOBJECTS) $(LFLAGS) -o $@
	@echo "Linking tests complete!"
$(OBJDIR)/tests.o: $(OBJDIR)/%.o : $(SRCDIR)/tests.cpp 
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo "Compiled "$<" successfully!"


