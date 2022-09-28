#
# Copyright (c) 2019-2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_sources(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/battery.c)
# target_sources_ifdef(CONFIG_LED_USE_PWM app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/led_pwm.c)
