#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_include_directories(app PRIVATE .)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/modules_common.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/bm_w_module.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/thingy_module.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ble_module.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/peripheral_module.c)
