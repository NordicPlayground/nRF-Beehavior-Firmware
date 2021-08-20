**This project is a modified version of a [sample](https://github.com/IRNAS/firmware-nordic-nrf52840dk-ble-nus) created by [IRNAS](https://github.com/IRNAS), which again is based on a [firmware](https://github.com/edgeimpulse/firmware-nrf52840-5340-dk) created by [Edge Impulse](https://www.edgeimpulse.com/).**

The same goes for the layout and content of this readme.

# Edge Impulse woodpecker detector for nRF52840 DK / nRF5340 DK

The woodpecker detector uses a machine learning algorithm created using the [Edge Impulse studio](https://studio.edgeimpulse.com/). When the sound of a woodpecker pecking or drumming on the hive is detected, a message is both printed on the serial port and sent to a connected device using the bluetooth service [NUS](https://devzone.nordicsemi.com/f/nordic-q-a/10567/what-is-nus-nordic-uart-service). As of now the information sent over NUS is a *const char* array with 8 elements, but this can be changed if it is desired to limit the amount of data to be sent.

To view the public version of this woodpecker detector project in the Edge Impulse studio follow [this link](https://studio.edgeimpulse.com/public/41560/latest).

On the serial port a table with the number of classifications for each class is printed. Over NUS there will only be sent an "alarm" or "streak alarm" if the sound we're looking for is detected a certain number of times, either in total for "alarm" or in a row for "streak alarm".

By using the Edge Impulse studio it is possible to create a different model detecting other sounds, and then implement that model with this framework by following the steps listed further down. When working with Edge Impulse their [docs page](https://docs.edgeimpulse.com/) is a great place to find tutorials and information, and their [forum](https://forum.edgeimpulse.com/) is a great place to ask questions.

This project also contains code for running a classification using an accelerometer. However this has not been the focus when designing the UART output, thus all changes in this project from the sample mentioned above only relates to the part of the application using the microphone.

The files that have been changed from what they look like in the original sample are "../prj.conf", "../edge-impulse/ingestion-sdk-c/ei_run_impulse.cpp", "../edge-impulse/ingestion-sdk-platform/NordicSemi-nrf52/ei_device_nordic_nrf52.cpp" and "../edge-impulse/ingestion-sdk-platform/NordicSemi-nrf52/ei_device_nordic_nrf52.h". See the sections with comments containing "BY ME" to see where the changes have been made.

IMPORTANT: As of now (Aug 13th 2021) this application has NOT been field tested!

## Requirements

**Hardware**

* Nordic Semiconductor [nRF52840 DK](https://docs.edgeimpulse.com/docs/nordic-semi-nrf52840-dk) or [nRF5340 DK](https://docs.edgeimpulse.com/docs/nordic-semi-nrf5340-dk).
* [X-NUCLEO-IKS02A1](https://www.st.com/en/ecosystems/x-nucleo-iks02a1.html) shield.
    > There may be an idea to put a piece of paper as insulation between the shield and the DK, to avoid unwanted contact between pins from the DK and the bottom of the shield.


**Software**

* [nRF Connect SDK](https://www.nordicsemi.com/Software-and-tools/Software/nRF-Connect-SDK)
    > As of Aug 12th 2021, make sure you select v1.5.1. v1.6.0 and newer created an error when building. See 2.1.1 in [this guide](https://devzone.nordicsemi.com/nordic/nrf-connect-sdk-guides/b/getting-started/posts/nrf-connect-sdk-tutorial---part-2-ncs-v1-4-0) on how to choose a specific version.

* [GNU ARM Embedded Toolchain 9-2019-q4-major](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads).
    > Using a newer version of this toolchain may also create a bug.

* [nRF Command Line Tools](https://www.nordicsemi.com/Software-and-tools/Development-Tools/nRF-Command-Line-Tools/Download).

* [Edge Impulse CLI](https://docs.edgeimpulse.com/docs/cli-installation)
    > If you want to control the application and read the output in the terminal on your computer (not the serial terminal). It is possible to run the application without installing this as well. Then you will have to use a serial terminal (e.g. Termite) to read the serial output from the DK, and use the mobile app mentioned below to run commands.
    > This tool can also be used to connect the DK to your project to record sound when doing *Data acquisition* or *Live classification* in the Edge Impulse studio.

* [nRF Toolbox app](https://www.nordicsemi.com/Products/Development-tools/nrf-toolbox)
    > A mobile app that lets you set up a connection between your mobile phone and your DK.


## Building and flashing the application (locally)

1. Clone this repository:

    ```
    git clone [git url of this project]
    ```

1. Build the application by running the following command in the application folder:

    **nRF52840 DK**

    ```
    west build -b nrf52840dk_nrf52840
    ```

    **nRF5340 DK**

    ```
    west build -b nrf5340dk_nrf5340_cpuapp
    ```

    > Add " -p" at the end if you want the whole of your project to be built again from scratch.

1. Flash the application by running the following command in the application folder:

    ```
    west flash
    ```

## Running the application (with NUS output on your mobile phone)

**Serial port output**

1. Run the command "edge-impulse-run-impulse --raw" in the terminal.

1. Choose your DK in the list of devices.
    > If you are unsure about which device to choose, take a look at the ports listed in your Device Manager (when using Windows OS).

1. Enter the command "AT+RUNIMPULSE".

1. The classification should now start, with the results printed to the terminal.


**NUS output**
In the nRF Toolbox app on your mobile phone:

1. Choose UART.

1. Click CONNECT.

1. Choose the device listed as "EdgeImpulse".

1. Edit the functionality of each button by clicking edit in the upper, right corner. A list of the different commands you can apply to each button is listed at the bottom of this readme.

1. Click the button you have given the command "AT+RUNIMPULSE" (it is also possible to write and send the command directly to the log described in the next step, but it is neater to use one of the buttons).

1. Click "Show log" in the menu displayed when clicking the three dots in the upper, right corner (or tilt your phone 90 degress to get a side-by-side view) to see the output that is sent using NUS.


## Deploy your own model

1. Download the the C++ library generated for your model in the "Deployment" section of the Edge Impulse studio.

1. Change the three folders "edge-impulse-sdk", "model-parameters" and "tflite-model" found in this project with the three folders, named the same, found in the downloaded C++ library.

1. Build and flash the application.


## Edge Impulse CLI commands

AT+HELP - Lists all commands

AT+CLEARCONFIG - Clears complete config and resets system

AT+CLEARFILES - Clears all files from the file system, this does not clear config

AT+CONFIG? - Lists complete config

AT+DEVICEINFO? - Lists device information

AT+SENSORS? - Lists sensors

AT+RESET - Reset the system

AT+WIFI? - Lists current WiFi credentials

AT+WIFI= - Sets current WiFi credentials (SSID,PASSWORD,SECURITY) (3 parameters)

AT+SCANWIFI - Scans for WiFi networks

AT+SAMPLESETTINGS? - Lists current sampling settings

AT+SAMPLESETTINGS= - Sets current sampling settings (LABEL,INTERVAL_MS,LENGTH_MS) (3 parameters)

AT+SAMPLESETTINGS= - Sets current sampling settings (LABEL,INTERVAL_MS,LENGTH_MS,HMAC_KEY) (4 parameters)

AT+UPLOADSETTINGS? - Lists current upload settings

AT+UPLOADSETTINGS= - Sets current upload settings (APIKEY,PATH) (2 parameters)

AT+UPLOADHOST= - Sets upload host (HOST) (1 parameter)

AT+MGMTSETTINGS? - Lists current management settings

AT+MGMTSETTINGS= - Sets current management settings (URL) (1 parameter)

AT+LISTFILES - Lists all files on the device

AT+READFILE= - Read a specific file (as base64) (1 parameter)

AT+READBUFFER= - Read from the temporary buffer (as base64) (START,LENGTH) (2 parameters)

AT+UNLINKFILE= - Unlink a specific file (1 parameter)

AT+SAMPLESTART= - Start sampling (1 parameter)

AT+READRAW= - Read raw from flash (START,LENGTH) (2 parameters)

AT+BOOTMODE - Jump to bootloader

AT+RUNIMPULSE - Run the impulse

AT+RUNIMPULSECONT - Run the impulse continuously

**Additional commands when the project is running**

'b' - break

'r' - reset (As of now this only works when command is sent using NUS, not when it is entered in the terminal of the connected computer)


## Data acquisition

* Data in the model labeled "noise_am" is noise from a beehive, contributed to the project by [Anya McGuirk](https://www.sas.com/content/dam/SAS/support/en/sas-global-forum-proceedings/2020/4509-2020.pdf). Thank you, Anya!
* Data labeled "noise_general" is noise from several different environments.
* Data labeled "woodpecker" is data gathered from [xeno-canto](https://www.xeno-canto.org/), and mostly sounds from the species [Dendrocopos major](https://www.xeno-canto.org/species/Dendrocopos-major).
* To make sure all of the data has the same sampling frequency, which is essential for the Edge Impulse tool to be able to analyze the data, [Audacity](https://www.audacityteam.org/) was used for up- or downsampling.
* A python script used to split longer wav files can be found [here](https://stackoverflow.com/questions/37999150/how-to-split-a-wav-file-into-multiple-wav-files/62872679#62872679).



## Further work

* Field testing
* Create a more robust model if necessary (would then possibly be an idea to collect more woodpecker data)
* Merge with rest of beehive monitoring system
* Energy consumption optimization