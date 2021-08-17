BUILD_DIR = build/$(shell uname -s)-$(shell uname -m)

INCLUDES = -Iinclude

LIB_DIRS =
LIBS = -ldl -lpthread

CXX = g++
CPPFLAGS = -Werror -Wall -Winline -Wpedantic
CXXFLAGS = -std=c++11 -march=native

LDFLAGS = -Wl,-E -Wl,-export-dynamic
DEPFLAGS = -MM

SOURCES = $(wildcard src/*.cpp)
OBJ_FILES = $(SOURCES:src/%.cpp=$(BUILD_DIR)/%.o)

BIN_DIR := bin/$(shell uname -s)-$(shell uname -m)

.PHONY : all test clean clean-dep

all : dtest test

test :
	@$(MAKE) -C test --no-print-directory EXTRACXXFLAGS="$(EXTRACXXFLAGS)" nodep="$(nodep)"

ifndef nodep
include $(SOURCES:src/%.cpp=.dep/%.d)
else
ifneq ($(nodep), true)
include $(SOURCES:src/%.cpp=.dep/%.d)
endif
endif

# cleanup

clean :
	@rm -rf dtest bin build
	@echo "Cleaned dtest/"
	@echo "Cleaned dtest/bin/"
	@echo "Cleaned dtest/build/"
	@$(MAKE) -C test --no-print-directory clean nodep="$(nodep)"

clean-dep :
	@rm -rf .dep
	@echo "Cleaned dtest/.dep/"
	@$(MAKE) -C test --no-print-directory clean-dep nodep="$(nodep)"

# dirs

.dep $(BUILD_DIR) $(BIN_DIR):
	@echo "MKDIR     dtest/$@/"
	@mkdir -p $@

# core

dtest: $(BIN_DIR)/dtest
	@echo "LN        dtest/dtest"
	@ln -sf $(BIN_DIR)/dtest dtest

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
