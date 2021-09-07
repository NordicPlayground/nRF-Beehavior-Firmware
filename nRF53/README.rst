nRF5340dk: Beehavior Monitoring
#########################

.. contents::
   :local:
   :depth: 2

The Beehavior Monitoring is an application for the nRF5340dk used in the Summer Project for 2021.

This module is based on multiple nrf-samples,  and the asset_tracker_v2 application, customized and merged to fit the project.

* Multi-role feature - The nRF5340dk acts as a central to the peripheral sensors, and as a peripheral to the Cloud-unit with communication going both ways.
* Modular build - The application is based on the event_manager library and can be expanded to include other sensors.
* Ultra-low power by design - WIP! The goal of the application is to design a greedy BLE transmission algorithm and preprocess sensor data before transmitting it to the cloud module highlights.
* Batching of data - WIP! Data can be batched to reduce the number of messages transmitted, and to be able to retain collected data while the device is offline.
* Configurable at run time - WIP! The application behavior (for example, accelerometer or sensitivity) can be configured at run time. This improves the development experience with individual devices or when debugging the device behavior in specific areas and situations. It also reduces the cost for transmitting data to the devices by reducing the frequency of sending firmware updates to the devices.

Implementation of the above features required a rework of existing nrf samples and applications. Most noteworthy are the peripheral_uart and central_uart samples.

.. note: ::
    The code is currently a work in progress and is not fully optimized yet. It will undergo changes and improvements in the future.

Overview
********

The application initializes BLE and scan modules before scanning and connecting to up to 19 other devices.
For now, the module scans for a Thingy:52 over NAME and connects with it and subscribes to the Thingy:52 environmental and motion services. 
When connected, the nRF5340dk writes to the Thingy:52 to change the sample time and to toggle the lights off to reduce light pollution inside the hive.
The nRF5340dk also scans for advertisements from a BroodMinder Weight, and reads weight measurements from the data message advertised. 
This module can easily be scaled to include other BroodMinder products.
The nRF5340dk also connects to a cloud module (nRF9160dk/Thingy:91), and broadcasts sensordata and/or preprocessed data over NUS to the cloud module.

Firmware architecture
=====================

The nRF5340dk part of Smarthive: Beehavior Monitoring has a modular structure, where each module has a defined scope of responsibility.
The application makes use of the :ref:`event_manager` to distribute events between modules in the system.
The event manager is used for all the communication between the modules.
The final messages sent to the Cloud module of the project is arranged into a BLE data message which supports up to 20 bytes.


Data types and sampling rate
============================

Data from multiple sensor sources are collected to construct information about the orientation, battery life, and environment.
The application supports the following data types:

+---------------+-------------------------------------------+
| Data type     | Description                               |
+===============+===========================================+
| Environmental | Temperature, humidity and air pressure    |
+---------------+-------------------------------------------+
| Movement      | Orientation                               |
+---------------+-------------------------------------------+
| Battery       | Remaining power                           |
+---------------+-------------------------------------------+

WIP! The sets of sensor data that are published to the cloud service consist of relative `timestamps <Timestamping_>`_ that originate from the time of sampling.
WIP! The data sampling should be concatenated in a buffer matrix containing a finite number of measurements, f.ex 15 samples. 
The elements in the buffer matrix contains the 15 prior measurements with a timestamp.
If an event such as swarm event is triggered, all the measurements in the buffer are sent to increase the resolution of the graphs and figures.
Otherwise, send every 15th sample to the cloud unit. 

This is motivated by a possible energy conservation from transmitting data, and that the dynamics of the system is rather slow and does not require a 1/60 Hz resolution.

The sensor data used in the system so far can be seen in the following table:

+-------------------------------------+-----------------------+----------------+------------------------+
| Sensor data from peripheral sensors | Description           | Data size      | [Unit]                 |
+=====================================+=======================+================+========================+
| T:52 - Air pressure                 | Air pressure in hPa   | 5 bytes        | [hPa]                  |
+-------------------------------------+-----------------------+----------------+------------------------+
| T:52 - Temperature                  | Temperature in Celsius| 2 bytes        | [Celsius]              |
+-------------------------------------+-----------------------+----------------+------------------------+
| T:52 - Relative humidity            | Relative humidity in %| 1 byte         | [%]                    |
+-------------------------------------+-----------------------+----------------+------------------------+
| T:52 - Battery charge               | Battery charge in %   | 1 byte         | [%]                    |
+-------------------------------------+-----------------------+----------------+------------------------+
| BM_W - Weight                       | Weight in Kg          | 2 bytes        | [Kg]                   |
+-------------------------------------+-----------------------+----------------+------------------------+
| BM_W - Temperature (external)       | Temperature in Celsius| 2 bytes        | [Celsius]              |
+-------------------------------------+-----------------------+----------------+------------------------+
| BeeCounter - Flux of bees in/out    | Flux in/out per minute| 2/2 bytes      | [Bees/min]             |
+-------------------------------------+-----------------------+----------------+------------------------+

