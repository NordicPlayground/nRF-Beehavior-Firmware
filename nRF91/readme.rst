nRF9160dk/Thingy:91: Beehavior Monitoring
#########################

.. contents::
   :local:
   :depth: 2

The Beehavior Monitoring is a real-time configurable ultra-low power capable application for the nRF9160dk/Thingy:91 used in the
Summer Project for 2021. This project was developed with nRF connect version v1.6.0-rc2.

This module is based on multiple nrf-samples, and the asset_tracker_v2 application, customized and merged to fit the project.

* Multi-central feature - The nRF9160dk acts as a central to up to 20 peripheral sensors, each sensor corresponding to one hive.
* Battery state notifications - Sends updates of the battery percentage. WIP! Sends an alert when the battery is about to die.
* Ultra-low power by design - WIP! The goal of the application is to design a greedy BLE transmission algorithm and preprocess sensor data before transmitting it to the cloud module highlights.
* Batching of data - WIP! Data can be batched to reduce the number of messages transmitted, and to be able to retain collected data while the device is offline.
* Configurable at run time - WIP! The application behavior (for example, data sampling rate) can be configured at run time. This improves the development experience with individual devices or when debugging the device behavior in specific areas and situations. It also reduces the cost for transmitting data to the devices by reducing the frequency of sending firmware updates to the devices.

Implementation of the above features required a rework of existing nrf samples and applications. Most noteworthy are the event_manager, cloud_client and central_uart samples.

.. note::
    The code is currently a work in progress and is not fully optimized yet. It will undergo changes and improvements in the future.

Overview
********

The application initializes the modem and connects to nRF Cloud, before scanning for the peripheral units by UUID. 
When connecting to a peripheral it sends the peripheral an unique id and stores its name. That way the 9160 knows which unit it receives data from.
The data received from the units is then sent to nRF Cloud with an AppID, the name of the hive/unit and a timestamp.
The user can send commands to the 9160dk/Thingy:91 through the terminal in nRF Cloud.

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
The application makes use of the :ref:`event_manager` to distribute events between modules in the system.
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

User interface
**************

The application uses button number 1 and 2, plus the reset button which restarts the dk.
Button1 turns of LED's to save power.
Button2 starts scanning for peripheral units. PS. Button2 only works on the DevKit.

Additionally, the application displays LED behavior that corresponds to the task performed by the application.

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

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: thingy91_nrf9160ns@1.0.0, nrf9160dk_nrf9160ns@1.0.0

.. include:: /includes/spm.txt

Setup
=====
Config
------
Change CONFIG_BT_MAX_CONN in prj.conf to amount of nRF5340dk you want to connect to. 

Setting up the nrf9160dk/Thingy:91 part of Beehavior Monitoring
--------------------------------------------------

To set up the nrf9160dk/Thingy:91 part of the Beehavior Monitoring application to work, see the following steps:

* Make sure your Thingy:91/nRF9160dk is connected to your cloud account, see :ref:`asset_tracker_v2` for how to connect.
* Set number of connections in prj.conf (CONFIG_BT_MAX_CONN).
* Flash the :ref:`hci_lpuart` example to the thingy91_nrf52840@1.0.0/nrf9160dk_nrf52840@1.0.0 chip.
* Build and flash to the thingy91_nrf9160ns@1.0.0/nrf9160dk_nrf9160ns@1.0.0

Configuration files
===================

The application provides predefined configuration files for the summer project in 2021.
You can find the configuration files in the :file:`nRF-Beehavior-Firmware/` directory.

It is possible to build the application with overlay files for both DTS and Kconfig to override the default values for the board.
The application contains examples of Kconfig overlays.

The following configuration files are available in the application folder:

* :file:`prj.conf` - Configuration file common for all build targets
* :file:`boards/thingy91_nrf9160ns.conf` - Configuration file specific for Thingy:91. The file is automatically merged with :file:`prj.conf` when you build for the ``thingy91_nrf9160ns@1.0.0`` build target. This board might need some further work.
* :file:`WIP! TO DO. overlay-low-power.conf` - Configuration file that achieves the lowest power consumption by disabling features  that consume extra power like LED control and logging.
* :file:`WIP! TO DO. overlay-debug.conf` - Configuration file that adds additional verbose logging capabilities to the application

Generally, Kconfig overlays have an ``overlay-`` prefix and a ``.conf`` extension.
Board-specific configuration files are placed in the :file:`boards` folder and are named as :file:`<BOARD>.conf`.
DTS overlay files are named the same as the build target and use the file extension ``.overlay``.
When the DTS overlay filename matches the build target, the overlay is automatically chosen and applied by the build system.

Building and running
********************

Before building and running the firmware ensure that the nRF9160dk/Thingy:91 is connected to your nRF Cloud account.

Building with overlays
======================

To build with Kconfig overlay, it must be based to the build system, as shown in the following example:

``west build -b thingy91_nrf9160ns@1.0.0 \ nrf9160dk_nrf9160ns@1.0.0 -- -DOVERLAY_CONFIG=overlay-low-power.conf``. Note that this overlay is not yet created.

The above command will build for Thingy:91 \ nRF9160 DK using the configurations found in :file:`overlay-low-power.conf`, in addition to the configurations found in :file:`thingy53_nrf5340_cpuappns.conf`.
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

#. Observe that the device establishes connection to the cloud::

    <inf> event_manager: CLOUD_EVT_CONNECTING
    ...
    <inf> event_manager: CLOUD_EVT_CONNECTED

#. Observe that data is sampled periodically and sent to the cloud::

    <inf> cloud_module: Message formatted: {"appID":"Thingy""TEMP":"23.48""HUMID":"44""AIR":"1017.3""BTRY":"58""TIME":"1630395932""NAME":"Hive1"}, length: 102 


Known issues and limitations
****************************

Following are the current limitations in the nRF9160dk/Thingy:91 code:

* Due to a firmware issue the 9160 and the 52840 chips have to start at the same time so the program initially crashes and reboots. After this reboot the program works as itended. 

* As of now this project only supports nRF CLoud. If you want to expand the project to other Cloud services you can use the nRF9160/Cloud_Client example as a guide.

* The Thingy:91/nRF9160dk need to send a message to nRF Cloud every 5 minutes or you need to manually reconnect to Cloud to send new messages. It should be possible to ping nRF Cloud to keep the connection alive, but as of now it is not implemented in this project.

Dependencies
************
This application uses the following |NCS| libraries and drivers:

* :ref:`event_manager`

* :ref:`nordic_uart_service`

* :ref:`modem`

In addition, it uses the following sample:

* :ref:`event_manager`

* :ref:`bluetooth/peripheral_uart`

* :ref:`nrf9160/cloud_client`
