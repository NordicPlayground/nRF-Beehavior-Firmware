/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <ei_wrapper.h>
#include <logging/log.h>

#include "input_data.h"
#include "ble.h"

LOG_MODULE_REGISTER(MAIN);

#define FRAME_ADD_INTERVAL_MS	100


static size_t frame_surplus;


static void result_ready_cb(int err)
{
	if (err) {
		LOG_ERR("Result ready callback returned error (err: %d)\n", err);
		return;
	}

	const char *label;
	float value;
	float anomaly;

	LOG_INF("\nClassification results\n");
	LOG_INF("======================\n");

	while (true) {
		err = ei_wrapper_get_next_classification_result(&label, &value, NULL);

		if (err) {
			if (err == -ENOENT) {
				err = 0;
				
			}
			else{
			LOG_ERR("Edge Impulse wrapper failed to initialize (err: %d)\n",
		       err);
			   break;
			}
		}
		
		
		LOG_INF("Value: %.2f\tLabel: %s\n", value, label);
		
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

	if (frame_surplus > 0) {
		err = ei_wrapper_start_prediction(0, 1);
		if (err) {
			LOG_ERR("Cannot restart prediction (err: %d)\n", err);
		} else {
			LOG_INF("Prediction restarted...\n");
		}

		frame_surplus--;
	}
}

K_THREAD_DEFINE(bleconn, 2048, init_wp_ble, NULL, NULL, NULL, 7, 0, 0);

void main(void)
{


	LOG_ERR("Edge Impulse wrapper failed to initialize");

	
	int err = ei_wrapper_init(result_ready_cb);

	if (err) {
		LOG_ERR("Edge Impulse wrapper failed to initialize (err: %d)\n",
		       err);
		return;
	};

	if (ARRAY_SIZE(input_data) < ei_wrapper_get_window_size()) {
		LOG_ERR("Not enough input data\n");
		return;
	}

	if (ARRAY_SIZE(input_data) % ei_wrapper_get_frame_size() != 0) {
		LOG_ERR("Improper number of input samples\n");
		return;
	}

	LOG_INF("Machine learning model sampling frequency: %zu\n",
	       ei_wrapper_get_classifier_frequency());
	LOG_INF("Labels assigned by the model:\n");
	for (size_t i = 0; i < ei_wrapper_get_classifier_label_count(); i++) {
		LOG_DBG("- %s\n", ei_wrapper_get_classifier_label(i));
	}
	printk("\n");

	size_t cnt = 0;

	/* input_data is defined in input_data.h file. */
	err = ei_wrapper_add_data(&input_data[cnt],
				  ei_wrapper_get_window_size());
	if (err) {
		LOG_ERR("Cannot provide input data (err: %d)\n", err);
		LOG_ERR("Increase CONFIG_EI_WRAPPER_DATA_BUF_SIZE\n");
		return;
	}
	cnt += ei_wrapper_get_window_size();

	err = ei_wrapper_start_prediction(0, 0);
	if (err) {
		LOG_ERR("Cannot start prediction (err: %d)\n", err);
	} else {
		LOG_INF("Prediction started...\n");
	}

	/* Predictions for additional data are triggered in the result ready
	 * callback. The prediction start can be triggered before the input
	 * data is provided. In that case the prediction is started right
	 * after the prediction window is filled with data.
	 */
	frame_surplus = (ARRAY_SIZE(input_data) - ei_wrapper_get_window_size())
			/ ei_wrapper_get_frame_size();

	while (cnt < ARRAY_SIZE(input_data)) {
		err = ei_wrapper_add_data(&input_data[cnt],
					  ei_wrapper_get_frame_size());
		if (err) {
			LOG_ERR("Cannot provide input data (err: %d)\n", err);
			LOG_ERR("Increase CONFIG_EI_WRAPPER_DATA_BUF_SIZE\n");
			return;
		}
		cnt += ei_wrapper_get_frame_size();


		k_sleep(K_MSEC(FRAME_ADD_INTERVAL_MS));
	}
}