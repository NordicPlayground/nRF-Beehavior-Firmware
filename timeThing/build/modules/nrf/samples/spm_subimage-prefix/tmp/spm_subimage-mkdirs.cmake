# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/moho/ncs/nrf/samples/spm"
  "/home/moho/Documents/egrhoefhoef/get_time/build/spm"
  "/home/moho/Documents/egrhoefhoef/get_time/build/modules/nrf/samples/spm_subimage-prefix"
  "/home/moho/Documents/egrhoefhoef/get_time/build/modules/nrf/samples/spm_subimage-prefix/tmp"
  "/home/moho/Documents/egrhoefhoef/get_time/build/modules/nrf/samples/spm_subimage-prefix/src/spm_subimage-stamp"
  "/home/moho/Documents/egrhoefhoef/get_time/build/modules/nrf/samples/spm_subimage-prefix/src"
  "/home/moho/Documents/egrhoefhoef/get_time/build/modules/nrf/samples/spm_subimage-prefix/src/spm_subimage-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/moho/Documents/egrhoefhoef/get_time/build/modules/nrf/samples/spm_subimage-prefix/src/spm_subimage-stamp/${subDir}")
endforeach()
