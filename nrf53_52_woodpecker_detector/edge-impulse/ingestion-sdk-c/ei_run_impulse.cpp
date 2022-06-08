/* Edge Impulse ingestion SDK
 * Copyright (c) 2020 EdgeImpulse Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


// Modifications made for the woodpecker alarm project are indicated by the comment BY ME.


/* Include ----------------------------------------------------------------- */
#include "edge-impulse-sdk/classifier/ei_run_classifier.h"
#include "edge-impulse-sdk/dsp/numpy.hpp"
#include "ei_inertialsensor.h"
#include "ei_microphone.h"
#include "ei_device_nordic_nrf52.h"
#include "ble_nus.h"
//addded by Nora////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <zephyr.h>
#include <device.h>
#include <init.h>
#include <power/power.h>
#include <hal/nrf_gpio.h>

#define CONSOLE_LABEL DT_LABEL(DT_CHOSEN(zephyr_console))

#define BUSY_WAIT_S 2U
#define SLEEP_S 2U

//#define PIN_1_NODE  DT_ALIAS(gpio_pin_name)
#define PIN1 3
const struct device * devi;

//vil at PIN1 skal settes til gpio-pinnen P0.03
//dette er for å kunne bruke tid fra 91-eren, men må nok teste MED selve kortet først hehe
/*#include <date_time.h>*/

/*TIME*/
/*s64_t unix_time_ms;*/

/*Force the date_time library to get current time */
/*date_time_update();*/

/*Read current time and put in container */
/*err = date_time_now(&unix_time_ms);*/

/*Print the current time */
/*printk("Date_time: %i %i\n", (u32_t)(unix_time_ms/1000000), (u32_t)(unix_time_ms%1000000));*/

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



#if defined(EI_CLASSIFIER_SENSOR) && EI_CLASSIFIER_SENSOR == EI_CLASSIFIER_SENSOR_ACCELEROMETER

/* Private variables ------------------------------------------------------- */
static float acc_buf[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE];
static int acc_sample_count = 0;

static bool acc_data_callback(const void *sample_buf, uint32_t byteLength)
{
    float *buffer = (float *)sample_buf;
    for(int i = 0; i < (int)(byteLength / sizeof(float)); i++) {
        acc_buf[acc_sample_count + i] = buffer[i];
    }

    return true;
}

// static void acc_read_data(float *values, size_t value_size)
// {    
//     for (size_t i = 0; i < value_size; i++) {
//         values[i] = acc_buf[i];
//     }
// }

