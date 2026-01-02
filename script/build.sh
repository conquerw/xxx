#!/bin/bash

set -e

FIRMWARE_PATH="../firmware"

build_third_party()
{
	echo "start build_third_party ..."
	
	mkdir third_party
	tar -zxvf $FIRMWARE_PATH/cJSON-1.7.18.tar.gz -C third_party
	tar -zxvf $FIRMWARE_PATH/mosquitto-2.0.18.tar.gz -C third_party
	tar -zxvf $FIRMWARE_PATH/sqlite-autoconf-3460100.tar.gz -C third_party
	tar -zxvf $FIRMWARE_PATH/zlog-1.2.18.tar.gz -C third_party
	
	cd third_party/cJSON-1.7.18
	mkdir build && cd build && cmake -DCMAKE_INSTALL_PREFIX=../install .. && make && make install
	
	cd ../../mosquitto-2.0.18
	sed -i '30s/ON/OFF/g' CMakeLists.txt && sed -i '99s/ON/OFF/g' CMakeLists.txt && mkdir build && cd build && cmake -DCMAKE_INSTALL_PREFIX=../install .. && make && make install
	
	# cd ../../sqlite-autoconf-3460100
	# ./configure --host=arm-linux CC=arm-openwrt-linux-gcc --prefix=$TOPDIR/conquerw/third_party/sqlite-autoconf-3460100/install && make && make install
	
	cd ../../zlog-1.2.18
	mkdir build && cd build && cmake -DCMAKE_INSTALL_PREFIX=../install .. && make && make install
	
	cd ../../../
}

build_app()
{
	echo "start build_app ..."

	mkdir build && cd build && cmake -DCOMPILE_TYPE=local -DRELEASE_TYPE=debug -DCMAKE_INSTALL_PREFIX=../install .. && make

	cd ../../
}

build_third_party
build_app
