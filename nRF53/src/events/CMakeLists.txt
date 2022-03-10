#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_sources(app PRIVATE
		   ${CMAKE_CURRENT_SOURCE_DIR}/thingy_event.c
<<<<<<< HEAD
<<<<<<< HEAD
=======
		   #${CMAKE_CURRENT_SOURCE_DIR}/thingy_matrix_event.c
>>>>>>> remotes/origin/kim
=======
		#    ${CMAKE_CURRENT_SOURCE_DIR}/thingy_matrix_event.c
>>>>>>> master
		   ${CMAKE_CURRENT_SOURCE_DIR}/bm_w_event.c
		   ${CMAKE_CURRENT_SOURCE_DIR}/ble_event.c
		   ${CMAKE_CURRENT_SOURCE_DIR}/bee_count_event.c
)