void run_nn(bool debug)
{
    char ble_printf[64] = {0};
    bool stop_inferencing = false;
    // summary of inferencing settings (from model_metadata.h)
    ei_printf("Inferencing settings:\n");
    ei_printf("\tInterval: ");
    ei_printf_float((float)EI_CLASSIFIER_INTERVAL_MS);
    ei_printf("ms.\n");
    ei_printf("\tFrame size: %d\n", EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE);
    ei_printf("\tSample length: ");
    ei_printf_float(1000.0f * static_cast<float>(EI_CLASSIFIER_RAW_SAMPLE_COUNT) /
                  (1000.0f / static_cast<float>(EI_CLASSIFIER_INTERVAL_MS)));
    ei_printf("ms.\n");
    ei_printf("\tNo. of classes: %d\n", sizeof(ei_classifier_inferencing_categories) /
                                            sizeof(ei_classifier_inferencing_categories[0]));

    ei_printf("Starting inferencing, press 'b' to break\n");

    ei_inertial_sample_start(&acc_data_callback, EI_CLASSIFIER_INTERVAL_MS);

    while (stop_inferencing == false) {
        ei_printf("Starting inferencing in 2 seconds...\n");

        // instead of wait_ms, we'll wait on the signal, this allows threads to cancel us...
        if (ei_sleep(2000) != EI_IMPULSE_OK) {
            break;
        }

        if(ei_user_invoke_stop()) {
            ei_printf("Inferencing stopped by user\r\n");
            break;
        }

        if (stop_inferencing) {
            break;
        }

        ei_printf("Sampling...\n");

            /* Run sampler */
        acc_sample_count = 0;
        for(int i = 0; i < EI_CLASSIFIER_RAW_SAMPLE_COUNT; i++) {
            ei_inertial_read_data();
            acc_sample_count += EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME;
        }

        // Create a data structure to represent this window of data
        signal_t signal;
        int err = numpy::signal_from_buffer(acc_buf, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, &signal);
        if (err != 0) {
            ei_printf("ERR: signal_from_buffer failed (%d)\n", err); 
        }

        // run the impulse: DSP, neural network and the Anomaly algorithm
        ei_impulse_result_t result = { 0 };
        EI_IMPULSE_ERROR ei_error = run_classifier(&signal, &result, debug);
        if (ei_error != EI_IMPULSE_OK) {
            ei_printf("Failed to run impulse (%d)\n", ei_error);
            break;
        }

        // print the predictions
        ei_printf("Predictions (DSP: %d ms., Classification: %d ms., Anomaly: %d ms.): \n",
            result.timing.dsp, result.timing.classification, result.timing.anomaly);
        for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {            
            ei_printf("    %s: \t", result.classification[ix].label);
            ei_printf_float(result.classification[ix].value);
            ei_printf("\r\n");
        }
#if EI_CLASSIFIER_HAS_ANOMALY == 1
        ei_printf("    anomaly score: ");
        ei_printf_float(result.anomaly);
        ei_printf("\r\n");        
#endif

    /*BLE PRINTF*/
        for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {            
            sprintf(ble_printf, "%s: %f\n", result.classification[ix].label, result.classification[ix].value);
            ble_nus_send_data(ble_printf, strlen(ble_printf));
        }
#if EI_CLASSIFIER_HAS_ANOMALY == 1
        sprintf(ble_printf, "anomaly score: %f\n", result.anomaly);
        ble_nus_send_data(ble_printf, strlen(ble_printf));
#endif

        memset(ble_printf, 0x00, sizeof(ble_printf));
    /*END BLE PRINTF*/

        if(ei_user_invoke_stop()) {
            ei_printf("Inferencing stopped by user\r\n");
            ble_nus_send_data("Inferencing stopped by user\n", strlen("Inferencing stopped by user\n"));
            memset(ble_printf, 0x00, sizeof(ble_printf));
            break;
        }
    }
}
#elif defined(EI_CLASSIFIER_SENSOR) && EI_CLASSIFIER_SENSOR == EI_CLASSIFIER_SENSOR_MICROPHONE

// **BY ME**
// Create an array to gather classification data in.
const char* classification_label_name [EI_CLASSIFIER_LABEL_COUNT+1];
uint8_t classification_label_count [EI_CLASSIFIER_LABEL_COUNT+1];
uint8_t classification_label_count_streak [EI_CLASSIFIER_LABEL_COUNT+1];
bool label_found = false; // Flag telling if a class is found (true) or if it is uncertain (false).
bool label_alert = false; // Flag telling an alert has been triggered (true), used for activation of LEDs.
bool label_streak_alert = false; // Flag telling a streak alert has been triggered (true), used for activation of LEDs.
uint8_t min_conf_rating = 70; // Minimum confidence rating x100 for deciding on a class.
uint8_t num_total_alert = 5; // Number of observations in total to trigger an alert.
uint8_t num_streak_alert = 3; // Number of observations in a row to trigger a streak alert.
uint8_t index_alert = 2; // Index of interesting sound in classification_label_name array.
// *****

/*
#define EI_LED_ALL      BOARD_ledSetLedOn(1, 1, 1, 1)
#define EI_LED_BLUE     BOARD_ledSetLedOn(1, 0, 0, 0)
#define EI_LED_GREEN    BOARD_ledSetLedOn(0, 1, 0, 0)
#define EI_LED_YELLOW   BOARD_ledSetLedOn(0, 0, 1, 0)
#define EI_LED_RED      BOARD_ledSetLedOn(0, 0, 0, 1)
#define EI_LED_OFF      BOARD_ledSetLedOn(0, 0, 0, 0)
*/

/*
// Resets classification table values. **BY ME**
if(ei_user_invoke_reset() || ei_ble_user_invoke_reset()) {
    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
        classification_label_count[ix] = 0;
        classification_label_count_streak[ix] = 0;
    }
}
// *****
*/

