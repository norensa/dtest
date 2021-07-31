BUILD_DIR = build/$(shell uname -s)-$(shell uname -m)

INCLUDES = -Iinclude

LIB_DIRS =
LIBS = -ldl

CXX = g++
CPPFLAGS = -Werror -Wall -Winline -Wpedantic
CXXFLAGS = -std=c++11 -march=native -fPIC

LDFLAGS = -Wl,-E -Wl,-export-dynamic
DEPFLAGS = -MM

SOURCES = $(wildcard src/*.cpp)
OBJ_FILES = $(SOURCES:src/%.cpp=$(BUILD_DIR)/%.o)

TEST_SOURCES = $(wildcard test/*.cpp)
TEST_OBJ_FILES = $(TEST_SOURCES:test/%.cpp=test/$(BUILD_DIR)/%.so)

ifeq ($(BIN_DIR),)
BIN_DIR := bin/$(shell uname -s)-$(shell uname -m)
endif

.PHONY : all clean clean-dep

all : dtest $(TEST_OBJ_FILES)

ifndef nodep
include $(SOURCES:src/%.cpp=.dep/%.d)
include $(TEST_SOURCES:test/%.cpp=test/.dep/%.d)
else
ifneq ($(nodep), true)
include $(SOURCES:src/%.cpp=.dep/%.d)
include $(TEST_SOURCES:test/%.cpp=test/.dep/%.d)
endif
endif

# cleanup

clean :
	@rm -rf dtest bin build test/build
	@echo "Cleaned dtest/"
	@echo "Cleaned dtest/bin/"
	@echo "Cleaned dtest/build/"
	@echo "Cleaned dtest/test/build/"

clean-dep :
	@rm -rf .dep test/.dep
	@echo "Cleaned dtest/.dep/"
	@echo "Cleaned dtest/test/.dep/"

# dirs

.dep test/.dep $(BUILD_DIR) test/$(BUILD_DIR) $(BIN_DIR):
	@echo "MKDIR     dtest/$@/"
	@mkdir -p $@

# core

dtest: $(BIN_DIR)/dtest
	@echo "LN        dtest/dtest"
	@ln -s $(BIN_DIR)/dtest dtest

$(BIN_DIR)/dtest: $(OBJ_FILES) | $(BIN_DIR)
	@echo "LD        dtest/$@"
	@$(CXX) $(CXXFLAGS) $(EXTRACXXFLAGS) $(LDFLAGS) $(LIB_DIRS) $(OBJ_FILES) $(LIBS) -o $@

.dep/%.d : src/%.cpp | .dep
	@echo "DEP       dtest/$@"
	@set -e; rm -f $@; \
	$(CXX) $(DEPFLAGS) $(INCLUDES) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,$(BUILD_DIR)/\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

$(BUILD_DIR)/%.o : src/%.cpp | $(BUILD_DIR)
	@echo "CXX       dtest/$@"
	@$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $(EXTRACXXFLAGS) $(INCLUDES) $< -o $@

# test

test/.dep/%.d : test/%.cpp | test/.dep
	@echo "DEP       dtest/$@"
	@set -e; rm -f $@; \
	$(CXX) $(DEPFLAGS) $(INCLUDES) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,test/$(BUILD_DIR)/\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

test/$(BUILD_DIR)/%.so : test/%.cpp | test/$(BUILD_DIR)
	@echo "CXX       dtest/$@"
	@$(CXX) -shared $(CPPFLAGS) $(CXXFLAGS) $(EXTRACXXFLAGS) $(INCLUDES) $< -o $@
