c++ -w -Wno-return-type -Wno-undefined-bool-conversion -Wno-potentially-evaluated-expression -fPIC   -o external_lib.o -c external_lib.cpp
c++ -std=c++11 -O3 -dynamiclib -Wl,-headerpad_max_install_names  -o external_lib.so external_lib.o