void run_nn(bool debug)
{      
    char ble_printf[8] = {0};  // Can be lowered to send fewer bytes.
    extern signal_t ei_microphone_get_signal();

    // summary of inferencing settings (from model_metadata.h)
    ei_printf("Inferencing settings:\n");
    ei_printf("\tInterval: ");
    ei_printf_float((float)EI_CLASSIFIER_INTERVAL_MS);
    ei_printf("ms.\n");
    ei_printf("\tFrame size: %d\n", EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE);
    ei_printf("\tSample length: %d ms.\n", EI_CLASSIFIER_RAW_SAMPLE_COUNT / 16);
    ei_printf("\tNo. of classes: %d\n", sizeof(ei_classifier_inferencing_categories) /
                                            sizeof(ei_classifier_inferencing_categories[0]));

    if (ei_microphone_inference_start(EI_CLASSIFIER_RAW_SAMPLE_COUNT) == false) {
        ei_printf("ERR: Failed to setup audio sampling\r\n");
        return;
    }

    ei_printf("Starting inferencing. Press 'b' to break.\n");


    // Fill upper row of classification label array with class labels. **BY ME**
    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
       classification_label_name [ix] = ei_classifier_inferencing_categories[ix];
    }

    classification_label_name [EI_CLASSIFIER_LABEL_COUNT] = "undefined"; // Adds undefined label for when min conf rating isn't reached.

    while (1) {
        ei_printf("Starting inferencing in 2 seconds...\n");

        BOARD_ledSetLedOn(0, 0, 0, 0); // resets LEDs from last classification **BY ME**

        // instead of wait_ms, we'll wait on the signal, this allows threads to cancel us...
        if (ei_sleep(2000) != EI_IMPULSE_OK) {
            break;
        }

        if(ei_user_invoke_stop() || ei_ble_user_invoke_stop()) {
            ei_printf("Inferencing stopped by user\r\n");
            break;
        }

        ei_printf("Recording...\n");

        ei_microphone_inference_reset_buffers();
        bool m = ei_microphone_inference_record();
        if (!m) {
            ei_printf("ERR: Failed to record audio...\n");
            break;
        }

        ei_printf("Recording done\n");

        signal_t signal;
        signal.total_length = EI_CLASSIFIER_RAW_SAMPLE_COUNT;
        signal.get_data = &ei_microphone_audio_signal_get_data;
        ei_impulse_result_t result = {0};

        EI_IMPULSE_ERROR r = run_classifier(&signal, &result, debug);
        if (r != EI_IMPULSE_OK) {
            ei_printf("ERR: Failed to run classifier (%d)\n", r);
            break;
        }

        // print the predictions
        ei_printf("Predictions (DSP: %d ms., Classification: %d ms., Anomaly: %d ms.): \n",
            result.timing.dsp, result.timing.classification, result.timing.anomaly);
        for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
            ei_printf("    %s: \t", result.classification[ix].label);
            ei_printf_float(result.classification[ix].value);
            ei_printf("\r\n");
            // ** BLE printf ** add this if you want 
            //sprintf(ble_printf, "%s: %f\n", result.classification[ix].label, result.classification[ix].value);
            //ble_nus_send_data(ble_printf, strlen(ble_printf));
            // ** End BLE printf **
            
            // Adding classifications to array **BY ME**
            if ((result.classification[ix].value)*100 >= min_conf_rating) {
                classification_label_count [ix] += 1;
                classification_label_count_streak [ix] += 1;
                label_found = true; // confirms that a class is found.
                // result_ble += ix; // code to be sent over BLE is updated to correct value.
            }
            
            // Resetting streak of classifications **BY ME**
            else {
                classification_label_count_streak [ix] = 0;
            }

        }

        // Adds observation to undefined class (last index), if the min conf rating isn't reached by any of the defined classes. **BY ME**
        if (!label_found) {
            classification_label_count[EI_CLASSIFIER_LABEL_COUNT] += 1;
        }

        // Code for printing table **BY ME**
        for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT+1; ix++) {
            ei_printf("%s \t", classification_label_name[ix]);    
            }   
        ei_printf("\r\n");
           
        for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT+1; ix++) {
            ei_printf("%d \t\t", classification_label_count[ix]);
        }
        ei_printf("\r\n");

        for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT+1; ix++) {
            ei_printf("%d \t\t", classification_label_count_streak[ix]);
        }
        
        // Code for deciding when to give alert and/or streak alert **BY ME**
        for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
            if (classification_label_count[ix] > num_total_alert && ix == index_alert) {
            label_alert = true;
            ei_printf("\n %s ALERT!!!", classification_label_name[ix]);
            sprintf(ble_printf, "\n %s ALERT!!!", classification_label_name[ix]);
            ble_nus_send_data(ble_printf, strlen(ble_printf));
            }
            if (classification_label_count_streak[ix] > num_streak_alert && ix == index_alert) {
            label_streak_alert = true;
            ei_printf("\n %s STREAK ALERT!!!", classification_label_name[ix]);
            sprintf(ble_printf, "\n %s STREAK ALERT!!!", classification_label_name[ix]);
            ble_nus_send_data(ble_printf, strlen(ble_printf));
            }
        }
        ei_printf("\r\n");

        /*
        // Activates LEDs if alert and/or streak alert is triggered (not really necessary).
        if (label_alert && label_streak_alert) { // Blinks LED 1 & 4 if both alerts are triggered.
            BOARD_ledSetLedOn(1, 0, 0, 1);
            k_msleep(500);    
        }

        else if (label_alert) { // Blinks only LED 1 if only regular alert is triggered.
            BOARD_ledSetLedOn(1, 0, 0, 0);
            k_msleep(500);    
        }

        else if (label_streak_alert) { // Blinks only LED 4 if only streak alert is triggered.
            BOARD_ledSetLedOn(0, 0, 0, 1);
            k_msleep(500);    
        }
        */

        label_alert = false; // resets flag
        label_streak_alert = false; // resets flag
        label_found = false; // resets flag