Data buffers (TO DO!)
=======================================
Data sampled from the onboard modem and the external sensors is to be stored in a buffer, where a ring buffer is a suggested option.
This enables to store data for a while if the units become disconnected from each other, and reduce transmission rate to conserve power.

User interface
**************

The application uses non of the buttons at this point in time, except the reset button which restarts the dk. 
It is suggested that buttons can be configured to read health (battery charge) from Thingy:52 and to send a message to the cloud unit to verify if the units are connected.

Additionally, the application displays LED behavior that corresponds to the task performed by the application.
The following table shows the purpose of each supported button:


The following table shows the LED behavior demonstrated by the application:

+----------------------------------+-------------------------+
| State                            | nRF5340dk               |
+==================================+=========================+
| Connected to Thingy:52           | LED1 on                 |
+----------------------------------+-------------------------+
| Connected to nrf9160dk/Thingy:91 | LED2 on                 |
+----------------------------------+-------------------------+
| Scanning for BM_W advertisements | LED3 on                 |
+----------------------------------+-------------------------+
| TO DO! Error                     |  All 4 LEDs blinking    |
+----------------------------------+-------------------------+


Requirements
************

The application supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: thingy53_nr5340_cpuappns, nrf5340dk_nrf5340_cpuappns

.. include:: /includes/spm.txt

Configuration
*************

The application has a Kconfig file with options that are specific to the the 5340dk part of Beehavior Monitoring, where each of the modules has a separate submenu.
These options can be used to enable and disable modules and modify their behavior and properties.


Setup
=====

Setting up the 5340dk part of Beehavior Monitoring
--------------------------------------------------

To set up the nrf5340dk part of the Beehavior Monitoring application to work, see the following steps:

* Enable the peripheral sensors available for your setup.
* Change the name in the Thingy mobile application and set the same name in Kconfig as a scan parameter.
* Set the name of the nRF5340dk to the name the 91 unit will send to nRF Cloud.


Configuration options
=====================
Peripheral ensor support can be disabled in prj.conf to prevent the 5340dk to search for sensors that are not needed.

CONFIG_BROODMINDER_WEIGHT_ENABLE=n

CONFIG_BEE_COUNTER_ENABLE=n

CONFIG_THINGY_ENABLE=n

All of these ara enabled by default.

CONFIG_THINGY_NAME="Thingy123"

Set to the name of the Thingy:52 in your hive.

CONFIG_BT_DEVICE_NAME="Hive1"

Set to the name of your hive.

CONFIG_LOG_DEFAULT_LEVEL=4

Set to 0 to turn of logging.

1 = LOG_INF

2 = LOG_WRN

3 = LOG_ERR

4 = LOG_DBG


Mandatory library configuration
===============================

You can set the mandatory library-specific Kconfig options in the :file:`prj.conf` file of the application.


Optional library configurations
===============================
Needs filling?

You can add the following optional configurations to configure the heap or to provide additional information such as APN to the modem for registering with an LTE network:

* :option:`CONFIG_HEAP_MEM_POOL_SIZE` - Configures the size of the heap that is used by the application when encoding and sending data to the cloud. More information can be found in :ref:`memory_allocation`.
* :option:`CONFIG_PDN_DEFAULTS_OVERRIDE` - Used for manual configuration of the APN. Set the option to ``y`` to override the default PDP context configuration.
* :option:`CONFIG_PDN_DEFAULT_APN` - Used for manual configuration of the APN. An example is ``apn.example.com``.


Configuration files
===================

The application provides predefined configuration files for the summer project in 2021.
You can find the configuration files in the :file:`nRF-Beehavior-Firmware/` directory.

It is possible to build the application with overlay files for both DTS and Kconfig to override the default values for the board.
The application contains examples of Kconfig overlays.

The following configuration files are available in the application folder:

