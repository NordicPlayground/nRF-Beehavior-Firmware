#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_sources(app PRIVATE
		   ${CMAKE_CURRENT_SOURCE_DIR}/ble_event.c
		   ${CMAKE_CURRENT_SOURCE_DIR}/cloud_event.c

)