#if EI_CLASSIFIER_HAS_ANOMALY == 1
        ei_printf("    anomaly score: ");
        ei_printf_float(result.anomaly);
        ei_printf("\r\n");
#endif


    // Add the following if you want classification results to be printed over nus as well.
    /*BLE PRINTF
        for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {            
            sprintf(ble_printf, "%s: %f\n", result.classification[ix].label, result.classification[ix].value);
            ble_nus_send_data(ble_printf, strlen(ble_printf));
        }
#if EI_CLASSIFIER_HAS_ANOMALY == 1
        sprintf(ble_printf, "anomaly score: %f\n", result.anomaly);
        ble_nus_send_data(ble_printf, strlen(ble_printf));
#endif

        memset(ble_printf, 0x00, sizeof(ble_printf));
    END BLE PRINTF*/
        
        if(ei_user_invoke_stop() || ei_ble_user_invoke_stop()) {
            ei_printf("Inferencing stopped by user\r\n");
            ble_nus_send_data("Inferencing stopped by user\n", strlen("Inferencing stopped by user\n"));
            memset(ble_printf, 0x00, sizeof(ble_printf));
            break;
        }

        
        // Resets classification table values by entering 'r'. **BY ME**
        else if(ei_user_invoke_reset() || ei_ble_user_invoke_reset()) {
            for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT+1; ix++) {
                classification_label_count[ix] = 0;
                classification_label_count_streak[ix] = 0;
            }
        }
        // *****
    }

    ei_microphone_inference_end();
}

