#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.13.1)
set(spm_CONF_FILE ${CMAKE_CURRENT_SOURCE_DIR}/spm.conf)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(ble_to_cloud_from_scratch)

# NORDIC SDK APP START
target_sources(app PRIVATE src/main.c)
# NORDIC SDK APP END

# Include source directory
target_include_directories(app PRIVATE src)

add_subdirectory(src/events)
add_subdirectory(src/modules)

