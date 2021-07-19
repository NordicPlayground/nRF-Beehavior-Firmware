.. _dual_uart:

Bluetooth: Concurrent peripheral and central UART
#######################

.. contents::
   :local:
   :depth: 2

This module is a multi-role unit meant to be the link between external sensors and the nRF Cloud unit. 
The code is a combination of nrf/samples/bluetooth/central_uart, NordicMatt's Multi-NUS and zephyr/samples/bluetooth/scan_adv. 
central_uart was used as a basis, and the functionality of 20 concurrent connections was added to the sample. 
The code for switching between scanning and advertising was formed from scan_adv.

Behavior
********
Device should advertise in exchange with scanning until connected to a central unit. 
While connected to a central unit, the device should not advertise.

Device should scan for peripherals until maximum peripheral connections is reached. 
While connected to the maximum connections allowed, the device should not scan. 

Data is received from peripherals and processing module. Data received from peripherals is sent to processing module.
Data received from processing module is sent to BLE->Cloud.



Requirements
************

The sample supports the following development kits:
+--------------+-----------------------------+
|Board         |Build target                 |
+==============+=============================+
|nRF5340-DK    |nrf5340dk_nrf5340_cpuapp     |
|              |nrf5340dk_nrf5340_cpuappns   |
+--------------+-----------------------------+
|nRF52840-DK   |nrf52840dk_nrf52840          |
+--------------+-----------------------------+

External BLE sensors, compatible with this module. 

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`nus_client_readme`
* :ref:`gatt_dm_readme`
* :ref:`nrf_bt_scan_readme`

In addition, it uses the following Zephyr libraries:

* ``include/zephyr/types.h``
* ``boards/arm/nrf*/board.h``
* :ref:`zephyr:kernel_api`:

  * ``include/kernel.h``

* :ref:`zephyr:api_peripherals`:

   * ``include/uart.h``

* :ref:`zephyr:bluetooth_api`:

  * ``include/bluetooth/bluetooth.h``
  * ``include/bluetooth/gatt.h``
  * ``include/bluetooth/hci.h``
  * ``include/bluetooth/uuid.h``
