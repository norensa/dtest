# module name
MODULE = dtest

# dtest dir
DTEST = ..

################################################################################

BUILD_DIR = build/$(shell uname -s)-$(shell uname -m)

INCLUDES = -I$(DTEST)/include

LIB_DIRS =
LIBS = 

CXX = g++
CPPFLAGS = -Werror -Wall -Winline -Wpedantic
CXXFLAGS = -std=c++11 -march=native -fPIC -fopenmp

LDFLAGS = -Wl,-E -Wl,-export-dynamic
DEPFLAGS = -MM

SOURCES = $(wildcard *.cpp)
OBJ_FILES = $(SOURCES:%.cpp=$(BUILD_DIR)/%.so)

.PHONY : all clean clean-dep

all : $(OBJ_FILES)

ifndef nodep
include $(SOURCES:%.cpp=.dep/%.d)
else
ifneq ($(nodep), true)
include $(SOURCES:%.cpp=.dep/%.d)
endif
endif

# cleanup

clean :
	@rm -rf build
	@echo "Cleaned $(MODULE)/test/build/"

clean-dep :
	@rm -rf .dep
	@echo "Cleaned $(MODULE)/test/.dep/"

# dirs

.dep $(BUILD_DIR):
	@echo "MKDIR     $(MODULE)/test/$@/"
	@mkdir -p $@

# test

.dep/%.d : %.cpp | .dep
	@echo "DEP       $(MODULE)/test/$@"
	@set -e; rm -f $@; \
	$(CXX) $(DEPFLAGS) $(INCLUDES) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,$(BUILD_DIR)/\1.so $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

$(BUILD_DIR)/%.so : %.cpp | $(BUILD_DIR)
	@echo "CXX       $(MODULE)/test/$@"
	@$(CXX) -shared $(CPPFLAGS) $(CXXFLAGS) $(EXTRACXXFLAGS) $(INCLUDES) $< -o $@
