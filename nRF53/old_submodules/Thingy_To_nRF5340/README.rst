.. _central_uart:

Bluetooth: Thingy_To_nRF5340dk
#######################

.. contents::
   :local:
   :depth: 2

This module illustrates a BLE connection between a Thingy:52 and a nRF5340dk as a central. 


Overview
********

The central discovers and connects to multiple, selected services, and subscribes to them. Whenever the Thingy:52 recieves a new measurement, it sends a notification to the central, which in turn recieves the data and logs it to a terminal. The module is based on callbacks for each service discovery, and the gatt-table are set up in an iterative manner. This is due to bt_gatt_dm_start(.) which requires that the discovery of one service is complete before the next one starts. The next service discovery is called inside the prior discovery function, after the release has happened.

The module still requires some partition into modules, and a merge with the code for BM-W scanning and nrf53_to_91_to_clod.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf5340dk_nrf5340_cpuapp_and_cpuappns, nrf52840dk_nrf52840, nrf52dk_nrf52832, nrf52833dk_nrf52833, nrf52833dk_nrf52820
