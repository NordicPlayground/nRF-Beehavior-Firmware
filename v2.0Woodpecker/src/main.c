
#include <zephyr/kernel.h>
#include <ei_wrapper.h>
#include <logging/log.h>

#include "input_data.h"
#include "ble.h"
#include "mic.h"

LOG_MODULE_REGISTER(MAIN);

#define FRAME_ADD_INTERVAL_MS	100


static size_t frame_surplus;


static void result_ready_cb(int err)
{
	if (err) {
		LOG_ERR("Result ready callback returned error (err: %d)\n", err);
		return;
	}

	LOG_INF("Input data: %f", input_data[14983]);

	const char *label;
	float value;
	size_t inx;
	float anomaly;

	LOG_INF("Classification results\n");

	while (true) {
		err = ei_wrapper_get_next_classification_result(&label, &value, &inx);

		if (err) {
			if (err == -ENOENT) {
				err = 0;
				LOG_ERR("Edge Impulse wrapper failed to initialize (err: %d)\n",
		       		err);
			}
			else{
				LOG_ERR("Edge Impulse wrapper failed to initialize (err: %d)\n",
				err);
			   
			}
			break;
		}
		LOG_INF("idx: %i", inx);
		LOG_INF("Value: %.2f\tLabel: %s\n", value, label);
		if (inx == 2){

			woodpecker = 1; 

			LOG_INF("%f", woodpecker);

			memcpy(&woodpecker, &value, sizeof(woodpecker));

			LOG_INF("%f", woodpecker);

			
		}
	}

	LOG_INF("Escaped from while(true)");
	
	if (err) {
		LOG_ERR("Cannot get classification results (err: %d)", err);
	} else {
		if (ei_wrapper_classifier_has_anomaly()) {
			err = ei_wrapper_get_anomaly(&anomaly);
			if (err) {
				LOG_WRN("Cannot get anomaly (err: %d)\n", err);
			} else {
				LOG_INF("Anomaly: %.2f\n", anomaly);
			}
		}
		else{
			LOG_INF("No anomaly");
		}
	}
	LOG_INF("Out of if(err)");

	LOG_INF("Fram_surplus: %i", frame_surplus);
	// if(frame_surplus > 0){
	// 	LOG_DBG("Frame surplus");
	// 	err = ei_wrapper_start_prediction(0, 1);
	// 	if (err) {
	// 		LOG_ERR("Cannot restart prediction (err: %d)\n", err);
	// 	} else {
	// 		LOG_INF("Prediction restarted...\n");
	// 	}

	// 	frame_surplus--;
	// }
	bool cancelled;
	LOG_INF("Clearing data");
	err = ei_wrapper_clear_data(&cancelled);
	LOG_DBG("Data cleared: %i, error: %i", cancelled, err);
}
void init_ei_go(void){
	

	
	int err = ei_wrapper_init(result_ready_cb);

	k_sleep(K_SECONDS(2));

	LOG_INF("Frame size: %i", ei_wrapper_get_frame_size());

	if (err) {
		LOG_ERR("Edge Impulse wrapper failed to initialize (err: %d)\n",
		       err);
		
	}


	// /* input_data is defined in input_data.h file. */
	// err = ei_wrapper_add_data(&input_data[0],
	// 			  ei_wrapper_get_window_size());
	// if (err) {
	// 	LOG_ERR("Cannot provide input data (err: %d)\n", err);
	// 	LOG_ERR("Increase CONFIG_EI_WRAPPER_DATA_BUF_SIZE\n");
	// 	return;
	// }
	


}


K_THREAD_DEFINE(bleT, 4096, init_wp_ble, NULL, NULL, NULL, 6, 0, 0);
K_THREAD_DEFINE(eiT, 4096, init_ei_go, NULL, NULL, NULL, 7, 0, 0);
K_THREAD_DEFINE(ble_write_thread_id, STACKSIZE, ble_write_thread, &woodpecker, NULL,
		NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(micS, 1024, mic, &input_data, NULL,
		NULL, 8, 0, 0);


void main(void)
{
	// input_data[100] = 100;
	LOG_INF("main()");
	while(true){
		LOG_INF("input_data[0]=%f", input_data[100]);
		k_sleep(K_SECONDS(10));
	}
	
}
