# Easy bee counter

This project is based on [hydronics bee counter project](https://www.instructables.com/Easy-Bee-Counter/). This particular project utilizes the adafruit itsybitsy nrf52840 express, which the code is written for. 

## Assembling instructions

Follow the assembling instructions as shown in the above link. 

The itsybitsy does not come with an onboard debugger, which is why a [nRF52840 DK](https://www.nordicsemi.com/Products/Development-hardware/nrf52840-dk), is required to flash to the uController. 

The underside of the adafruit itsybitsy nRF52840 includes **SWCLK** & **SWDIO** debug-interface pins, which needs to be soldered with wires and connected to the DK as shown [here](https://devzone.nordicsemi.com/f/nordic-q-a/14058/external-programming-using-nrf52-dk)

**NOTE** The SCK pinout on the PCB divides the voltage between the pinout for the itsybitsy and for the feather. To solve this issue, solder a wire between these holes as shown in the picture below.

![Solder example](https://github.com/NordicPlayground/nRF-Beehavior-Firmware/blob/master/bee_counter/pics/solder.jpg)


## How to build and flash

To build and flash using the Zephyr RTOS, make sure to have to necessary devicetree files which is included in this repository. Add the folder "adafruit_itsybitsy_nrf52840" to ncs/zephyr/arm/boards.

1. Install necessary software if you haven't already following [this](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/gs_installing.html) guide.

1. Build the project using west from commandline:

    ```
    west build -b adafruit_itsybitsy_nrf52840 -p
    ```


1. And flash using west:

    ```
    west flash
    ```

    > **Note:** Remember to test the IR LEDs **before** soldering the jumpers and the test registers **before** installing the headers for the gates. 

