# Copyright 2016 Yahoo Inc.
# Licensed under the terms of the New-BSD license. Please see LICENSE file in the project root for terms.
UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Darwin)
# this is from /usr/libexec/java_home -v 1.8
#JAVA_HOME=/Library/Java/JavaVirtualMachines/jdk1.8.0_72.jdk/Contents/Home
JAVA_HOME=`/usr/libexec/java_home -v 1.8`
JAVA_OS=darwin
EXT=dylib
CC=clang++
TARGET_ARCH=x86_64-darwin-clang
endif

ifeq ($(UNAME_S),Linux)
JAVA_HOME?=/usr/java/default
JAVA_OS=linux
LINUX_ADD=amd64/
EXT=so
CC=g++
TARGET_ARCH=x86_64-linux-gcc
endif

# include rules that can change the locations
-include custom.rules

SOURCES_DIR=src/main/native
LIBS_DIR=target/native/$(TARGET_ARCH)
OBJECTS_DIR=$(LIBS_DIR)

LIB_SOURCES=$(shell find '$(SOURCES_DIR)' -type f -name '*.cpp')
LIB_OBJECTS=$(LIB_SOURCES:$(SOURCES_DIR)/%.cpp=$(OBJECTS_DIR)/%.o)

JAVA_LIBRARY_PATH=$(JAVA_HOME)/jre/lib/$(LINUX_ADD)server/
JAVA_INCLUDES=-I$(JAVA_HOME)/include/ -I$(JAVA_HOME)/include/$(JAVA_OS)/ -L$(JAVA_LIBRARY_PATH)

CXXFLAGS=$(JAVA_INCLUDES) -I$(SOURCES_DIR)  -g -fPIC -lstdc++ -shared
LFLAGS = -Wall -lpthread -shared  -lstdc++  -fPIC

CXXFLAGS += -ffunction-sections -fdata-sections
LFLAGS += -ffunction-sections

ifeq ($(shell uname -s),Darwin)
LFLAGS += -Wl,-dead_strip
else
LFLAGS += -Wl,--gc-sections
endif
 

LIBNAME=libtestutf8.$(EXT)

JVTMI_LIBNAME=unsafe_jvmti.$(EXT)

all: check $(LIBNAME) $(JVTMI_LIBNAME)

check:
	echo $(JAVA_HOME)
	uname -a
	uname -s 

target/jvmti.o: jvmti.cc
	@echo $< 
	mkdir -p target
	$(CC) $(CXXFLAGS) -c jvmti.cc -o target/jvmti.o

unsafe_jvmti: $(JVTMI_LIBNAME)

$(JVTMI_LIBNAME): target/jvmti.o
	-mkdir -p $(LIBS_DIR)
	$(CC) $(LFLAGS)  target/jvmti.o -shared -o $(LIBS_DIR)/$@

$(LIBNAME): $(LIB_OBJECTS)
	$(CC) $(LFLAGS) $(LIB_OBJECTS) -shared -o $(LIBS_DIR)/$@

$(OBJECTS_DIR)/%.o: $(SOURCES_DIR)/%.cpp
	@echo $< 
	mkdir -p $(OBJECTS_DIR)
	$(CC) $(CXXFLAGS) -c $< -o $@

clean:
	-rm -rf *.$(EXT) $(OBJECTS_DIR)
