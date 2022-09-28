
#include <zephyr/kernel.h>
#include <ei_wrapper.h>
#include <logging/log.h>

#include "ble.h"
#include "mic.h"

LOG_MODULE_REGISTER(MAIN);

#define FRAME_ADD_INTERVAL_MS	100


static size_t frame_surplus;
float woodpecker;

static void result_ready_cb(int err)
{
	if (err) {
		LOG_ERR("Result ready callback returned error (err: %d)\n", err);
		return;
	}

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
	}

	bool cancelled;
	
	err = ei_wrapper_clear_data(&cancelled);
	if(err){
		LOG_ERR("Data cleared: %i, error: %i", cancelled, err);
	}
}


K_THREAD_DEFINE(bleT, 4096, init_wp_ble, NULL, NULL, NULL, 6, 0, 0);
K_THREAD_DEFINE(ble_write_thread_id, STACKSIZE, ble_write_thread, &woodpecker, NULL,
		NULL, PRIORITY, 0, 0);
K_THREAD_DEFINE(micS, 1024, mic, NULL, NULL,
		NULL, 8, 0, 0);


void main(void)
{
	int err = ei_wrapper_init(result_ready_cb);

	if(err) {
		LOG_ERR("Edge Impulse wrapper failed to initialize (err: %d)\n",
		       err);
		
	}
}
