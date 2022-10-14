EI MIC sample
##############

Usage
********************
This sample is a simple implementation of how to use sound in a Edge impulse model, running on the nRF52/53.
This specific sample will scan for the presence of woodpeckers.
It will output the data in a probability from 0 to 1.



Building and running
********************

+----------------------------------+
| Compatible devices               |
+==================================+
| nRF52840DK                       |
+----------------------------------+ 
| nRF5340DK                        |
+----------------------------------+

Building can be done using the vscode extension, or the following commands::

#For the nRF52
west build -p -b nrf52840dk_nrf52840

#For the nRF53
west build -p -b nrf5340dk_nrf5340_cpuapp
Testing
=======

The output data is accessible trough JLink RTT Viewer

Configuration notes
********************

You can use any PDM microphone by editing the devicetree overlay of the relevant board


.. code-block:: devicetree

   &pinctrl {
       pdm0_default_alt: pdm0_default_alt {
           group1 {
                  # Change this to fit your setup:
               psels = <NRF_PSEL(PDM_CLK, 1, 5)>,
                   <NRF_PSEL(PDM_DIN, 1, 6)>;
           };
       };
   };