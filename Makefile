CCPP = g++
CCPP_FLAGS = -std=c++14 -Wfatal-errors -Wall

OUTPUT_DIR = bin
INCLUDE_DIRS = -Iinclude

LIB_DEPS = -lpthread

all: dirs test_spmv

dirs:
	mkdir -p ${OUTPUT_DIR}

test_spmv: test/test_spmv.cpp 
	${CCPP} ${CCPP_FLAGS} ${INCLUDE_DIRS} -o ${OUTPUT_DIR}/$@ $< ${LIB_DEPS}

docs:
	doxygen docs/Doxyfile
