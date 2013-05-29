SRC_DIR = source
INC_DIR = include

SRCS = $(SRC_DIR)/libgrace.cpp $(SRC_DIR)/xthread.cpp $(SRC_DIR)/xcontext.cpp
DEPS = $(SRCS) $(INC_DIR)/xcontext.h $(INC_DIR)/xpersist.h $(INC_DIR)/xdefines.h $(INC_DIR)/xglobals.h $(INC_DIR)/xpersist.h $(INC_DIR)/xplock.h $(INC_DIR)/xrun.h $(INC_DIR)/warpheap.h $(INC_DIR)/xlatch.h $(INC_DIR)/xadaptheap.h $(INC_DIR)/xoneheap.h $(INC_DIR)/xfile.h $(INC_DIR)/xio.h $(INC_DIR)/xsection.h $(SRC_DIR)/wrapper.cpp

# CXX = icc
CXX = g++
# CXX = clang

INCLUDE_DIRS = -I$(INC_DIR) -I. -I./heaplayers -I./heaplayers/util

MACOS_COMPILE := $(CXX) -DNDEBUG -O3 -m32 $(INCLUDE_DIRS) -shared -fPIC -g -finline-limit=20000 $(SRCS) $(SRC_DIR)/dlmalloc.c -compatibility_version 1 -current_version 1 -dynamiclib -o libgrace.dylib -ldl -lpthread

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
	$(CXX) -DNDEBUG $(SRCS) $(SRC_DIR)/dlmalloc.c $(INCLUDE_DIRS) -shared -m32 -fPIC -o libgrace.so -ldl -lpthread -lrt -O3 -finline-limit=20000 

gcc-x86-debug: $(SRCS) $(DEPS)
	$(CXX) -m32 $(INCLUDE_DIRS) -shared -fPIC -g  $(SRCS) $(SRC_DIR)/dlmalloc.c -o libgrace.so  -ldl -lpthread

gcc-x86-64: $(SRCS) $(DEPS)
	$(CXX) -O3 -DNDEBUG $(INCLUDE_DIRS) -shared -fPIC -g -finline-limit=20000 -c $(SRC_DIR)/dlmalloc.c
	$(CXX) -DNDEBUG $(INCLUDE_DIRS) -shared -fPIC -g -finline-limit=20000 $(SRCS) dlmalloc.o -o libgrace.so  -ldl -lpthread


gcc-x86-64-debug: $(SRCS) $(DEPS)
	$(CXX) $(INCLUDE_DIRS) -shared -fPIC -g  $(SRCS) -o libgrace.so  -ldl -lpthread

gcc-sparc: $(SRCS) $(DEPS)
	g++ -g -m32 $(INCLUDE_DIRS) -shared -fPIC $(SRCS) -o libgrace.so  -ldl -lrt -lpthread

#gcc-sparc: $(SRCS) $(DEPS)
#	g++ -DNDEBUG -O3 -m32 $(INCLUDE_DIRS) -shared -fPIC -g -finline-limit=20000 $(SRCS) -o libgrace.so  -ldl -lpthread


clean:
	rm -f libgrace.so 

