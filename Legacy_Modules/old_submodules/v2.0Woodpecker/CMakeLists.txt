#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)
#added by Nora


find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project("Edge Impulse wrapper sample")

set(PM_STATIC_YML_FILE
  ${CMAKE_CURRENT_SOURCE_DIR}/boards/pm_static_${BOARD}.yml
  )

# NORDIC SDK APP START
target_include_directories(app PRIVATE src/include)
target_sources(app PRIVATE src/main.c)
# NORDIC SDK APP END

# Preinitialization related to Thingy:53 DFU
target_sources_ifdef(CONFIG_BOARD_THINGY53_NRF5340_CPUAPP app PRIVATE
  boards/thingy53.c
)
target_sources_ifdef(CONFIG_BOARD_THINGY53_NRF5340_CPUAPP_NS app PRIVATE
  boards/thingy53.c
)

add_subdirectory(src/modules)