void run_nn_continuous(bool debug)
{
    char ble_printf[8] = {0}; // Can be lowered to send fewer bytes.
    bool stop_inferencing = false;
    int print_results = -(EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW);
    // summary of inferencing settings (from model_metadata.h)
    ei_printf("Inferencing settings:\n");
    ei_printf("\tInterval: ");
    ei_printf_float((float)EI_CLASSIFIER_INTERVAL_MS);
    ei_printf("ms.\n");
    ei_printf("\tFrame size: %d\n", EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE);
    ei_printf("\tSample length: %d ms.\n", EI_CLASSIFIER_RAW_SAMPLE_COUNT / 16);
    ei_printf("\tNo. of classes: %d\n", sizeof(ei_classifier_inferencing_categories) /
                                            sizeof(ei_classifier_inferencing_categories[0]));

    ei_printf("Starting inferencing. Press 'b' to break.\n");

    run_classifier_init();
    ei_microphone_inference_start(EI_CLASSIFIER_SLICE_SIZE);

    // Fill upper row of classification label array with class labels. **BY ME**
    for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
       classification_label_name [ix] = ei_classifier_inferencing_categories[ix];
    }

    while (stop_inferencing == false) {

        bool m = ei_microphone_inference_record();
        if (!m) {
            ei_printf("ERR: Failed to record audio...\n");
            break;
        }

        signal_t signal;
        signal.total_length = EI_CLASSIFIER_SLICE_SIZE;
        signal.get_data = &ei_microphone_audio_signal_get_data;
        ei_impulse_result_t result = {0};

        EI_IMPULSE_ERROR r = run_classifier_continuous(&signal, &result, debug);
        if (r != EI_IMPULSE_OK) {
            ei_printf("ERR: Failed to run classifier (%d)\n", r);
            break;
        }

        if (++print_results >= (EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW >> 1)) {
            // print the predictions
            ei_printf("Predictions (DSP: %d ms., Classification: %d ms., Anomaly: %d ms.): \n",
                result.timing.dsp, result.timing.classification, result.timing.anomaly);
            for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
                ei_printf("    %s: \t", result.classification[ix].label);
                ei_printf_float(result.classification[ix].value);
                ei_printf("\r\n");

                // Adding classifications to array **BY ME**
                if ((result.classification[ix].value)*100 >= min_conf_rating) {
                    classification_label_count [ix] += 1;
                    classification_label_count_streak [ix] += 1;
                    label_found = true; // confirms that a class is found.
                }
            
                // Resetting streak of classifications if the actual class isn't classified this time. **BY ME**
                else {
                    classification_label_count_streak [ix] = 0;
                }
            }

            // Adds observation to uncertain class, if the min conf rating isn't fulfilled by any of the defined classes. **BY ME**
            if (!label_found) {
                classification_label_count[EI_CLASSIFIER_LABEL_COUNT] += 1;
            }

            // Code for printing table **BY ME**
            for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
                ei_printf("%s \t", classification_label_name[ix]);    
            }   
            ei_printf("\r\n");
           
            for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
                ei_printf("%d \t\t", classification_label_count[ix]);
            }
            ei_printf("\t Total");
            ei_printf("\r\n");

            for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
                ei_printf("%d \t\t", classification_label_count_streak[ix]);
            }
            ei_printf("\t Streak");
            // *****
        
            // Code for deciding when to give alert **BY ME**
            for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
                if (classification_label_count[ix] > num_total_alert) {
                    //label_alert = true;
                    ei_printf("\n %s ALERT!!!", classification_label_name[ix]);
                }
                if (classification_label_count_streak[ix] > num_streak_alert) {
                    //label_alert = true;
                    ei_printf("\n %s STREAK ALERT!!!", classification_label_name[ix]);
                }
            }
            ei_printf("\r\n");

            label_found = false; // resets flag
            // *****

#if EI_CLASSIFIER_HAS_ANOMALY == 1
            ei_printf("    anomaly score: ");
            ei_printf_float(result.anomaly);
            ei_printf("\r\n");
#endif

            print_results = 0;
        }

    // Add the following if you want classification results to be printed over nus as well.
    /*BLE PRINTF
        for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {            
            sprintf(ble_printf, "%s: %f\n", result.classification[ix].label, result.classification[ix].value);
            ble_nus_send_data(ble_printf, strlen(ble_printf));
        }
#if EI_CLASSIFIER_HAS_ANOMALY == 1
        sprintf(ble_printf, "anomaly score: %f\n", result.anomaly);
        ble_nus_send_data(ble_printf, strlen(ble_printf));
#endif

        memset(ble_printf, 0x00, sizeof(ble_printf));
    END BLE PRINTF*/

        if(ei_user_invoke_stop() || ei_ble_user_invoke_stop()) {
            ei_printf("Inferencing stopped by user\r\n");
            ble_nus_send_data("Inferencing stopped by user\n", strlen("Inferencing stopped by user\n"));
            memset(ble_printf, 0x00, sizeof(ble_printf));
            break;
        }

        // Resets classification table values by entering 'r'. **BY ME**
        else if(ei_user_invoke_reset() || ei_ble_user_invoke_reset()) {
            ei_printf("Table reset\r\n");
            for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT+1; ix++) {
                classification_label_count[ix] = 0;
                classification_label_count_streak[ix] = 0;
            }
        }
        // *****
    }

    ei_microphone_inference_end();
}

#else

#error "EI_CLASSIFIER_SENSOR not configured, cannot configure `run_nn`"

#endif  // EI_CLASSIFIER_SENSOR

