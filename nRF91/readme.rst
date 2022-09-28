nRF9160dk/Thingy:91: Beehavior Monitoring
#########################

.. contents::
   :local:
   :depth: 2

The Beehavior Monitoring is a real-time configurable ultra-low power capable application for the nRF9160dk/Thingy:91 used in the
Summer Project for 2021 and 2022. This project was developed with nRF connect version v2.0.0.
NOTE: As of now Thingy:91 is not supported for this version due to the lte_ble_gateway sample it is based on does not work in version 2.0.0 of NCS.

This module is based on multiple nrf-samples, and the lte_ble_gateway sample, customized and merged to fit the project.

* Multi-central feature - The nRF9160dk acts as a central to up to 20 peripheral units, each unit corresponding to one hive.
* Battery state notifications - Sends updates of the battery percentage, only available on the Thingy:91. WIP! Sends an alert when the battery is about to die.
* Ultra-low power by design - The application changes the default connection interval over Bluetooth to save power, aswell as turning off the LTE while it is not in use. 
* Batching of data - Data can be batched to reduce the number of messages transmitted, and to be able to retain collected data while the device is offline.
* Firmware Over The Air - Untested! The application supports FOTA updates.

Implementation of the above features required a rework of existing nrf samples and applications. Most noteworthy are the app_event_manager, lte_ble_gateway and central_uart samples.

.. note::
    The code is currently a work in progress and is not fully optimized yet. It will undergo changes and improvements in the future.

Overview
********

The application initializes the modem and retrives the current time from the modem, before scanning for the peripheral units by UUID. 
When connecting to a peripheral it sends the peripheral a unique id and stores its name. That way the nRF9160 knows which unit it receives data from.
The data received from the units is then sent to nRF Cloud with an AppID, the name of the hive/unit and a timestamp.
The user can send commands to the 9160dk/Thingy:91 through the terminal in nRF Cloud.
NOTE: This requires that the nRF9160 is connected to nRF Cloud, so this feature should be changed to use SMS.

+---------------+-----------------------------------------------+
| Commands      | Description                                   |
+===============+===============================================+
| StartScan     | Starts scanning for peripheral units          |
+---------------+-----------------------------------------------+
| BLE_status    | Returns number of connected peripheral units  |
+---------------+-----------------------------------------------+

Firmware architecture
=====================

The nRF9160dk/Thingy:91 part of Smarthive: Beehavior Monitoring has a modular structure, where each module has a defined scope of responsibility.
The application makes use of the :ref:`app_event_manager` to distribute events between modules in the system.
The event manager is used for all the communication between the modules.
The final messages sent to the Cloud module of the project is taken from BLE data messages which supports up to 20 bytes.


Data types
==========

Data from multiple peripherals are collected to construct information about the weight, battery and environment. 
The application supports the following sensor types:

+-----------------------+-------------------------------------------------------+---------------+
| Sensor type           | Description                                           | App ID        |
+=======================+=======================================================+===============+
| BroodMinder Weight    | Weight and Temperature                                | BM-W          |
+-----------------------+-------------------------------------------------------+---------------+
| Thingy:52             | Temperature, Humidity, Air Pressure and battery       | THINGY        |
+-----------------------+-------------------------------------------------------+---------------+
| BeeCounter            | Bees in and out of hive                               | BEE-CNT       |
+-----------------------+-------------------------------------------------------+---------------+
| WoodPecker            | Likelihood of woodpecker, positive and total triggers | W-PECK        |
+-----------------------+-------------------------------------------------------+---------------+
The sets of sensor data that are published to the cloud service consist of relative `timestamps <Timestamping_>`_ that originate from the time
the nRF91 unit received the data. All data are sent with a NAME to differentiate messages from different peripherals/hives.

The sensor data used in the system so far can be seen in the following tables:

Thingy:52
---------
+-----------------------+-------+-----------------------+----------------+------------------------+
| Data                  | ID    | Description           | Data size      | [Unit]                 |
+=======================+=======+=======================+================+========================+
| Air pressure          | AIR   | Air pressure in hPa   | 5 bytes        | [hPa]                  |
+-----------------------+-------+-----------------------+----------------+------------------------+
| Temperature           | TEMP  | Temperature in Celsius| 2 bytes        | [Celsius]              |
+-----------------------+-------+-----------------------+----------------+------------------------+
| Relative humidity     | HUMID | Relative humidity in %| 1 byte         | [%]                    |
+-----------------------+-------+-----------------------+----------------+------------------------+
| Battery charge        | BTRY  | Battery charge in %   | 1 byte         | [%]                    |
+-----------------------+-------+-----------------------+----------------+------------------------+        

