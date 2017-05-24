#!/usr/bin/env bash

curl -R -O http://www.lua.org/ftp/lua-5.3.4.tar.gz
tar zxf lua-5.3.4.tar.gz
cd lua-5.3.4
sed -i -e 's/INSTALL_TOP=.*$/INSTALL_TOP=\/usr/g' Makefile
make linux test
sudo make install
sudo sh -c 'cat > /usr/lib/pkgconfig/lua53.pc' << 'EOF' 
prefix=/usr
major_version=5.3
version=5.3.4
lib_name_include=lua5.3

lib_name=lua
libdir=${prefix}/lib
includedir=${prefix}/include

#
# The following are intended to be used via "pkg-config --variable".

# Install paths for Lua modules.  For example, if a package wants to install
# Lua source modules to the /usr/local tree, call pkg-config with
# "--define-variable=prefix=/usr/local" and "--variable=INSTALL_LMOD".
INSTALL_LMOD=${prefix}/share/lua/${major_version}
INSTALL_CMOD=${prefix}/lib/lua/${major_version}

Name: Lua
Description: Lua language engine
Version: ${version}
Requires:
Libs: -L${libdir} -l${lib_name} -ldl -lm
Cflags: -I${includedir}/${lib_name_include}
EOF
