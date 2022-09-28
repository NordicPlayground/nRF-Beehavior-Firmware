Thingy:52 GATT: Beehavior Monitoring
##########################

.. contents::
   :local:
   :depth: 2

The Thingy:52 GATT Beehavior Monitoring is an application for the Thingy:52 used in the Summer Project for 2022.

This module is based on the GATT bluetooth API to create services that the nRF5340 can subsribe to. 

* Ultra-low power by design - The goal of the application is to design a greedy BLE transmission algorithm by increasing advertising and connection intervals and turning off unneeded peripherals. Since the Thingy:52 is measuring inside the hive, it is important that it lasts long before it runs out of battery, to minimize disruptions of the hive.
* Configurable at run time - The sampling frequency of sensors and sending of data can be configured at run time through the nRF5340. This improves the development experience with individual devices or when debugging the device behavior in specific areas and situations.

Implementation of the above features required a rework of existing nRF samples and applications. Most noteworthy is the throughput sample.

.. note ::
    The code is currently a work in progress and is not fully optimized yet. It will undergo changes and improvements in the future.

Overview
********

The application initializes BLE and connection callbacks before advertising to the nRF5340. Once the nRF5340 has connected it subscribes to the Thingy:52's bee service. The bee service has a characteristic for each sampled value (temperature, humidity, etc.) and one write characteristic so the nRF5340 can configure the sending frequency of data. 
For now, the nRF5340 scans for the Thingy:52 over NAME and connects with it and then subscribes to the Thingy:52 characteristics. 
It is important that the name of the Thingy:52 (CONFIG_BT_DEVICE_NAME="ThingyName" in prj.conf) is the same as the nRF5340 scans after (CONFIG_THINGY_NAME="ThingyName" in prj.conf in the nRF53 directory).

Firmware architecture
=====================

The Thingy:52 GATT part of the Smarthive: Beehavior Monitoring is divided into two parts, one for establishing bluetooth connection and sampling the sensors and one for setting up the ADC and measuring the battery.
The application makes use of the BT_GATT_SERVICE_DEFINE to set up all the services and bt_gatt_notify() to send data to the nRF5340.
The battery sampling is based on the nRF battery sample (ncs\zephyr\samples\boards\nrf\battery).

Sensors and sampling rate
============================

Data from multiple sensor sources are collected to construct information about the battery life and the environment.
The application samples the following sensors:

+---------------+-------------------------------------------+
| Sensor        | Description                               |
+===============+===========================================+
| HTS221        | Temperature and humidity                  |
+---------------+-------------------------------------------+
| LPS22HB       | Air pressure                              |
+---------------+-------------------------------------------+
| ADC           | Battery voltage converted to percent      |
+---------------+-------------------------------------------+

The battery measurerment is turned off between sampling to reduce power.

User interface
**************

The application uses non of the buttons at this point in time. All you have to do is to turn on the Thingy:52 and put it inside the hive. It will then advertise until it makes a connection. If the nRF5340 for some reason disconnects from the Thingy:52, the Thingy:52 will start to advertise (or reboot if that fails) again until a new connection is made.

Requirements
************

The application supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: thingy52_nrf52832

.. include:: /includes/spm.txt

Configuration
*************

The only thing the user needs to configure is the name of the Thingy:52. This is done by changing the CONFIG_BT_DEVICE_NAME in prj.conf. If you have more than one hive that you want to measure inside, you will need a uniqe name for each Thingy:52 to ensure that the nRF5340's connect to the correct Thingy:52. During testing you might want to turn on the logging to check that everything works. After insuring that the nRF5340 and Thingy:52 connect and that the Thingy:52 sends data, you should turn off the logging before placing it inside your hive. This is done by building with the "overlay-low-power.conf" as described in the Building and running section.

Building and running
********************

Before building and running the firmware ensure that the Thingy:52 is set up and configured to a name recognized by the code used in the nRF5340.
In the configuration used this summer the name is set to "ThingyHive1". If this name is to be reused, change the name of the Thingy:52 in the prj.conf to "ThingyHive1".

Building with overlays
======================

To build with Kconfig overlay, it must be based to the build system, as shown in the following example:

``west build -b thingy52_nrf52832 -- -DOVERLAY_CONFIG="overlay-low-power.conf"``. 

The above command will build for Thingy:52 using the configurations found in :file:`overlay-low-power.conf`, in addition to the configurations found in :file:`prj.conf`.
If some options are defined in both files, the options set in the overlay take precedence.

Testing
=======

After programming the application and all the prerequisites to your Thingy:52, test the application by performing the following steps:

1. |connect_kit|
#. Connect to the kit with using J-LINK RTT viewer. Note: the Thingy:52 does not have a debugger, so you will need an external debugger (for example the nRF5340 development kit) and a T pin.
#. Reset the Thingy:52.
#. Observe in the terminal window that the THINGY:52 starts up and that the application starts.
   This is indicated by several <inf> module_name: "placeholder text" outputs and observe in the terminal window that the advertising has started, indicated by the following output::

      *** Booting Zephyr OS build v2.4.0-ncs1-2616-g3420cde0e37b  ***
      <inf> main: Starting advertising

#. Observe that the device establishes connection to nRF5340::

    <inf> main: Connected <Address of nRF5340>

#. Observe that data is sampled periodically and notifies the nRF5340::

    <inf> main: HTS221: Humidity: <X>%
   . . .
    <inf> main: Battery: <X>%


Known issues and limitations
****************************

The power floor in this application is higher than the standard firmware for the Thingy:52. So it should be possible to decrease the power floor further. Note: This application still uses less power overall due to lower advertising and connection intervals.

Dependencies
************
This section might need filling.
This application uses the following |NCS| libraries and drivers:

* :ref:`bluetooth/gatt`
* :ref:`drivers/sensor`
* :ref:`drivers/gpio`