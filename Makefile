BUILD_DIR = build/$(shell uname -s)-$(shell uname -m)

INCLUDES = -Iinclude

LIB_DIRS =
LIBS = -ldl

CXX = g++
CPPFLAGS = -Werror -Wall -Winline -Wpedantic
CXXFLAGS = -march=native -pthread

LDFLAGS = -Wl,-E -Wl,-export-dynamic
DEPFLAGS = -MM

SOURCES = $(wildcard src/*.cpp)
OBJ_FILES = $(SOURCES:src/%.cpp=$(BUILD_DIR)/%.o)
OBJ_FILES_CXX11 = $(SOURCES:src/%.cpp=$(BUILD_DIR)/cxx11/%.o)
OBJ_FILES_CXX14 = $(SOURCES:src/%.cpp=$(BUILD_DIR)/cxx14/%.o)
OBJ_FILES_CXX17 = $(SOURCES:src/%.cpp=$(BUILD_DIR)/cxx17/%.o)

BIN_DIR := bin/$(shell uname -s)-$(shell uname -m)

.PHONY : all build-test-only test clean clean-dep

all : dtest

build-test-only:
	@$(MAKE) -C test --no-print-directory EXTRACXXFLAGS="$(EXTRACXXFLAGS)" nodep="$(nodep)"

test : dtest build-test-only
	@./dtest

ifndef nodep
include $(SOURCES:src/%.cpp=.dep/%.d)
else
ifneq ($(nodep), true)
include $(SOURCES:src/%.cpp=.dep/%.d)
endif
endif

# cleanup

clean :
	@rm -rf dtest* bin build
	@echo "Cleaned dtest/"
	@echo "Cleaned dtest/bin/"
	@echo "Cleaned dtest/build/"
	@$(MAKE) -C test --no-print-directory clean nodep="$(nodep)"

clean-dep :
	@rm -rf .dep
	@echo "Cleaned dtest/.dep/"
	@$(MAKE) -C test --no-print-directory clean-dep nodep="$(nodep)"

# dirs

.dep $(BUILD_DIR) $(BUILD_DIR)/cxx11 $(BUILD_DIR)/cxx14 $(BUILD_DIR)/cxx17 $(BIN_DIR):
	@echo "MKDIR     dtest/$@/"
	@mkdir -p $@

# core

dtest: $(BIN_DIR)/dtest
	@echo "LN        dtest/$@"
	@ln -sf $(BIN_DIR)/$@ $@

dtest-%: $(BIN_DIR)/dtest-%
	@echo "LN        dtest/$@"
	@ln -sf $(BIN_DIR)/$@ $@

$(BIN_DIR)/dtest: $(OBJ_FILES) | $(BIN_DIR)
	@echo "LD        dtest/$@"
	@$(CXX) $(CXXFLAGS) $(EXTRACXXFLAGS) $(LDFLAGS) $(LIB_DIRS) $(OBJ_FILES) $(LIBS) -o $@

$(BIN_DIR)/dtest-cxx11: $(OBJ_FILES_CXX11) | $(BIN_DIR)
	@echo "LD        dtest/$@"
	@$(CXX) -std=c++11 $(CXXFLAGS) $(EXTRACXXFLAGS) $(LDFLAGS) $(LIB_DIRS) $(OBJ_FILES_CXX11) $(LIBS) -o $@

$(BIN_DIR)/dtest-cxx14: $(OBJ_FILES_CXX14) | $(BIN_DIR)
	@echo "LD        dtest/$@"
	@$(CXX) -std=c++14 $(CXXFLAGS) $(EXTRACXXFLAGS) $(LDFLAGS) $(LIB_DIRS) $(OBJ_FILES_CXX14) $(LIBS) -o $@

$(BIN_DIR)/dtest-cxx17: $(OBJ_FILES_CXX17) | $(BIN_DIR)
	@echo "LD        dtest/$@"
	@$(CXX) -std=c++17 $(CXXFLAGS) $(EXTRACXXFLAGS) $(LDFLAGS) $(LIB_DIRS) $(OBJ_FILES_CXX17) $(LIBS) -o $@

.dep/%.d : src/%.cpp | .dep
	@echo "DEP       dtest/$@"
	@set -e; rm -f $@; \
	$(CXX) $(DEPFLAGS) $(INCLUDES) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,$(BUILD_DIR)/\1.o $@ : ,g' < $@.$$$$ > $@; \
	sed 's,\($*\)\.o[ :]*,$(BUILD_DIR)/cxx11/\1.o $@ : ,g' < $@.$$$$ >> $@; \
	sed 's,\($*\)\.o[ :]*,$(BUILD_DIR)/cxx14/\1.o $@ : ,g' < $@.$$$$ >> $@; \
	sed 's,\($*\)\.o[ :]*,$(BUILD_DIR)/cxx17/\1.o $@ : ,g' < $@.$$$$ >> $@; \
	rm -f $@.$$$$

$(BUILD_DIR)/%.o : src/%.cpp | $(BUILD_DIR)
	@echo "CXX       dtest/$@"
	@$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $(EXTRACXXFLAGS) $(INCLUDES) $< -o $@

$(BUILD_DIR)/cxx11/%.o : src/%.cpp | $(BUILD_DIR)/cxx11
	@echo "CXX       dtest/$@"
	@$(CXX) -c $(CPPFLAGS) -std=c++11 $(CXXFLAGS) $(EXTRACXXFLAGS) $(INCLUDES) $< -o $@

$(BUILD_DIR)/cxx14/%.o : src/%.cpp | $(BUILD_DIR)/cxx14
	@echo "CXX       dtest/$@"
	@$(CXX) -c $(CPPFLAGS) -std=c++14 $(CXXFLAGS) $(EXTRACXXFLAGS) $(INCLUDES) $< -o $@

$(BUILD_DIR)/cxx17/%.o : src/%.cpp | $(BUILD_DIR)/cxx17
	@echo "CXX       dtest/$@"
	@$(CXX) -c $(CPPFLAGS) -std=c++17 $(CXXFLAGS) $(EXTRACXXFLAGS) $(INCLUDES) $< -o $@