BroodMinder Weight
------------------
+---------------+-------+-----------------------+----------------+------------------------+
| Data          | ID    | Description           | Data size      | [Unit]                 |
+===============+=======+=======================+================+========================+
| Weight        | RTW   | Weight in Kg          | 2 bytes        | [Kg]                   |
+---------------+-------+-----------------------+----------------+------------------------+
| Temperature   | TEMP  | Temperature in Celsius| 2 bytes        | [Celsius]              |
+---------------+-------+-----------------------+----------------+------------------------+
| Weight Right* | WT-R  | Weight on right side  | 2 bytes        | [Kg]                   |
+---------------+-------+-----------------------+----------------+------------------------+
| Weight Left*  | WT-L  | Weight on left side   | 2 bytes        | [Kg]                   |
+---------------+-------+-----------------------+----------------+------------------------+
*Not sent to cloud to save power and data.

Bee Counter
------------------
+---------------+-------+-----------------------+----------------+------------------------+
| Data          | ID    | Description           | Data size      | [Unit]                 |
+===============+=======+=======================+================+========================+
| Bees Out      | OUT   | Flux of bees out      | 2 bytes        | [Bees/min]             |
+---------------+-------+-----------------------+----------------+------------------------+
| Bees In       | IN    | Flux of bees in       | 2 bytes        | [Bees/min]             |
+---------------+-------+-----------------------+----------------+------------------------+

WoodPecker
------------------
+---------------+---------------+-------------------------------------------------------------------+-----------+-----------+
| Data          | ID            | Description                                                       | Data size | [Unit]    |
+===============+===============+===================================================+===========+===========+
| Total         | TOTAL         | Amount of times noise have exceeded the threshold                 | 2 bytes   | Unitless  |
+---------------+---------------+-------------------------------------------------------------------+-----------+-----------+
| Postive       | POSITIVE      | Amount of times likelihood of woodpecker exceeded the threshold   | 1 byte    | Unitless  |
+---------------+---------------+-------------------------------------------------------------------+-----------+-----------+
| Probability   | PROBABILITY   | Highest probability of woodpecker                                 | 1 bytes   | [%]       |
+---------------+---------------+-------------------------------------------------------------------+-----------+-----------+
| Battery       | BTRY          | Remaining battery percentage of the Thingy:53                     | 1 bytes   | [%]       |
+---------------+---------------+-------------------------------------------------------------------+-----------+-----------+

User interface
**************
The application displays LED behavior that corresponds to the task performed by the application.

The following table shows the LED behavior demonstrated by the application:

+----------------------------------+-------------------------+--------------------------+
| State                            | nRF9160dk               | Thingy:91                |
+==================================+=========================+==========================+
| Connected to Cloud               | LED1 on                 | Blue                     |
+----------------------------------+-------------------------+--------------------------+
| Connected to nRF53/peripheral    | LED2 on                 | Green                    |
+----------------------------------+-------------------------+--------------------------+
| WIP! Error                       |  all 4 LEDs blinking    | Red                      |
+----------------------------------+-------------------------+--------------------------+


Requirements
************

The application supports the following development kits:
NOTE: Thingy:91 does not work after v2.0.0.

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: thingy91_nrf9160ns@1.0.0, nrf9160dk_nrf9160ns@1.0.0

.. include:: /includes/spm.txt

Setup
=====

Setting up the nrf9160dk/Thingy:91 part of Beehavior Monitoring
--------------------------------------------------

To set up the nrf9160dk/Thingy:91 part of the Beehavior Monitoring application to work, see the following steps:

* Make sure your Thingy:91/nRF9160dk is connected to your cloud account, see :ref:`asset_tracker_v2` for how to connect.
* Set number of connections in prj.conf (CONFIG_BT_MAX_CONN).
* Flash the :ref:`hci_lpuart` example to the thingy91_nrf52840@1.0.0/nrf9160dk_nrf52840@1.0.0 chip.
* Build and flash to the thingy91_nrf9160ns@1.0.0/nrf9160dk_nrf9160ns@1.0.0

Configuration options
=====================
All configurations should be set in prj.conf.

CONFIG_BT_MAX_CONN=<Number of hives>
Amount of hives you want to connect to should be set, to avoid scanning for new devices when connected to all available devices.

CONFIG_MAX_OUTGOING_MESSAGES
Amount of messages that are sent at once can be configured with:
If you want the device to send messages as soon as they arrive, set CONFIG_MAX_OUTGOING_MESSAGES to 1.

CONFIG_SMS_WARNING_ENABLED
Set to y if you want to get SMS warnings when the hive reaches alarming values.

CONFIG_WARNING_DELAY_HOURS
Set to amount of hours between each warning. 

Configuration files
===================

The application provides predefined configuration files for the summer project in 2022.
You can find the configuration files in the :file:`nRF-Beehavior-Firmware/` directory.

