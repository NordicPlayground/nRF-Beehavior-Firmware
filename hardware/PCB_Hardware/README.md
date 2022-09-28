# README - Hardware


## Abstract

This is a README file for the PCB_hardware project.
This readme should give you a good introduction to the PCB, so that you can improve the design and make changes of your own. In short, this PCB converts power from a solar panal in to stable 5V. **The PCB is therefor a _DC-DC converter_**, and will be refered to this throughout this readme. 

## Background

One of the goals of the nRF-Beehavior project is that the system can run independently on batteries and charge itself. To de this we have decides that we are going to use solar panels. The input of the DC-DC converter is therefor directly connected to the spolar panel which fluctuate around 6V depending on the weather. The output voltage of this DC-DC converter can then be used to charge a powerbank (10 000 mA) which will be the main battery for the whole nRF-Beehavior system. The whole system has not yet been tested so we can't garantee that 1 powerbank is enough. If 1 powerbank is not enough, you can easly connect another powerbank but this may require you to solde up another DC-DC converter amd get more solar panels. 

Below you can see a image of the block diagram of the system:

![This is a image](https://github.com/NordicPlayground/nRF-Beehavior-Firmware/blob/master/PCB_Hardware/Illustrations/block_diagram.PNG)

Note that the PCBs for the nRF-Beehavior is then connected to 1 of the 2 outputs from the batterybank.

## Design

The PCb was designed in Altrium designer. Nordic has some keys for this program avalible but not enough for everyone hence you must contact someone that has administrative access to the Altium keys. I took contact with Runar Kjellhaug who also gave me good advice when it came to design choices and he helped me solve problems in Altium. Runar also helped me set ur the Altium libraries for Nordic, making it easier for me to fid footprints for all the components. I recommend having some experience with altium before you start improving this PCB. The PCB was designed with the following schematic and layout:

Schematic:

![Schematic](https://github.com/NordicPlayground/nRF-Beehavior-Firmware/blob/master/PCB_Hardware/Illustrations/Schematic.PNG)



PCB layout (Planes removed):

![PCB layout](https://github.com/NordicPlayground/nRF-Beehavior-Firmware/blob/master/PCB_Hardware/Illustrations/PCB_altium.PNG)



Here is the schematic but with explenations of the different parts of the circuit:

![Schematic with explenations](https://github.com/NordicPlayground/nRF-Beehavior-Firmware/blob/master/PCB_Hardware/Illustrations/Schematic_with_lines.PNG)



## Production 

When it comes to producing the PCB you have to export a "bill og materials". File can be a .csv or .xlsx file and should be goven to someone that can order the components. Depending on who you contact, you may have to use different sites to order from since not everyone at Nordic can order from every producer. I contacted Ketil Aas-Johansen and he could order from Farnell, Elfa Distrelec and digikey. 
The soldered PCB is shown below:

![PCB irl](https://github.com/NordicPlayground/nRF-Beehavior-Firmware/blob/master/PCB_Hardware/Illustrations/PCB_irl.jpg)


The full system assembled ready to charge the batterybank:
![System irl](https://github.com/NordicPlayground/nRF-Beehavior-Firmware/blob/master/PCB_Hardware/Illustrations/system_irl.jpg)


### Component list
Link to the exported bill of materials can be found [Here](https://github.com/NordicPlayground/nRF-Beehavior-Firmware/blob/master/PCB_Hardware/DC-DC%20converter/Bill%20of%20materials%20DC-DC%20converter.xlsx)

All components:
- 1 x Isolated DC/DC Converters 6W 4.5-9Vin 5Vout 1.2A SIP8 (RS6-0505S)
- 1 x Aluminum Electrolytic Capacitor, 220 uF, +/- 20%, 50 V, -40 to 85 degC, 2-Pin THD, RoHS, Bulk (ECA-1HM221)
- 1 x Chip Capacitor, 22µF +/-40%, 16V, 0805, Thickness 1.45 mm (Capacitor 22µF +/-40% 16V 0805)
- 1 x TVS DIODE 22V 41V 1610 (ESDA25P35-1U1M)
- 1 x Micro USB 2.0 B Receptacle, 5 Position, Height 3.24 mm, -55 to 85 degC, RoHS, Tape and Reel (10103594-0001LF)
- 2 x Chip Inductor, 1 uH, +/- 20%, 1 MHz, -55 to 125 degC, 1008 (2520 Metric), RoHS, Tape and Reel (LQM2HPN1R0MJ0L)
- 2 x Chip LED 0402, Red, 0.02 A, 2.0 to 2.5 V, -40 to 80 degC, 2-Pin SMD, RoHS, Tape and Reel (QBLP595-R)
- 1 x WR-MPC4 4.2mm Male Single Row Angled Header with Mounting Flanges for Screw-in Retention , 2p (64900211022)
- 1 x 6 Watt SIP8 Single and Dual Output (RS6-0505S)
- 2 x 1K 0.125W 5% 0805 (2012 Metric)  SMD (1KR5J)
- 1 x 0R 0805 (2012 Metric)  SMD (0R 0805)
- 4 x Wire crimps 3.00mm (MPC3)
- 3 x Female connector (MPC3)
- 2 x 3.5 Watt 6 Volt Solar Panel
- 1 x Female 3.5x1.1mm - Extension with Leads



## Test results

### Frequenzy respons test
We test the frequezy respons on the input. This shows the effectiveness of the bandpassfilter. We sweept the frequenzy from 10Hz - 100 Mhz:
- Not done



### Output noice
The noice of the output is shown below.

First we check noice from the DC power suplly (PSU) used for the test. We observe observe low noice with a maximum peak-to-peak voltage around 100mV. Note that this is not the optimal way to calculate noice however due to the lab being used a lot I didn't have access to the newer ocillioscope and hence I could export the data. This is why screenshots are used instead:

Noice from only PSU, peak-to-peak noice: 100mV:
![Noice_from_PSU](https://github.com/NordicPlayground/nRF-Beehavior-Firmware/blob/master/PCB_Hardware/Illustrations/Noice_from_DC%20power%20supply.PNG)


Then we test the noice on the output of the PCB with no load. Here we observe a peak-to-peak noice of 600mV.

Noice on the output of the PCB with no load, peak-to-peak noice: 600mV.
![Noice_from_system no load](https://github.com/NordicPlayground/nRF-Beehavior-Firmware/blob/master/PCB_Hardware/Illustrations/Noice_on_output_noload.PNG)


The we observe the noice when the system is connected to the powerbank (PB) at the output. Here we observed a simular peak-to-peak noice around 600mV.

Noice on the output of the PCB with powerbank as load, peak-to-peak noice: 600mV.
![Noice_from_system with load](https://github.com/NordicPlayground/nRF-Beehavior-Firmware/blob/master/PCB_Hardware/Illustrations/Noice_on_output_withload.PNG)















### Charge test:
We let the PCB be connected to a DC-powersupply delivering a constant 6V and a max current of 1A. We left the batterybank charging for 3 hours. We observed the following results:

Timestamp          | 13.15 | 13.30 | 13.45 | 14.00 | 14.15 | 14.30 | 14.45 | 15.00 | 15.15 | 15.30 | 15.45 | 16.00 |

Current PSU (6V)   | 0.32A | 0.35A | 0.33A | 0.33A | 0.34A |  0.31A | 0.32A | 0.34A | 0.34A | 0.35A | 0.35A | 0.34A |

Current PB (5V)    | 0.301A | 0.327A | 0.311A | 0.310A | 0.327A | 0.292A | 0.302A | 0.323A | 0.326A | 0.335A | 0.335A | 0.324A |

Power lost (W)     | 415mW | 465mW | 425mW |  430mW | 405mW | 400mW | 410mW | 425mW | 400mW | 425mW | 425mW | 420mW |

We observe that the batterybank is charging when the blue light on it is blinking.
During the test the powerbank went from 1 bar to 2 bars.


Results plotted:
![results plot](https://github.com/NordicPlayground/nRF-Beehavior-Firmware/blob/master/PCB_Hardware/Illustrations/resultat.png)

The test setup looked like this:
![charge_blokk](https://github.com/NordicPlayground/nRF-Beehavior-Firmware/blob/master/PCB_Hardware/Illustrations/testsetup_charge_blokkdiagram.PNG)

![charge test](https://github.com/NordicPlayground/nRF-Beehavior-Firmware/blob/master/PCB_Hardware/Illustrations/Testsetup_chargetest.jpeg)









### Solar panel test
The test was performed the same way as in the charge test but using the solar panels instead of the DC power supply. We took samples once every 15 minuttes for 1 hour. In this test we need to measure both voltage and current from the solar panels, hence the setup looked like this:
![solar panel test blokk](https://github.com/NordicPlayground/nRF-Beehavior-Firmware/blob/master/PCB_Hardware/Illustrations/solar_panel_test_setup.PNG)

First we set up the only 1 solar panel then we did 2.




#### 1 solar panel:
Timestamp          | 14.00 | 14.15 | 14.30 | 14.45 | 15.00 | 

Voltage solar panel (V) | 3.377 | 3.300 | 3.330 | 3.404 | 3.411 |

Current solar panel (V)  | 0.563 | 0.543 | 0.524 | 0.509 | 0.513 |

Power from solar panels | 1.879 | 1.792 | 1.729 | 1.733 | 1.750 | 
 






#### 2 solar panels:

Timestamp          | 14.00 | 14.15 | 14.30 | 14.45 | 15.00 | 

Voltage solar panel (V) | 3.660 | 3.550 | 3.593 | 3.653 | 3.704 |

Current solar panel (V)  | 1.094 | 1.000 | 1.011 | 1.014 | 1.066 |

Power from solar panels (W) | 4.004 | 3.550 | 3.633 | 3.704 | 3.948 | 


![solar_panel_plot](https://github.com/NordicPlayground/nRF-Beehavior-Firmware/blob/master/PCB_Hardware/Illustrations/plot_solarpanel_test.png)

(No picture of us testing, we forgot to take one)
 
 
## Discussion

We can se that from both the charge test and solar panel test that the powerbank charges when we connect it to the PCB with a power source. 
One important thing to not is that we lose a lot of power to the DC-DC converter (RS6-0505S). We know the powerloss comes from this source since the leds have a 1k resistor meaning they will only lose aroud 0.05W in total from leds (at 5V). This means that the DC-DC comverter is responsible for a loss of power around 21-24%. For our implementation this is probably not a problem but it can couse overheating if we connect more solar panels. If a rev.2 is to be made. 

Another problem we experienced is that the wires kept falling out. The general robustness of the system is not to good and can be improved. Also, the system is not waterproof something it has te be before we instal it outside.



## Improvements
- Might be a good idea to change the DC-DC converter to something more efficient. Maybe this: (https://www.digikey.no/no/products/detail/artesyn-embedded-power/LDO10C-005W05-SJ/2347825)
- System need to be waterproof. This can be done with making a box and using nail polish on the components.
- Improve robustnes. This means better crimps and better connectors.








## 3D printed box
This box is a housing for the DC-DC converter and can be found under "3D_prints" folder. In this folder there is a bottom part and top part. Insert the PCB inside the bottom part and put the top part on top. use the sqrew holes to secure the bottom and top part together.

![DC_DC_box](https://github.com/NordicPlayground/nRF-Beehavior-Firmware/blob/master/PCB_Hardware/Illustrations/box_DCDC.PNG)

NB! The width of the bow was 36mm (inside) however this is wrong the pcb is 38mm wide. This meant I had to sand the pcb and bow to make it fit. The Fit between the top and bottom part is also a bit tight but it will fit if you use some force. I don't think it is as easy to seperate it though so be careful...
