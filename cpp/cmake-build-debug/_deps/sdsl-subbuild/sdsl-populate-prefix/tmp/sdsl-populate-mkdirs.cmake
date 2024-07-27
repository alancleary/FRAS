# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "F:/FRAS-Bitpacked/cpp/cmake-build-debug/_deps/sdsl-src"
  "F:/FRAS-Bitpacked/cpp/cmake-build-debug/_deps/sdsl-build"
  "F:/FRAS-Bitpacked/cpp/cmake-build-debug/_deps/sdsl-subbuild/sdsl-populate-prefix"
  "F:/FRAS-Bitpacked/cpp/cmake-build-debug/_deps/sdsl-subbuild/sdsl-populate-prefix/tmp"
  "F:/FRAS-Bitpacked/cpp/cmake-build-debug/_deps/sdsl-subbuild/sdsl-populate-prefix/src/sdsl-populate-stamp"
  "F:/FRAS-Bitpacked/cpp/cmake-build-debug/_deps/sdsl-subbuild/sdsl-populate-prefix/src"
  "F:/FRAS-Bitpacked/cpp/cmake-build-debug/_deps/sdsl-subbuild/sdsl-populate-prefix/src/sdsl-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "F:/FRAS-Bitpacked/cpp/cmake-build-debug/_deps/sdsl-subbuild/sdsl-populate-prefix/src/sdsl-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "F:/FRAS-Bitpacked/cpp/cmake-build-debug/_deps/sdsl-subbuild/sdsl-populate-prefix/src/sdsl-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
