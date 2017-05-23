#!/usr/bin/env bash

curl -R -O http://www.lua.org/ftp/lua-5.3.4.tar.gz
tar zxf lua-5.3.4.tar.gz
cd lua-5.3.4
sed -i -e 's/INSTALL_TOP=.*$/INSTALL_TOP=\/usr/g' Makefile
make linux test
sudo make install
