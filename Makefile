MESHOPTDIR?=${HOME}/dev/EXT/zeux/meshoptimizer

LTO=-flto=auto -fuse-linker-plugin
OPT=-O3 -fomit-frame-pointer -funroll-loops -fstrict-aliasing -march=native -mtune=native
WARNFLAGS=-Wall -Wextra -Wshadow -Wstrict-aliasing -Wcast-qual -Wcast-align -Wpointer-arith -Wredundant-decls -Wfloat-equal -Wswitch-enum
MISCFLAGS=-fvisibility=hidden -fstack-protector
DEVFLAGS=-ggdb -DDEBUG -D_FORTIFY_SOURCE=3 -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function

CXXFLAGS=-std=gnu++20 -fno-rtti $(OPT) $(LTO) $(WARNFLAGS) $(ARCHFLAGS) $(MISCFLAGS)

YELLOW='\033[1;33m'
NC='\033[0m'

INCS:=-I$(MESHOPTDIR)/src

MESHOPTOBJDIR:=build
MESHOPTSRCS:=$(wildcard $(MESHOPTDIR)/src/*.cpp)
MESHOPTOBJS:=$(addprefix $(MESHOPTOBJDIR)/,$(notdir $(MESHOPTSRCS:%.cpp=%.o)))
MESHOPTLIB:= $(MESHOPTOBJDIR)/libmeshoptimizer.a
#MESHOPTLIB:=${MESHOPTDIR}/build/libmeshoptimizer.a

.PHONY: clean

all: maze2mesh

maze2mesh: maze2mesh.cpp $(MESHOPTLIB)
	$(CXX) $< $(CXXFLAGS) $(INCS) -o $@ -lm $(filter %.a %.o, $^)

$(MESHOPTOBJDIR)/%.o: $(MESHOPTDIR)/src/%.cpp
	$(CXX) -c $(CXXFLAGS) -Wno-float-equal -o $@ $<

$(MESHOPTOBJS): | $(MESHOPTOBJDIR)

$(MESHOPTOBJDIR):
	@mkdir -p $@

$(MESHOPTLIB): $(MESHOPTOBJS)
	@ar rs $@ $^

clean:
	@echo -e $(YELLOW)Cleaning$(NC)
	rm -f maze2mesh
	rm -rf $(MESHOPTOBJDIR)
