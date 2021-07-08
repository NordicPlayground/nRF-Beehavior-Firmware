Pr. 08.07.2021:

This folder contains the individual modules for the nRF5340dk for the Beehavior-Firmware which works (somewhat) as a prototype for the individual modules. The code has not 
yet been merged to work as one unit.

To do:
- Send this to the nRF91 in a similar manner to Peripheral_nRF53_To_nRF91, and upload to the cloud
- Add Scan_For_Broodminder module as another peripheral and integrate this data to the system aswell

- Make the system more energy conservative:
    - Reduce the sampling rate on the Thingy:52 from 5(?) seconds to 60 seconds
    - Buffer the sampled sensor data on the nRF5340dk and only upload every x'th sample. Suggestion for x = 15 minutes, since the BM-W samples with 15 minute sample time.
            - Event detection: Increase the sample rate by using the buffered sensor data in case of an event such as Swarming. 
                      if (temp_x+1 >> temp_x) {also send temp_k}, where temp_k is the buffered samples between x+1 and x to increase the resolution of the temp data on the cloud

- Add extra functionality and data preprocessing:
    - State estimation for amount of honey in hive
    - State estimation for honey production efficiency (nectar to honey evaporation rate)
    - Bee counter
    - Add Edge Impulse on sound 

- Rewrite module descriptions as we go and update README.rst


******** Sketch for main(): Alternative 1, assuming it is possible for nRF5340dk to have both central and peripheral role
main(){
    Init BLE, lights, buttons etc;
    
    Connect to external sensor services;
    
    Connect to central (9160DK);
 
    for(;;){
        Read_data_from_sensors_at_time_k;
            - = A merge of Thingy_To_nRF5340 and scan_for_broodminder
        
        Buffer_data_from_sensors_at_time_k;
            - To do
        
        Preprocess_data;
            - To do
        
        Send_PP_data_to_Central
            - = A merge of above mentioned functions and peripheral_nRF53_To_nRF91
            
        OR
        
        Advertise_PP_data_And_Let_9160dk_catch_them;
            - May not require multi-role nRF5340dk?
       
        }


******** Sketch for main(): Alternative 2, assuming it is not possible for nRF5340dk to have both central and peripheral role
main(){
    Init BLE, lights, buttons etc;
    

 
    for(;;){
        Set_role_as_central;
        Connect_to_external_sensor_services;
        Read_data_from_
    
        
        Read_data_from_sensors_at_time_k;
            - = A merge of Thingy_To_nRF5340 and scan_for_broodminder
        
        Buffer_data_from_sensors_at_time_k;
            - To do
        
        Preprocess_data;
            - To do
        
        Set_role_as_peripheral;
        Connect_to_central (9160dk);
        Send_PP_data_to_Central
            - = A merge of above mentioned functions and peripheral_nRF53_To_nRF91
        
        or 
        
        Advertise_data_and_let_9160DK_pick_up_advertisements;
       
        }
        
        
// Module description:
Thingy_To_nRF5340:
nRF5340DK connects as a central to a Thingy:52 and gets access to multiple services. The services available at this time is:
- Temperature
- Relative Humidity
- Orientation

Peripheral_nRF53_To_nRF91:
Acts as the peripheral uart example and communicates with an nRF9160dK, which in turn sends data recieved from the nRF5340dk to the cloud. The source code for the nRF9160dk can
be seen in the nRF91 folder.

The goal is to augment the nRF53-peripheral uart demo to take in info from Thingy_To_nRF5340 module instead.

scan_for_broodminder:
Scans for a broodminder advertisement device and reads the advertised data. The goal is to extract the weight bytes and merge this module with the Thingy_To_nRF5340 module.
