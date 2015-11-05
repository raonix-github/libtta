#!/bin/sh

export CC=gcc
export NM=nm
export AR=ar
export AS=as
export RANLIB=ranlib
export OBJDUMP=objdump
export STRIP=strip

PREFIX=/home/gomma/projects/libtta/install_i386

./configure \
	--host=i686-pc-linux-gnu \
	--build=i686-pc-linux-gnu \
	--prefix=$PREFIX \
	--enable-static
