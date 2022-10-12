# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/moho/Documents/testbase/ei_sample/build/edge_impulse/src/edge_impulse_project"
  "/home/moho/Documents/testbase/ei_sample/build/edge_impulse/src/edge_impulse_project-build"
  "/home/moho/Documents/testbase/ei_sample/build/edge_impulse"
  "/home/moho/Documents/testbase/ei_sample/build/edge_impulse/tmp"
  "/home/moho/Documents/testbase/ei_sample/build/edge_impulse/src/edge_impulse_project-stamp"
  "/home/moho/Documents/testbase/ei_sample/build/edge_impulse/src"
  "/home/moho/Documents/testbase/ei_sample/build/edge_impulse/src/edge_impulse_project-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/moho/Documents/testbase/ei_sample/build/edge_impulse/src/edge_impulse_project-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/moho/Documents/testbase/ei_sample/build/edge_impulse/src/edge_impulse_project-stamp${cfgdir}") # cfgdir has leading slash
endif()
