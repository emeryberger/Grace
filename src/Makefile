SRCS = libgrace.cpp xthread.cpp xcontext.cpp
DEPS = $(SRCS) xcontext.h xpersist.h xdefines.h xglobals.h xpersist.h xplock.h xrun.h warpheap.h xlatch.h xadaptheap.h xoneheap.h xfile.h xio.h xsection.h wrapper.cpp

# CXX = icc
CXX = g++
# CXX = clang

INCLUDE_DIRS = -I. -I./heaplayers -I./heaplayers/util

MACOS_COMPILE := $(CXX) -DNDEBUG -O3 -m32 $(INCLUDE_DIRS) -shared -fPIC -g -finline-limit=20000 $(SRCS) dlmalloc.c -compatibility_version 1 -current_version 1 -dynamiclib -o libgrace.dylib -ldl -lpthread

MACOS_COMPILE_DEBUG := $(CXX) -g -m32 $(INCLUDE_DIRS) -shared -fPIC $(SRCS)  -compatibility_version 1 -current_version 1 -dynamiclib -o libgrace.dylib -ldl -lpthread

all:
	@echo "Specify the desired build target."
	@echo "  macos"
	@echo "  gcc-x86"
	@echo "  gcc-x86-debug"
	@echo "  gcc-x86-64"
	@echo "  gcc-x86-64-debug"
	@echo "  gcc-sparc"

.PHONY: gcc-x86 gcc-x86-debug gcc-x86-64 gcc-x86-64-debug gcc-sparc clean

macos: $(SRCS) $(DEPS)
	$(MACOS_COMPILE)

macos-debug: $(SRCS) $(DEPS)
	$(MACOS_COMPILE_DEBUG)

gcc-x86: $(SRCS) $(DEPS)
	$(CXX) -DNDEBUG $(SRCS) dlmalloc.c $(INCLUDE_DIRS) -shared -m32 -fPIC -o libgrace.so -ldl -lpthread -lrt -O3 -finline-limit=20000 

gcc-x86-debug: $(SRCS) $(DEPS)
	$(CXX) -m32 $(INCLUDE_DIRS) -shared -fPIC -g  $(SRCS) dlmalloc.c -o libgrace.so  -ldl -lpthread

gcc-x86-64: $(SRCS) $(DEPS)
	$(CXX) -O3 -DNDEBUG $(INCLUDE_DIRS) -shared -fPIC -g -finline-limit=20000 -c dlmalloc.c
	$(CXX) -DNDEBUG $(INCLUDE_DIRS) -shared -fPIC -g -finline-limit=20000 $(SRCS) dlmalloc.o -o libgrace.so  -ldl -lpthread


gcc-x86-64-debug: $(SRCS) $(DEPS)
	$(CXX) $(INCLUDE_DIRS) -shared -fPIC -g  $(SRCS) -o libgrace.so  -ldl -lpthread

gcc-sparc: $(SRCS) $(DEPS)
	g++ -g -m32 $(INCLUDE_DIRS) -shared -fPIC $(SRCS) -o libgrace.so  -ldl -lrt -lpthread

#gcc-sparc: $(SRCS) $(DEPS)
#	g++ -DNDEBUG -O3 -m32 $(INCLUDE_DIRS) -shared -fPIC -g -finline-limit=20000 $(SRCS) -o libgrace.so  -ldl -lpthread


clean:
	rm -f libgrace.so 

