# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "X:/transmission-jr/third-party/dht"
  "X:/transmission-jr/build-dbg/third-party/dht.bld/src/dht-build"
  "X:/transmission-jr/build-dbg/third-party/dht.bld/pfx"
  "X:/transmission-jr/build-dbg/third-party/dht.bld/tmp"
  "X:/transmission-jr/build-dbg/third-party/dht.bld/src/dht-stamp"
  "X:/transmission-jr/build-dbg/third-party/dht.bld/src"
  "X:/transmission-jr/build-dbg/third-party/dht.bld/src/dht-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "X:/transmission-jr/build-dbg/third-party/dht.bld/src/dht-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "X:/transmission-jr/build-dbg/third-party/dht.bld/src/dht-stamp${cfgdir}") # cfgdir has leading slash
endif()
