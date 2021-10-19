# module name
MODULE = dtest

################################################################################

CXX = g++
CPPFLAGS = -Werror -Wall -Winline -Wpedantic
CXXFLAGS = -std=c++11 -march=native -fPIC -fopenmp -pthread

LDFLAGS = -Wl,-E -Wl,-export-dynamic
DEPFLAGS = -MM

BUILD_DIR = build/$(shell uname -s)-$(shell uname -m)

SOURCES = $(wildcard *.dtest.cpp)
OBJ_FILES = $(SOURCES:%.dtest.cpp=$(BUILD_DIR)/%.dtest.so)
STATIC_OBJ_FILES = $(SOURCES:%.dtest.cpp=$(BUILD_DIR)/static/%.dtest.o)

.PHONY : all clean clean-dep

all : $(OBJ_FILES) $(STATIC_OBJ_FILES)

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

.dep $(BUILD_DIR) $(BUILD_DIR)/static :
	@echo "MKDIR     $(MODULE)/test/$@/"
	@mkdir -p $@

# test

.dep/%.d : %.cpp | .dep
	@echo "DEP       $(MODULE)/test/$@"
	@set -e; rm -f $@; \
	$(CXX) $(DEPFLAGS) $(INCLUDES) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,$(BUILD_DIR)/\1.so $@ : ,g' < $@.$$$$ > $@; \
	sed 's,\($*\)\.o[ :]*,$(BUILD_DIR)/static/\1.o $@ : ,g' < $@.$$$$ >> $@; \
	rm -f $@.$$$$

$(BUILD_DIR)/%.dtest.so : %.dtest.cpp | $(BUILD_DIR)
	@echo "CXX       $(MODULE)/test/$@"
	@$(CXX) -shared $(CPPFLAGS) $(CXXFLAGS) $(EXTRACXXFLAGS) -I../include $< -o $@

$(BUILD_DIR)/static/%.dtest.o : %.dtest.cpp | $(BUILD_DIR)/static
	@echo "CXX       $(MODULE)/test/static/$@"
	@$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $(EXTRACXXFLAGS) -I../include $< -o $@
