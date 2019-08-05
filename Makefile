# Usage:
#   make # by default it will generate test which supports three kinds of db
#
# If make test to support shannon_db only, we disable leveldb and rocksdb:
#   make LEVELDB_FLAG= ROCKSDB_FLAG=

CFLAGS += -Wall
LIBS += -lpthread -lstdc++
SHANNONDB_FLAG += -lshannon_db -DSHANNONDB
LEVELDB_FLAG += -lleveldb -lsnappy -DLEVELDB
ROCKSDB_FLAG += -lrocksdb -lsnappy -DROCKSDB
test: main.c kv_wrapper.c util.c
	gcc main.c kv_wrapper.c util.c ${CFLAGS} ${ROCKSDB_FLAG} ${LEVELDB_FLAG} ${SHANNONDB_FLAG} ${LIBS} -o test
clean:
	rm -rf test

