SHELL := /bin/sh

# Toolchain (override from environment if needed)
CXX ?= g++
AR ?= ar
RM ?= rm -f
MKDIR_P ?= mkdir -p

# Build options for old gmake/gcc environments
CPPFLAGS := -DRAYTRACING_ENABLED -DMGF_ENABLED
CXXFLAGS ?= -O2 -ffast-math -Wall
LDFLAGS ?=
LDLIBS ?= -lm

ROOT_DIR := .
BASE_DIR := $(ROOT_DIR)/base/src
APP_DIR := $(ROOT_DIR)/testsuite/ApplicationCases/RenderparkApplication/src

OBJ_DIR := $(ROOT_DIR)/build
BIN_DIR := $(ROOT_DIR)/testsuite/ApplicationCases/RenderparkApplication/build
BIN := $(BIN_DIR)/rpk
BIN_ALIAS := $(BIN_DIR)/rpm

BASE_SRCS := $(shell find $(BASE_DIR) -type f -name '*.cpp')
APP_SRCS := $(shell find $(APP_DIR) -type f -name '*.cpp')
SRCS := $(BASE_SRCS) $(APP_SRCS)

OBJS := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(SRCS))

INCLUDES := -I$(BASE_DIR) -I$(APP_DIR)

.PHONY: all clean dirs

all: $(BIN) $(BIN_ALIAS)

dirs:
	@$(MKDIR_P) $(OBJ_DIR)
	@$(MKDIR_P) $(BIN_DIR)

$(BIN): dirs $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

$(BIN_ALIAS): $(BIN)
	@cp $(BIN) $(BIN_ALIAS)

$(OBJ_DIR)/%.o: %.cpp
	@$(MKDIR_P) $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	$(RM) -r $(OBJ_DIR)
	$(RM) $(BIN) $(BIN_ALIAS)
