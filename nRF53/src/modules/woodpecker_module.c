/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#define MODULE app_module

#include <zephyr.h>
#include <stdio.h>
#include <app_event_manager.h>
#include <drivers/gpio.h>
#include <ei_wrapper.h>
#include <audio/dmic.h>

#include <zephyr/settings/settings.h>
#include "events/woodpecker_event.h"
#include "battery/battery.h"

#define SEND_INTERVAL CONFIG_WOODPECKER_SAMPLE_RATE

LOG_MODULE_REGISTER(WOODPECKER, CONFIG_LOG_DEFAULT_LEVEL);

static K_SEM_DEFINE(dout_triggered, 0, 1);

static struct k_work_delayable send_interval;

#define FRAME_ADD_INTERVAL_MS	100

static size_t frame_surplus;

uint8_t positive_triggers = 0;
uint16_t total_triggers = 0;
float highest_probability = 0;

//Used for conversion from voltage to percent
static const struct battery_level_point levels[] = {
#if DT_NODE_HAS_PROP(DT_INST(0, voltage_divider), io_channels)
	/* "Curve" here eyeballed from captured data for the [Adafruit
	 * 3.7v 2000 mAh](https://www.adafruit.com/product/2011) LIPO
	 * under full load that started with a charge of 3.96 V and
	 * dropped about linearly to 3.58 V over 15 hours.  It then
	 * dropped rapidly to 3.10 V over one hour, at which point it
	 * stopped transmitting.
	 *
	 * Based on eyeball comparisons we'll say that 15/16 of life
	 * goes between 3.95 and 3.55 V, and 1/16 goes between 3.55 V
	 * and 3.1 V.
	 */

	{ 10000, 4100 },
	{ 625, 3550 },
	{ 0, 3100 },
#else
	/* Linear from maximum voltage to minimum voltage. */
	{ 10000, 3600 },
	{ 0, 1700 },
#endif
};

/* Edge Impulse results ready. */
static void result_ready_cb(int err)
{
	if(err){
		LOG_ERR("Result ready callback error: %d", err);
		return;
	}

	const char *label;
	float value;
	size_t inx;

	while(true){
		err = ei_wrapper_get_next_classification_result(&label, &value, &inx);

		if(err){
			LOG_ERR("Unable to get next classification result: %d", err);
			break;
		}
		if(inx == 2){
            total_triggers++;
			
			if(value>0.5){
                positive_triggers++;
            }

            if(value>highest_probability){
                highest_probability = value;
            }
		}
	}
	if(!err){
		if(ei_wrapper_classifier_has_anomaly()){
			float anomaly;
			err = ei_wrapper_get_anomaly(&anomaly);
		}
	}

	bool cancelled;
	err = ei_wrapper_clear_data(&cancelled);
	if(err){
		LOG_ERR("Unable to clear data: %i", err);
	}
}

/* Function for sending data periodically. */
static void send_data_fn(struct k_work *work)
{
	int batt_mV = battery_sample();
	if (batt_mV < 0) {
		LOG_ERR("Failed to read battery voltage: %d\n",
			batt_mV);
	}

	unsigned int batt_pptt = battery_level_pptt(batt_mV, levels);
	LOG_INF("Battery percentage: %i", batt_pptt/100);
	
	/* Send woodpecker data to the peripheral. */
	struct woodpecker_event *data_ready = new_woodpecker_event();

	data_ready->positive_triggers = positive_triggers;
	data_ready->total_triggers = total_triggers;
	data_ready->highest_probability = (uint16_t)(highest_probability*100);
	data_ready->bat_percentage = (uint8_t)(batt_pptt/100);
	
	APP_EVENT_SUBMIT(data_ready);

	/* Clear the data. */
    positive_triggers = 0;
    total_triggers = 0;
    highest_probability = 0;

	/* Schedule next send. */
	k_work_reschedule(&send_interval, K_MINUTES(SEND_INTERVAL));
}

#define MAX_SAMPLE_RATE  16000

#define SAMPLE_BIT_WIDTH 16
#define BYTES_PER_SAMPLE sizeof(int16_t)
/* Milliseconds to wait for a block to be read. */
#define READ_TIMEOUT     1000

/* Size of a block for 100 ms of audio data. */
#define BLOCK_SIZE(_sample_rate, _number_of_channels) \
	(BYTES_PER_SAMPLE * (_sample_rate / 10) * _number_of_channels)

/* Driver will allocate blocks from this slab to receive audio data into them.
 * Application, after getting a given block from the driver and processing its
 * data, needs to free that block. */
#define MAX_BLOCK_SIZE   BLOCK_SIZE(MAX_SAMPLE_RATE, 2)
#define BLOCK_COUNT      4

K_MEM_SLAB_DEFINE_STATIC(mem_slab, MAX_BLOCK_SIZE, BLOCK_COUNT, 4);
float audio[16000];

