
1.功能说明

LevelDB是Google开源的持久化KV单机数据库，具有很高的读/写性能
Rocksdb是facebook 的一个实验项目，是在leveldb的基础上优化而得，解决了leveldb的一些问题。
Shannondb是宝存在ssd基础上开发的一套基于键值对存储的kv存储系统 。
现在通过此测试工具，用来对比leveldb，rocksdb 和shannondb 的性能。


2.leveldb测试前须知：
	测试leveldb 首先需要安装leveldb（下载leveldb 源代码，make 编译，然后把编译的文件拷贝到相关目录下 sudo cp out-shared/libleveldb.so* /usr/local/lib & sudo cp -R include/* /usr/local/include)
	测试leveldb 性能时，为了对齐相关测试条件，我们要将leveldb 的数据是要同步到磁盘上。所以在加载驱动后，需要使用ocnvme 创建块设备，然后为块设备创建文件系统，然后进行挂载。

相关操作：
2.1:安装ssd卡, 加载ssd 对应的驱动

2.2:驱动加载成功后，创建创建块设备

2.3：为块设备创建文件系统
	mkfs.ext4 /dev/xxxx

2.4：挂载块设备到/mnt 目录下
	mount /dev/xxx    /mnt

2.5：编译当前leveldb测试文件
	make

3.shannondb测试前须知：
	测试shannondb 性能时，需要加载驱动，然后使用ocnvme kv 创建 设备。然后安装shannon_db的 c 库。

相关操作：
3.1:安装ssd卡, 加载ssd 对应的驱动(宝存对外提供deb 和 rpm 包，可直接安装)

3.2创建kv设备(宝存对外提供ocnvme 工具， 用来操作ssd 设备)
	ocnvme kv create  -d lnvm0n1 -c 1TB / -o 20

3.3加载动态库
	在shannon_db /目录下执行 make install 脚本(宝存对外提供shannondb 的c 库， 编译后直接安装)

3.4：编译当前shannondb测试文件
	make

4.rocksb 测试前须知

	测试rocksdb前首先需要安装rocksdb,(网上下载rocksdb 源代码，make 编译，编译前有些库需要安装, zlib (library for data compression)  bzip2( library for data compression)  snappy(library for fast data compression) gflags (library that handles command line flags processing)  You can compile rocksdb library even if you don't have gflags installed

	ubuntun 安装：sudo apt-get install libgflags-dev sudo apt-get install libsnappy-dev  sudo apt-get install zlib1g-dev sudo apt-get install libbz2-dev 
	centos  安装：
		Install gflags: wget https://gflags.googlecode.com/files/gflags-2.0-no-svn-files.tar.gz (网上下载源码编译)
						tar -xzvf gflags-2.0-no-svn-files.tar.gz
						cd gflags-2.0
						./configure && make && sudo make install
		Install snappy: sudo yum install snappy snappy-devel
		Install zlib:
						sudo yum install zlib
						sudo yum install zlib-devel
		Install bzip2:
				        sudo yum install bzip2
					    sudo yum install bzip2-devel

库安装好编译通过后，
	修改环境变量
	export CPLUS_INCLUDE_PATH=${CPLUS_INCLUDE_PATH}:`pwd`/include
	export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:`pwd`
	export LIBRARY_PATH=${LIBRARY_PATH}:`pwd`
	sudo cp librocksdb.so /usr/local/lib
	sudo mkdir -p /usr/local/include/rocksdb/ 
	sudo cp -r ./include/* /usr/local/include/ 

相关命令：

4.1:安装ssd卡, 加载ssd 对应的驱动

4.2:驱动加载成功后，创建创建块设备

4.3：为块设备创建文件系统
	mkfs.ext4 /dev/xxxx

4.4：挂载块设备到/mnt 目录下
	mount /dev/xxx    /mnt

4.5：编译当前rocksdb测试文件
	make


5.使用说明：
	编译执行通过后，运行可执行程序文件。

运行leveldb  随机读测试时(测试随机读时，要首先需要顺序写入一部分数据，顺序写 参数需要指定--num 即读之前需要写入的个数, 在执行随机读时，要指定随机读取最大的max_seq 范围)
	./test  --target=leveldb  --path=/mnt/leveldb  --rw=randread   --value-size=1k  --maxseq=1000000  --num-threads=1
运行leveldb  顺序读测试时
	./test  --target=leveldb  --path=/mnt/leveldb  --rw=read  --value-size=1k  --maxseq=1000000  --num-threads=1
运行leveldb  随机写测试时
	./test  --target=leveldb  --path=/mnt/leveldb  --rw=randwrite  --value-size=1k --maxseq=1000000  --num-threads=1  --num=1000000
运行leveldb  顺序写测试时
	./test  --target=leveldb  --path=/mnt/leveldb  --rw=write  --value-size=1k  --maxseq=1000000  --num-threads=1  --num=1000000

运行shannondb  随机写测试时
	./test  --target=shannon_db  --path=/dev/kvdev0   --rw=randwrite  --value-size=1k  --num-threads=1  --maxseq=100000  --num=1000000 --dbname=test.db
运行shannondb  顺序写测试时
	./test  --target=shannon_db  --path=/dev/kvdev0   --rw=write  --value-size=1k  --num-threads=1  --maxseq=100000  --num=1000000 --dbname=test.db
运行shannondb  随机读测试时
	./test  --target=shannon_db  --path=/dev/kvdev0   --rw=randread  --value-size=1k  --num-threads=1  --maxseq=100000  --num=1000000 --dbname=test.db
运行shannondb  顺序读测试时
	./test  --target=shannon_db  --path=/dev/kvdev0   --rw=read  --value-size=1k  --num-threads=1  --maxseq=100000  --num=1000000 --dbname=test.db
(组合添加--batch_count 参数　为测试批量读写,参数值为一次批量读写的个数，目前只适用swiftkv)


运行rocksdb  随机写测试时
	./test  --target=rocksdb  --path=/mnt/rocksdb   --rw=randwrite  --value-size=1k  --maxseq=100000   --num-threads=1   --num=1000000
运行rocksdb  顺序写测试时
	./test  --target=rocksdb  --path=/mnt/rocksdb   --rw=write  --value-size=1k  --maxseq=100000   --num-threads=1   --num=1000000
运行rocksdb  随机读测试时
	./test  --target=rocksdb  --path=/mnt/rocksdb   --rw=randread   --value-size=1k  --maxseq=1000000  --num-threads=1
运行rocksdb  顺序读测试时
	./test  --target=rocksdb  --path=/mnt/rocksdb   --rw=read   --value-size=1k  --maxseq=1000000  --num-threads=1
	
相关命令语法：

options    
--target  指定说明需要测试的对象  leveldb/venice

--path    指定说明需要测试对象 的路径  .
		target = leveldb 时，--path 等于磁盘挂载mount 的路径 + 需要创建leveldb  数据库 名称。
		Target = venice 时，--path == /dev/kvdev0 (加载驱动时，路径已经固定生成)


--rw       指定进行读 还是写测试 。
		--rw = read 顺序读测试
		--rw = write 顺序写测试
		--rw = randread 随机读测试
		--rw = randwrite 随机写测试

--value-size 指定value  的大小 即一次要读写的value 的值的大小，目前范围1–4k(需要加单位)

--num  需要注意的是，测试随机读时，我们第一步需要在磁盘中顺序写入一定数量的kv 值，然后再进行随机读取的测试。
		--num 用来指定测试写入的数据量。--num = 100000

--loginfo  将log输出到指定的重定向文件

--maxseq   测试随机读时，需要指定随机读的keyd的范围 即maxseq;

--num-threads  开启的线程个数

--batch_count  write_batch/read_batch 时 添加到batch中的个数（目前只适用于shannondb,不指定时，默认为直接put）。

--dbname   测试shannondb 时，需要指定对应的数据库（目前只适用于shannondb）。
