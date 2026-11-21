SHELL := /bin/sh

# Toolchain (override from environment if needed)
CXX ?= g++
AR ?= ar
RM ?= rm -f
MKDIR_P ?= mkdir -p

# Build options for old gmake/gcc environments
CPPFLAGS := -DRAYTRACING_ENABLED -DMGF_ENABLED -D_FILE_OFFSET_BITS=64
# RAY_CAPTURE=1 builds the ray capture configuration (CMake WITH_RAY_CAPTURE)
ifeq ($(RAY_CAPTURE),1)
CPPFLAGS += -DRPK_RAY_CAPTURE
endif
CXXFLAGS ?= -O0 -g -Wall -fno-exceptions -fno-rtti
LDFLAGS ?=
LDLIBS ?= -lm

ROOT_DIR := .
BASE_DIR := $(ROOT_DIR)/base/src/main
APP_DIR := $(ROOT_DIR)/testsuite/ApplicationCases/RenderparkApplication/src

OBJ_DIR := $(ROOT_DIR)/build
BIN_DIR := $(ROOT_DIR)/testsuite/ApplicationCases/RenderparkApplication/build
BIN := $(BIN_DIR)/rpk
BIN_ALIAS := $(BIN_DIR)/rpm
REPLAY_DIR := $(ROOT_DIR)/testsuite/ApplicationCases/RayCorpusReplay/src
REPLAY_BIN_DIR := $(ROOT_DIR)/testsuite/ApplicationCases/RayCorpusReplay/build
REPLAY_BIN := $(REPLAY_BIN_DIR)/rayCorpusReplay

BASE_SRCS := $(shell find $(BASE_DIR) -type f -name '*.cpp')
APP_SRCS := $(shell find $(APP_DIR) -type f -name '*.cpp')
SRCS := $(BASE_SRCS) $(APP_SRCS)
REPLAY_SRCS := $(shell find $(REPLAY_DIR) -type f -name '*.cpp')
REPLAY_APP_SRCS := $(filter-out $(APP_DIR)/app/Main.cpp,$(APP_SRCS))

OBJS := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(SRCS))
REPLAY_OBJS := $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(BASE_SRCS) $(REPLAY_APP_SRCS) $(REPLAY_SRCS))

INCLUDES := -I$(BASE_DIR) -I$(APP_DIR) -I$(REPLAY_DIR)

.PHONY: all clean dirs replay

all: $(BIN) $(BIN_ALIAS)

replay: $(REPLAY_BIN)

dirs:
	@$(MKDIR_P) $(OBJ_DIR)
	@$(MKDIR_P) $(BIN_DIR)

$(BIN): dirs $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

$(REPLAY_BIN): dirs $(REPLAY_OBJS)
	@$(MKDIR_P) $(REPLAY_BIN_DIR)
	$(CXX) $(LDFLAGS) -o $@ $(REPLAY_OBJS) $(LDLIBS)

$(BIN_ALIAS): $(BIN)
	@cp $(BIN) $(BIN_ALIAS)

$(OBJ_DIR)/%.o: %.cpp
	@$(MKDIR_P) $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	$(RM) -r $(OBJ_DIR)
	$(RM) $(BIN) $(BIN_ALIAS) $(REPLAY_BIN)