static int do_pdm_transfer(const struct device *dmic_dev,
			   struct dmic_cfg *cfg,
			   size_t block_count)
{
	int ret;

	/* Start the microphone. */
	ret = dmic_trigger(dmic_dev, DMIC_TRIGGER_START);
	if (ret < 0) {
		LOG_ERR("START trigger failed: %d", ret);
		return ret;
	}

	for (int i = 0; i < 11; ++i) {
		void *buffer;
		uint32_t size;

		ret = dmic_read(dmic_dev, 0, &buffer, &size, READ_TIMEOUT);
		if (ret < 0) {
			LOG__ERR("read failed: %d", ret);
			return ret;
		}

		/* Discard first readout due to microphone needing to 
		 * stabilize before valid data can be read. */
		if(i!=0){
			int16_t tempInt;
			float tempFloat;
			for(int j=0; j<1600; j++){
				memcpy(&tempInt, buffer + 2*j, 2);
				tempFloat = (float)tempInt;
				audio[(i-1)*1600+j] = tempFloat;
			}
		}
		k_mem_slab_free(&mem_slab, &buffer);
	}

	/* Stop the microphone. */
	ret = dmic_trigger(dmic_dev, DMIC_TRIGGER_STOP);
	if (ret < 0) {
		LOG__ERR("STOP trigger failed: %d", ret);
		return ret;
	}

	/* Give the microphone data to Edge Impulse. */
	ret = ei_wrapper_add_data(&audio, ei_wrapper_get_window_size());
	if (ret) {
		LOG__ERR("Cannot provide input data (err: %d)\n", ret);
		LOG__ERR("Increase CONFIG_EI_WRAPPER_DATA_BUF_SIZE\n");
	}
	ei_wrapper_start_prediction(0,0);

	return 0;
}

void mic_thread_fn()
{
	int ret;
	ret = ei_wrapper_init(result_ready_cb);
    
	k_work_init_delayable(&send_interval, send_data_fn);
    k_work_reschedule(&send_interval, K_MINUTES(SEND_INTERVAL));

	ret = battery_measure_enable(true);
	if(ret){
		LOG_ERR("Battery measure enable error: %i", ret);
	}
	
    k_sleep(K_SECONDS(10));
	const struct device *gpio_dev;
	gpio_dev = device_get_binding("GPIO_0");
	ret = gpio_pin_configure(gpio_dev, 31, GPIO_OUTPUT_HIGH);
	k_sleep(K_SECONDS(5));
	
	const struct device *dmic_dev = DEVICE_DT_GET(DT_NODELABEL(dmic_dev));

	LOG__INF("DMIC module");

	if (!device_is_ready(dmic_dev)) {
		LOG__ERR("%s is not ready", dmic_dev->name);
		return;
	}

	/* Configuration of the microphone. */
	struct pcm_stream_cfg stream = {
		.pcm_width = SAMPLE_BIT_WIDTH,
		.mem_slab  = &mem_slab,
	};
	struct dmic_cfg cfg = {
		.io = {
			/* These fields can be used to limit the PDM clock
			 * configurations that the driver is allowed to use
			 * to those supported by the microphone.
			 */
			.min_pdm_clk_freq = 1100000,
			.max_pdm_clk_freq = 3500000,
			.min_pdm_clk_dc   = 40,
			.max_pdm_clk_dc   = 60,
		},
		.streams = &stream,
		.channel = {
			.req_num_streams = 1,
		},
	};

	cfg.channel.req_num_chan = 1;
	cfg.channel.req_chan_map_lo =
		dmic_build_channel_map(0, 0, PDM_CHAN_LEFT);
	cfg.streams[0].pcm_rate = MAX_SAMPLE_RATE;
	cfg.streams[0].block_size =
		BLOCK_SIZE(cfg.streams[0].pcm_rate, cfg.channel.req_num_chan);

	ret = dmic_configure(dmic_dev, &cfg);
	if (ret < 0) {
		return;
	}

	/* Configure the DOUT pin as an input.
	 * So that we can wait for it to go high to sample the microphone. */
	ret = gpio_pin_configure(gpio_dev, 25,
				GPIO_INPUT | GPIO_PULL_DOWN | GPIO_ACTIVE_HIGH);

	while(true){
		/* Check if the noise have exceeded the threshold. */
		if(gpio_pin_get_raw(gpio_dev, 25)){
			ret = do_pdm_transfer(dmic_dev, &cfg, 2 * BLOCK_COUNT);
			if (ret < 0) {
				return;
			}
			k_sleep(K_SECONDS(15));
		}
		else{
			k_sleep(K_SECONDS(1));
		}
	}
}

K_THREAD_DEFINE(mic_thread, 1024, mic_thread_fn, NULL, NULL,
		NULL, 8, 0, 0);