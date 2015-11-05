#!/bin/sh

TOP_DIR=`pwd`
INSTALL_DIR="$TOP_DIR"/install
DEST_DIR="$TOP_DIR"/../../lib

make 2>&1 | tee make.log
make install
cp -a $INSTALL_DIR/include/* $DEST_DIR/include
cp -a $INSTALL_DIR/lib/* $DEST_DIR/lib
