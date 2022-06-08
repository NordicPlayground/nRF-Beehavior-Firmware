# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/moho/ncs/bootloader/mcuboot/boot/zephyr"
  "/home/moho/Documents/egrhoefhoef/get_time/build/mcuboot"
  "/home/moho/Documents/egrhoefhoef/get_time/build/modules/mcuboot/mcuboot_subimage-prefix"
  "/home/moho/Documents/egrhoefhoef/get_time/build/modules/mcuboot/mcuboot_subimage-prefix/tmp"
  "/home/moho/Documents/egrhoefhoef/get_time/build/modules/mcuboot/mcuboot_subimage-prefix/src/mcuboot_subimage-stamp"
  "/home/moho/Documents/egrhoefhoef/get_time/build/modules/mcuboot/mcuboot_subimage-prefix/src"
  "/home/moho/Documents/egrhoefhoef/get_time/build/modules/mcuboot/mcuboot_subimage-prefix/src/mcuboot_subimage-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/moho/Documents/egrhoefhoef/get_time/build/modules/mcuboot/mcuboot_subimage-prefix/src/mcuboot_subimage-stamp/${subDir}")
endforeach()