It is possible to build the application with overlay files for both DTS and Kconfig to override the default values for the board.
The application contains examples of Kconfig overlays.

The following configuration files are available in the application folder:

* :file:`prj.conf` - Configuration file common for all build targets
* :file:`boards/thingy91_nrf9160ns.conf` - Configuration file specific for Thingy:91. The file is automatically merged with :file:`prj.conf` when you build for the ``thingy91_nrf9160ns@1.0.0`` build target. This board might need some further work.
* :file:`overlay-low-power.conf` - Configuration file that achieves the lowest power consumption by disabling features  that consume extra power like LED control and logging.
* :file:`WIP! TO DO. overlay-debug.conf` - Configuration file that adds additional verbose logging capabilities to the application

Generally, Kconfig overlays have an ``overlay-`` prefix and a ``.conf`` extension.
Board-specific configuration files are placed in the :file:`boards` folder and are named as :file:`<BOARD>.conf`.
DTS overlay files are named the same as the build target and use the file extension ``.overlay``.
When the DTS overlay filename matches the build target, the overlay is automatically chosen and applied by the build system.

Logging
=====================
To enable logging build without the overlay-low-power.conf and comment out 
/* Disable uart0 to conserve power, when logging is not needed */
&uart0 {
	status = "disabled";
};

in the <board>.overlay.

Building and running
********************

Before building and running the firmware ensure that the nRF9160dk/Thingy:91 is connected to your nRF Cloud account.

Building with overlays
======================

To build with Kconfig overlay, it must be based to the build system, as shown in the following example:

``west build -b thingy91_nrf9160ns@1.0.0 \ nrf9160dk_nrf9160ns@1.0.0 -- -DOVERLAY_CONFIG=overlay-low-power.conf``. Note that this overlay is not yet created.

The above command will build for Thingy:91 \ nRF9160 DK using the configurations found in :file:`overlay-low-power.conf`, in addition to the configurations found in :file:`nrf9160dk_nrf9160ns.conf`.
If some options are defined in both files, the options set in the overlay take precedence.

Testing
=======

After programming the application and all the prerequisites to your development kit, test the application by performing the following steps:

1. |connect_kit|
#. Connect to the kit with a terminal emulator (for example, LTE Link Monitor, Termite or RTT viewer if you are using a Thingy:91).
#. Reset the development kit.
#. Observe in the terminal window that the development kit starts up in the Secure Partition Manager and that the application starts.
   This is indicated by several <inf> module_name: function_name(): "placeholder text" outputs.

      *** Booting Zephyr OS build v2.4.0-ncs1-2616-g3420cde0e37b  ***
      <inf> event_manager: APP_EVT_START

#. Observe in the terminal window that LTE connection is established, indicated by the following output::

     <inf> event_manager: MODEM_EVT_LTE_CONNECTING
     ...
     <inf> event_manager: MODEM_EVT_LTE_CONNECTED

#. Observe that the device establishes connection to the peripherals::

    <inf> ble_module: Address <address>, name <Hive_Name> added as number <number>

#. Observe that data is sampled periodically and sent to the cloud::

    <inf> cloud_module: Message formatted: {"appID":"Thingy""TEMP":"23.48""HUMID":"44""AIR":"1017.3""BTRY":"58""TIME":"1630395932""NAME":"Hive1"}, length: 102 


Known issues and limitations
****************************

Following are the current limitations in the nRF9160dk/Thingy:91 code:

* Due to a firmware issue the 9160 and the 52840 chips have to start at the same time so the program initially crashes and reboots. After this reboot the program works as itended. 

* As of v2.0.0 of NCS the communication between the 52840 and 9160 chip does not work on the Thingy:91. Therefore you must use a nRF9160 DevKit. 

* As of now this project only supports nRF CLoud. If you want to expand the project to other Cloud services you can use the nRF9160/ aws or azure examples as a guide.

* The Thingy:91/nRF9160dk need to send a message to nRF Cloud every 5 minutes or you need to manually reconnect to Cloud to send new messages. It should be possible to ping nRF Cloud to keep the connection alive, but as of now it is not implemented in this project.

* Due to sleeping the LTE between messages, it is harder/not posssible to send messages from the Cloud to the nRF9160. This can be fixed with SMS, but won't be instant because of the LTE sleeping.

Dependencies
************
This application uses the following |NCS| libraries and drivers:

* :ref:`app_event_manager`

* :ref:`nordic_uart_service`

* :ref:`modem`

* :ref:`date_time`

In addition, it is based on the following samples:

* :ref:`app_event_manager`

* :ref:`bluetooth/peripheral_uart`

* :ref:`nrf9160/lte_ble_gateway`

* :ref:`nrf9160/sms`