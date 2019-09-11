# Usage:
#   make # by default it will generate test which supports three kinds of db
#
# If make test to support shannon_db only, we disable leveldb and rocksdb:
#   make LEVELDB_FLAG= ROCKSDB_FLAG=

CFLAGS += -Wall
LIBS += -lpthread -lstdc++
SHANNONDB_FLAG += -lshannondb -DSHANNONDB
SHANNONDB_CXX_FLAG += -lshannondb -DSHANNONDB
LEVELDB_FLAG += -lleveldb -lsnappy -DLEVELDB
ROCKSDB_FLAG += -lrocksdb -lsnappy -DROCKSDB

all: test test_cpp

kv_wrapper_cpp.o: kv_wrapper_cpp.cpp
	g++ -c -o kv_wrapper_cpp.o kv_wrapper_cpp.cpp ${CFLAGS} ${SHANNONDB_CXX_FLAG} -fpermissive -std=c++11

test: main.c kv_wrapper.c util.c
	gcc main.c kv_wrapper.c util.c ${CFLAGS} ${ROCKSDB_FLAG} ${LEVELDB_FLAG} ${SHANNONDB_FLAG} ${LIBS} -o test

# shannon_db only
test_cpp: main.c util.c kv_wrapper_cpp.o
	gcc main.c util.c kv_wrapper_cpp.o ${CFLAGS} ${SHANNONDB_CXX_FLAG} ${LIBS} -o test_cpp
clean:
	rm -rf test test_cpp *.o

