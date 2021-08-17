nRF5340dk: Beehavior Monitoring THIS IS A COPY, MUST BE REWRITTEN INTO 9160 SPECIFIC README
#########################

.. contents::
   :local:
   :depth: 2

The Beehavior Monitoring is a real-time configurable ultra-low power capable application for the nRF9160dk/Thingy:91 used in the
Summer Project for 2021.

This module is based on multiple nrf-samples, and the asset_tracker_v2 application, customized and merged to fit the project.

* Multi-central feature - The nRF9160dk acts as a central to up to 20 peripheral sensors, each sensor corresponding to one hive.
* Battery state notifications - Sends updates of the battery percentage. WIP! Sends an alert when the battery is about to die.
* Ultra-low power by design - WIP! The goal of the application is to design a greedy BLE transmission algorithm and preprocess sensor data before transmitting it to the cloud module highlights.
* Batching of data - WIP! Data can be batched to reduce the number of messages transmitted, and to be able to retain collected data while the device is offline.
* Configurable at run time - WIP! The application behavior (for example, accelerometer sensitivity or GPS timeout) can be configured at run time. This improves the development experience with individual devices or when debugging the device behavior in specific areas and situations. It also reduces the cost for transmitting data to the devices by reducing the frequency of sending firmware updates to the devices.

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
the nRF91 unit received the data. Aswell all data are sent with a NAME to differentiate messages from different peripherals/hives.

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
+---------------+-------------------------------+----------------+------------------------+
| Temperature   | TEMP  | Temperature in Celsius| 2 bytes        | [Celsius]              |
+---------------+-------------------------------+----------------+------------------------+
| Weight Right* | WT-R  | Weight on right side  | 2 bytes        | [Kg]                   |
+---------------+-------------------------------+----------------+------------------------+
| Weight Left*  | WT-L  | Weight on left side   | 2 bytes        | [Kg]                   |
+---------------+-------+-----------------------+----------------+------------------------+
*Not sent to cloud to save power and data.

Bee Counter
------------------
+---------------+-------+-----------------------+----------------+------------------------+
| Data          | ID    | Description           | Data size      | [Unit]                 |
+===============+=======+=======================+================+========================+
| Bees Out      | OUT   | Flux of bees out      | 2 bytes        | [Bees/min]             |
+---------------+-------------------------------+----------------+------------------------+
| Bees In       | IN    | Flux of bees in       | 2 bytes        | [Bees/min]             |
+---------------+-------+-----------------------+----------------+------------------------+

Data buffers (WIP! Remains to be done.)
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
| WIP! Error                       |  all 4 LEDs blinking    |
+----------------------------------+-------------------------+


Requirements
************

The application supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: thingy91_nrf9160ns@1.0.0, nrf9160dk_nrf9160ns@1.0.0

.. include:: /includes/spm.txt

Configuration
*************

The application has a Kconfig file with options that are specific to the the 5340dk part of Beehavior Monitoring, where each of the modules has a separate submenu.
These options can be used to enable and disable modules and modify their behavior and properties.


Setup
=====
Config
------
Maybe write something here? 

Setting up the nrf9160dk/Thingy:91 part of Beehavior Monitoring
--------------------------------------------------

To set up the nrf9160dk/Thingy:91 part of the Beehavior Monitoring application to work, see the following steps:
* Make sure your Thingy:91/nRF9160dk is connected to your cloud account, see :ref:`asset_tracker_v2` for how to connect.
* Set number of connections in prj.conf (CONFIG_BT_MAX_CONN).
* Flash the :ref:`hci_lpuart` example to the thingy91_nrf52840@1.0.0/nrf9160dk_nrf52840@1.0.0 chip.
* Build and flash to the thingy91_nrf9160ns@1.0.0/nrf9160dk_nrf9160ns@1.0.0


Configuration options
=====================
CONFIG_BT_MAX_CONN


Mandatory library configuration
===============================

CONFIG_BT_MAX_CONN


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
* :file:`WIP! TO DO. overlay-low-power.conf` - Configuration file that achieves the lowest power consumption by disabling features  that consume extra power like LED control and logging.
* :file:`WIP! TO DO. overlay-debug.conf` - Configuration file that adds additional verbose logging capabilities to the application

Generally, Kconfig overlays have an ``overlay-`` prefix and a ``.conf`` extension.
Board-specific configuration files are placed in the :file:`boards` folder and are named as :file:`<BOARD>.conf`.
DTS overlay files are named the same as the build target and use the file extension ``.overlay``.
When the DTS overlay filename matches the build target, the overlay is automatically chosen and applied by the build system.

Building and running
********************

Before building and running the firmware ensure that the 9160 unit is connected to your nRF Cloud account.

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

#. Observe in the terminal window that LTE connection is established, indicated by the following output::

     <inf> event_manager: MODEM_EVT_LTE_CONNECTING
     ...
     <inf> event_manager: MODEM_EVT_LTE_CONNECTED

