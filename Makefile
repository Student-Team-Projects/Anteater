#
# 'make'        build executable file 'main'
# 'make clean'  removes all .o and executable files
#

# local directories for build
OBJ		:= obj
BIN		:= bin
SRC		:= src
INCLUDE	:= include
CHISEL := proc_tree.lua

# define package name
PKGNAME := debugger

# installation locations
TEMPLATEDIR := $(DESTDIR)/etc/$(PKGNAME)
BINDIR := $(DESTDIR)/usr/bin
CHISELDIR := $(DESTDIR)/usr/share/sysdig/chisels
TARGET := $(DESTDIR)/usr/share/$(PKGNAME)

#define template directories
JSDIR := src/front/js_templates
HTMLDIR := src/front/html_templates
CSSDIR := src/front/css

# define the Cpp compiler to use
CXX = g++

# define any compile-time flags
CXXFLAGS	:= -std=c++17 -Wall -Wextra -g -D TEMPLATE_DIR="\"$(TEMPLATEDIR)\"" -D CHISEL="\"$(CHISEL)\""

# define html and css templates
HTMLTEMPLATE := exec_template.html
CSSTEMPLATE := style.css

# define html and css files of execs
MAINHTMLFILE := index.html
MAINCSSFILE := style.css

MAIN	:= main
SOURCEDIRS	:= $(shell find $(SRC) -type d)
INCLUDEDIRS	:= $(shell find $(INCLUDE) -type d)
# FIXPATH = $1
RM = rm -f
MD	:= mkdir -p
CP := cp
RMDIR := rm -r -f
SYMLINK := ln -s

INCLUDES	:= $(patsubst %,-I %, $(INCLUDEDIRS:%/=%))
SOURCES		:= $(wildcard $(patsubst %,%/*.cpp, $(SOURCEDIRS)))#patsubst pattern,replacement,text
OBJECTS		:= $(patsubst $(SRC)/%.cpp, $(OBJ)/%.o, $(SOURCES)) #$(SOURCES:.cpp=.o)

#
# The following part of the makefile is generic; it can be used to 
# build any executable just by changing the definitions above and by
# deleting dependencies appended to the file from 'make depend'
#

# OUTPUTMAIN	:= $(call FIXPATH,$(BIN)/$(MAIN))
OUTPUTMAIN	:= $(BIN)/$(MAIN)


all: output $(MAIN)
	
	@echo Executing 'all' complete!

# make output directories
output:
	$(MD) $(OBJ) $(BIN)

chisel:
	$(CP) $(SRC)/$(CHISEL) $(CHISELDIR)

# make main executable
$(MAIN): $(OBJECTS) 
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $(OUTPUTMAIN) $(OBJECTS)
	

$(OBJ)/%.o : $(SRC)/%.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $<  -o $@


.PHONY: clean
clean:
	$(RMDIR) $(OBJ)
	$(RMDIR) $(BIN)
# $(RM) $(OUTPUTMAIN)
# $(RM) $(call FIXPATH,$(OBJECTS))
	@echo Cleanup complete!

template:
	$(MD) $(TEMPLATEDIR)
	$(CP) $(HTMLDIR)/$(HTMLTEMPLATE) $(TEMPLATEDIR)/$(MAINHTMLFILE)
	$(CP) $(CSSDIR)/$(CSSTEMPLATE) $(TEMPLATEDIR)/$(MAINCSSFILE)

run: all template
	./$(OUTPUTMAIN)
	@echo Executing 'run: all' complete!

install: chisel template
	$(MD) $(TARGET)
	$(CP) $(BIN)/$(MAIN) $(TARGET)
	$(MD) $(BINDIR)
	$(SYMLINK) $(TARGET)/$(MAIN) $(BINDIR)/$(PKGNAME)
	@echo Install complete!

uninstall:
	$(RM) $(BINDIR)/$(PKGNAME)
	$(RMDIR) $(TARGET)
	$(RM) $(CHISELDIR)/$(CHISEL)