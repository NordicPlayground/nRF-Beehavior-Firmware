#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_include_directories(app PRIVATE .)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/ble_module.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/cloud_module.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/fota_support.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/modules_common.c)
target_sources_ifdef(CONFIG_SMS_WARNING_ENABLED app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/sms_module.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/nvs_module.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/wdt_module.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/sms_module.c)
target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/https_client_module.c)
# target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/post_module.c)