#. Observe that the device establishes connection to the cloud::

    <inf> event_manager: CLOUD_EVT_CONNECTING
    ...
    <inf> event_manager: CLOUD_EVT_CONNECTED

#. Observe that data is sampled periodically and sent to the cloud::

    <inf> event_manager: APP_EVT_DATA_GET_ALL
    <inf> event_manager: APP_EVT_DATA_GET - Requested data types (MOD_DYN, BAT, ENV, GNSS)
    <inf> event_manager: GPS_EVT_ACTIVE
    <inf> event_manager: SENSOR_EVT_ENVIRONMENTAL_NOT_SUPPORTED
    <inf> event_manager: MODEM_EVT_MODEM_DYNAMIC_DATA_READY
    <inf> event_manager: MODEM_EVT_BATTERY_DATA_READY
    <inf> event_manager: GPS_EVT_DATA_READY
    <inf> event_manager: DATA_EVT_DATA_READY
    <inf> event_manager: GPS_EVT_INACTIVE
    <inf> event_manager: DATA_EVT_DATA_SEND
    <wrn> data_module: No batch data to encode, ringbuffers empty
    <inf> event_manager: CLOUD_EVT_DATA_ACK

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

There are probably mane issues and limitations, but this will take some time to write. The following text is kept as a template for writing this one. NOTE!! Sample text for asset_tracker_v2 is kept as example.

Following are the current limitations in the nRF Cloud implementation of the Asset Tracker v2:

* Data that is sampled by the device must ideally be addressed to the cloud-side device state and published in a single packet for regular device updates.
  This is to avoid the unnecessary stack overhead associated with splitting the payload and the additional current consumption that might result from splitting and sending the data as separate packets.
  However, in the case of nRF Cloud implementation, the nRF Cloud front end supports only the display of APP_DATA_MODEM_DYNAMIC (networkInfo) and APP_DATA_MODEM_STATIC (deviceInfo) through the device shadow.
  The other supported data types (GPS, temperature, and humidity) must be sent in a specified format to a separate message endpoint for the front end to graphically represent the data.
  You can find the JSON protocol definitions for data sent to the message endpoint in `nRF Cloud JSON protocol schemas`_.

* The nRF Cloud web application does not support the manipulation of real-time configurations.
  However, this is possible by using the REST API calls described in `nRF Cloud Patch Device State`_.
  To manipulate the device configuration, the ``desired`` section of the device state must be populated with the desired configuration of the device.
  The following schema sets the various device configuration parameters to their default values:

   .. parsed-literal::
      :class: highlight

	{
		"desired":{
			"config":{
				"activeMode":true,
				"activeWaitTime":120,
				"movementTimeout":3600,
				"movementResolution":120,
				"gpsTimeout":60,
				"movementThreshold":10
			}
		}
	}

* nRF Cloud does not support a separate endpoint for *batch* data updates. To temporarily circumvent this, batched data updates are sent to the message endpoint.


Dependencies
************
WIP TODO! Must be customized to fit this unit
This application uses the following |NCS| libraries and drivers:

* :ref:`event_manager`

In addition, it uses the following sample:

* :ref:`secure_partition_manager`


Thread usage
============

WIP TODO! Need to write the final iteration with either/and/or threads and workqueue.

In addition to system threads, some modules have dedicated threads to process messages.
Some modules receive messages that may potentially take an extended amount of time to process.
Such messages are not suitable to be processed directly in the event handler callbacks that run on the system queue.
Therefore, these modules have a dedicated thread to process the messages.

Application-specific threads:

* Main thread (app module)
* Data management module
* Cloud module
* Sensor module
* Modem module

Modules that do not have dedicated threads process events in the context of system work queue in the event manager callback.
Therefore, their workloads must be light and non-blocking.

All module threads have the following identical properties by default:

* Thread is initialized at boot.
* Thread is preemptive.
* Priority is set to the lowest application priority in the system, which is done by setting ``CONFIG_NUM_PREEMPT_PRIORITIES`` to ``1``.
* Thread is started instantly after it is initialized in the boot sequence.

Following is the basic structure for all the threads:

.. code-block:: c

   static void module_thread_fn(void)
   {
           struct module_specific msg;

           self.thread_id = k_current_get();
           module_start(&self);

           /* Module specific setup */

           state_set(STATE_DISCONNECTED);

           while (true) {
                   module_get_next_msg(&self, &msg);

                   switch (state) {
                   case STATE_DISCONNECTED:
                           on_state_disconnected(&msg);
                           break;
                   case STATE_CONNECTED:
                           on_state_connected(&msg);
                           break;
                   default:
                           LOG_WRN("Unknown state");
                           break;
                   }

                   on_all_states(&msg);
           }
   }

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