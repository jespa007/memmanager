gcc test_c.c memmgr.c -o test_c.exe -pthread -fomit-frame-pointer -std=c99 -g -O0 -pipe -Wall -Wstrict-prototypes
#g++ test_cpp.cpp memmgr.cpp -o test_cpp.exe -pthread -fomit-frame-pointer -g -O0  -std=c++11 -pipe -Wall