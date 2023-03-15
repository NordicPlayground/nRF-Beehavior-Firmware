#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_include_directories(app PRIVATE .)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/modules_common.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/scan_module.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/peripheral_module.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/thingy_module.c)
target_sources_ifdef(CONFIG_WOODPECKER_ENABLE app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/woodpecker_module.c)
target_sources_ifdef(CONFIG_THINGY_ENABLE app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/thingy_module.c)
target_sources_ifdef(CONFIG_BEE_COUNTER_ENABLE app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/bee_counter_module.c)
target_sources_ifdef(CONFIG_WATCHDOG_ENABLE app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/nvs_module.c)
target_sources_ifdef(CONFIG_WATCHDOG_ENABLE app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/wdt_module.c)