void run_nn_continuous_normal()
{
#if defined(EI_CLASSIFIER_SENSOR) && EI_CLASSIFIER_SENSOR == EI_CLASSIFIER_SENSOR_MICROPHONE
    run_nn_continuous(false);
#else
    ei_printf("Error no continuous classification available for current model\r\n");
#endif
}

void run_nn_normal(void)
{
    run_nn(false);
}

//added BY NORA /////////////////////////////////////////////////////////////////////7


/* Prevent deep sleep (system off) from being entered on long timeouts
 * or `K_FOREVER` due to the default residency policy.
 *
 * This has to be done before anything tries to sleep, which means
 * before the threading system starts up between PRE_KERNEL_2 and
 * POST_KERNEL.  Do it at the start of PRE_KERNEL_2.
 */

//gpio_pin_configure(dev, LED0, GPIO_OUTPUT);
//Må adde i prjconfig for å kunne bruke tid!!!!!!!


static int disable_ds_1(const struct device *dev)
{
	ARG_UNUSED(dev);

	pm_ctrl_disable_state(POWER_STATE_DEEP_SLEEP_1);
	return 0;
}

SYS_INIT(disable_ds_1, PRE_KERNEL_2, 0);

void MakingSysSleepNight (int clock_signal){
    int rc;
    const struct device *cons = device_get_binding(CONSOLE_LABEL);
    const struct device * devi = device_get_binding("GPIO_0"); //der lager vi dev som  skal brukes 
    if (clock_signal>1900 || clock_signal<0600){
        

	    printk("\n%s system off demo\n", CONFIG_BOARD,"\n%s er i funksjonen\n");

	    /* Configure to generate PORT event (wakeup) on button 1 press. */
         nrf_gpio_cfg_input(PIN1, //må finne ut hvordan man setter pins for vi vil ikke ha knapp
			    NRF_GPIO_PIN_PULLDOWN);
	    nrf_gpio_cfg_sense_set(PIN1,
			       NRF_GPIO_PIN_SENSE_HIGH); //nylilg addet som gjør at man kan restarte dette programmet når en pin er høy!!!diggesen

       /* gpio_pin_configure(devi,PIN1, GPIO_INT_LEVEL_HIGH );// her tror jeg at den sier at det resettes når pin 1 mottar strøm 
        gpio_pin_set(devi,PIN1,0); // får ikke feilmelding, men det funker ikke mehehe*/

	    /*printk("Busy-wait %u s\n", BUSY_WAIT_S);
	    k_busy_wait(BUSY_WAIT_S * USEC_PER_SEC);
        
	    printk("Busy-wait %u s with UART off\n", BUSY_WAIT_S);
	    rc = device_set_power_state(cons, DEVICE_PM_LOW_POWER_STATE, NULL, NULL);
	    k_busy_wait(BUSY_WAIT_S * USEC_PER_SEC);
	    rc = device_set_power_state(cons, DEVICE_PM_ACTIVE_STATE, NULL, NULL);

	    printk("Sleep %u s\n", SLEEP_S);
	    k_sleep(K_SECONDS(SLEEP_S));

	    printk("Sleep %u s with UART off\n", SLEEP_S);
        */
	    printk("Entering system off; press BUTTON1 to restart\n");

	    rc = device_set_power_state(devi, DEVICE_PM_LOW_POWER_STATE, NULL, NULL);
	    /*k_sleep(K_MSEC(2));//lagt inn to sekunder her, så får vi se
	    rc = device_set_power_state(cons, DEVICE_PM_ACTIVE_STATE, NULL, NULL);*/
        
    

	    printk("Entering system off; press BUTTON1 to restart\n");

	    /* Above we disabled entry to deep sleep based on duration of
	     * controlled delay.  Here we need to override that, then
	     * force entry to deep sleep on any delay.
	     */
	    pm_power_state_force(POWER_STATE_DEEP_SLEEP_1);
	    k_sleep(K_MSEC(2));

	    printk("ERROR: System off failed\n");
	    while (true) {
		    /* spin to avoid fall-off behavior */
	    }

    }
    else {
        printk("it's dayyy");
    }
 

}
// hva skal bli gjort i dag???
//vil ha at det blir hver time sjekket hva tiden er i nrf91-ern, 
//om klokka er typ 07.00, så skal et spenningssignal sendes til nrf5284 og få den til å enten være våken eller sove. 
