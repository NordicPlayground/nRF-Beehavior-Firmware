#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ble_to_cloud)

# NORDIC SDK APP START
target_sources(app PRIVATE src/main.c)
# NORDIC SDK APP END

# Include source directory
target_include_directories(app PRIVATE src)

add_subdirectory(src/events)
add_subdirectory(src/modules)
#add_subdirectory(src/include)
#add_subdirectory(src/cert)