* :file:`prj.conf` - Configuration file common for all build targets
* :file:`boards/thingy53_nrf5340_cpuappns.conf` - Configuration file specific for Thingy:53. The file is automatically merged with :file:`prj.conf` when you build for the ``thingy53_nrf5340_cpuappns`` build target. This board might need some further work.
* :file:`TO DO. overlay-low-power.conf` - Configuration file that achieves the lowest power consumption by disabling features  that consume extra power like LED control and logging.
* :file:`TO DO. overlay-debug.conf` - Configuration file that adds additional verbose logging capabilities to the application

Generally, Kconfig overlays have an ``overlay-`` prefix and a ``.conf`` extension.
Board-specific configuration files are placed in the :file:`boards` folder and are named as :file:`<BOARD>.conf`.
DTS overlay files are named the same as the build target and use the file extension ``.overlay``.
When the DTS overlay filename matches the build target, the overlay is automatically chosen and applied by the build system.

Building and running
********************

Before building and running the firmware ensure that the Thingy:52 is set up and configured to a name recognized by the code used in the 53-unit.
In the configuration used this summer the name is set to "Hive1". If this name is to be reused, change the name of the Thingy:52 in the Thingy-app to "Hive1".

Building with overlays
======================

To build with Kconfig overlay, it must be based to the build system, as shown in the following example:

``west build -b thingy53_nrf5340_cpuappns \ nrf5340dk_nrf5340_cpuappns -- -DOVERLAY_CONFIG=overlay-low-power.conf``. Note that this overlay is not yet created.

The above command will build for Thingy:53 \ nRF5340 DK using the configurations found in :file:`overlay-low-power.conf`, in addition to the configurations found in :file:`thingy53_nrf5340_cpuappns.conf`.
If some options are defined in both files, the options set in the overlay take precedence.

Testing
=======

After programming the application and all the prerequisites to your development kit, test the application by performing the following steps:

1. |connect_kit|
#. Connect to the kit with a terminal emulator (for example, LTE Link Monitor or Termite).
#. Reset the development kit.
#. Observe in the terminal window that the development kit starts up in the Secure Partition Manager and that the application starts.
   This is indicated by several <inf> module_name: function_name(): "placeholder text" outputs, similar to the following example from asset_tracker_v2.

      *** Booting Zephyr OS build v2.4.0-ncs1-2616-g3420cde0e37b  ***
      <inf> event_manager: APP_EVT_START

#. Observe in the terminal window that Bluetooth is enabled, indicated by the following output::

     <inf> ble_module: Enabling BLE
     ...
     <inf> ble_module: BLE_READY event submitted.

#. Observe that the device establishes connection to Thingy:52 and LED1 turns on::

    <inf> central_module: Scanning for Thingy:52:
    ...
    <inf> central_module: Starting Thingy:52 service discovery chain.

#. Observe that the device establishes connection to the nRF9160dk/Thingy:91 and LED2 turn on::

    <inf> peripheral_module: bt_nus_init and bt_le_adv_start completed.
    ...
    <inf> peripheral_module: New ID is: <id>

#. Observe that data is sampled periodically and sent to the cloud::

    <inf> event_manager: Temperature [C]: %i,%i, Humidity [Percentage]: %i, Air pressure [hPa]: %d,%i, Battery charge [%%]: %i.

The order should be something like:
* Initialize BLE and scan.
* Scan for Thingy:52 and connect (LED1 ON).
* Discover services from Thingy:52.
* Subscribe to notifications for a set of services.
* Scan for Thingy:91 and connect either when notification is given OR 91 requests connection through scanning (LED2 ON).
* Scan for BroodMinder Weight advertisements and read data message (LED3 ON/OFF while doing/not doing this).
* Loop.

Known issues and limitations
****************************

There are probably many issues and limitations, but this will take some time to write. Thus, this section is incomplete.


Dependencies
************
This section might need filling.
This application uses the following |NCS| libraries and drivers:

* :ref:`event_manager`


.. _memory_allocation:

Memory allocation
=================

Mostly, the modules use statically allocated memory.
Following are some features that rely on dynamically allocated memory, using the :ref:`Zephyr heap memory pool implementation <zephyr:heap_v2>`:

* Event manager events
* Encoding of the data that will be sent to cloud

You can configure the heap memory by using the :option:`CONFIG_HEAP_MEM_POOL_SIZE`.
The data management module that encodes data destined for cloud is the biggest consumer of heap memory.
Therefore, when adjusting buffer sizes in the data management module, you must also adjust the heap accordingly.
This avoids the problem of running out of heap memory in worst-case scenarios.