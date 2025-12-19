#!/bin/bash

$CXX $CFLAGS $CXXFLAGS \
     -I $SRC/fast_float/include \
     -c $SRC/fast_float/fuzz/from_chars.cc -o from_chars.o

$CXX $CFLAGS $CXXFLAGS $LIB_FUZZING_ENGINE from_chars.o \
     -o $OUT/from_chars

# Build unit tests
cmake -DFASTFLOAT_TEST=ON -DCMAKE_EXE_LINKER_FLAGS="-lpthread"
make
