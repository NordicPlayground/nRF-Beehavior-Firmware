# Install script for directory: C:/ncs/zephyr

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files (x86)/Zephyr-Kernel")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "TRUE")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("C:/ncs/nRF-Beehavior-Firmware/TEST/build_1/zephyr/arch/cmake_install.cmake")
  include("C:/ncs/nRF-Beehavior-Firmware/TEST/build_1/zephyr/lib/cmake_install.cmake")
  include("C:/ncs/nRF-Beehavior-Firmware/TEST/build_1/zephyr/soc/arm/cmake_install.cmake")
  include("C:/ncs/nRF-Beehavior-Firmware/TEST/build_1/zephyr/boards/cmake_install.cmake")
  include("C:/ncs/nRF-Beehavior-Firmware/TEST/build_1/zephyr/subsys/cmake_install.cmake")
  include("C:/ncs/nRF-Beehavior-Firmware/TEST/build_1/zephyr/drivers/cmake_install.cmake")
  include("C:/ncs/nRF-Beehavior-Firmware/TEST/build_1/modules/nrf/cmake_install.cmake")
  include("C:/ncs/nRF-Beehavior-Firmware/TEST/build_1/modules/mcuboot/cmake_install.cmake")
  include("C:/ncs/nRF-Beehavior-Firmware/TEST/build_1/modules/nrfxlib/cmake_install.cmake")
  include("C:/ncs/nRF-Beehavior-Firmware/TEST/build_1/modules/hal_nordic/cmake_install.cmake")
  include("C:/ncs/nRF-Beehavior-Firmware/TEST/build_1/modules/tfm/cmake_install.cmake")
  include("C:/ncs/nRF-Beehavior-Firmware/TEST/build_1/modules/cddl-gen/cmake_install.cmake")
  include("C:/ncs/nRF-Beehavior-Firmware/TEST/build_1/modules/cmsis/cmake_install.cmake")
  include("C:/ncs/nRF-Beehavior-Firmware/TEST/build_1/modules/canopennode/cmake_install.cmake")
  include("C:/ncs/nRF-Beehavior-Firmware/TEST/build_1/modules/civetweb/cmake_install.cmake")
  include("C:/ncs/nRF-Beehavior-Firmware/TEST/build_1/modules/fatfs/cmake_install.cmake")
  include("C:/ncs/nRF-Beehavior-Firmware/TEST/build_1/modules/st/cmake_install.cmake")
  include("C:/ncs/nRF-Beehavior-Firmware/TEST/build_1/modules/libmetal/cmake_install.cmake")
  include("C:/ncs/nRF-Beehavior-Firmware/TEST/build_1/modules/lvgl/cmake_install.cmake")
  include("C:/ncs/nRF-Beehavior-Firmware/TEST/build_1/modules/mbedtls/cmake_install.cmake")
  include("C:/ncs/nRF-Beehavior-Firmware/TEST/build_1/modules/mcumgr/cmake_install.cmake")
  include("C:/ncs/nRF-Beehavior-Firmware/TEST/build_1/modules/open-amp/cmake_install.cmake")
  include("C:/ncs/nRF-Beehavior-Firmware/TEST/build_1/modules/loramac-node/cmake_install.cmake")
  include("C:/ncs/nRF-Beehavior-Firmware/TEST/build_1/modules/openthread/cmake_install.cmake")
  include("C:/ncs/nRF-Beehavior-Firmware/TEST/build_1/modules/segger/cmake_install.cmake")
  include("C:/ncs/nRF-Beehavior-Firmware/TEST/build_1/modules/tinycbor/cmake_install.cmake")
  include("C:/ncs/nRF-Beehavior-Firmware/TEST/build_1/modules/tinycrypt/cmake_install.cmake")
  include("C:/ncs/nRF-Beehavior-Firmware/TEST/build_1/modules/littlefs/cmake_install.cmake")
  include("C:/ncs/nRF-Beehavior-Firmware/TEST/build_1/modules/mipi-sys-t/cmake_install.cmake")
  include("C:/ncs/nRF-Beehavior-Firmware/TEST/build_1/modules/nrf_hw_models/cmake_install.cmake")
  include("C:/ncs/nRF-Beehavior-Firmware/TEST/build_1/modules/connectedhomeip/cmake_install.cmake")
  include("C:/ncs/nRF-Beehavior-Firmware/TEST/build_1/zephyr/kernel/cmake_install.cmake")
  include("C:/ncs/nRF-Beehavior-Firmware/TEST/build_1/zephyr/cmake/flash/cmake_install.cmake")
  include("C:/ncs/nRF-Beehavior-Firmware/TEST/build_1/zephyr/cmake/usage/cmake_install.cmake")
  include("C:/ncs/nRF-Beehavior-Firmware/TEST/build_1/zephyr/cmake/reports/cmake_install.cmake")

endif()

