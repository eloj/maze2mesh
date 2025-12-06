MESHOPTDIR?=${HOME}/dev/EXT/zeux/meshoptimizer

OPT=-O3 -fomit-frame-pointer -funroll-loops -fstrict-aliasing -march=native -mtune=native
WARNFLAGS=-Wall -Wextra -Wshadow -Wstrict-aliasing -Wcast-qual -Wcast-align -Wpointer-arith -Wredundant-decls -Wfloat-equal -Wswitch-enum
MISCFLAGS=-fvisibility=hidden -fstack-protector
DEVFLAGS=-ggdb -DDEBUG -D_FORTIFY_SOURCE=3 -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function

CXXFLAGS=-std=gnu++20 -fno-rtti $(OPT) $(WARNFLAGS) $(ARCHFLAGS) $(MISCFLAGS)

YELLOW='\033[1;33m'
NC='\033[0m'

LIBS:=-ldl -lpthread -lm
INCS:=-I$(MESHOPTDIR)/src

MESHOPTOBJDIR:=build
MESHOPTSRCS:=$(wildcard $(MESHOPTDIR)/src/*.cpp)
MESHOPTOBJS:=$(patsubst %,$(MESHOPTOBJDIR)/%,$(notdir $(MESHOPTSRCS:%.cpp=%.o)))
MESHOPTLIB:= $(MESHOPTOBJDIR)/meshoptimizer.a

.PHONY: clean test

maze2mesh: maze2mesh.cpp $(MESHOPTLIB)
	$(CXX) $< $(CXXFLAGS) $(LIBS) $(INCS) -o $@ $(filter %.a, $^)

$(MESHOPTOBJDIR)/%.o: $(MESHOPTDIR)/src/%.cpp | meshoptimizer.dir
	$(CXX) -c $(CXXFLAGS) -Wno-float-equal -o $@ $<

meshoptimizer.dir:
	@mkdir -p $(MESHOPTOBJDIR)

$(MESHOPTLIB): $(MESHOPTOBJS)
	@ar rs $@ $^

clean:
	@echo -e $(YELLOW)Cleaning$(NC)
	rm -f maze2mesh
	rm -rf $(MESHOPTOBJDIR)
