#!/usr/bin/env bash

curl -R -O http://www.lua.org/ftp/lua-5.3.4.tar.gz
tar zxf lua-5.3.4.tar.gz
cd lua-5.3.4
sed -i -e 's/INSTALL_TOP=.*$/INSTALL_TOP=\/usr/g' Makefile
make linux test
sudo make install
make pc | sudo dd of=/usr/lib/pkgconfig/lua53.pc
sudo sh -c 'cat << EOF >> /usr/lib/pkgconfig/lua53.pc
Name: lua53
Version: 5.3.4
Description: Lua runtime
EOF
'
