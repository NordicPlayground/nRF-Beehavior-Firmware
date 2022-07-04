/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mic.h"
#include <ei_wrapper.h>
// 

LOG_MODULE_REGISTER(dmic_sample);

static int do_pdm_transfer(const struct device *dmic_dev,
			   struct dmic_cfg *cfg,
			   size_t block_count, float *input_data)
{
	int ret;

	LOG_INF("PCM output rate: %u, channels: %u",
		cfg->streams[0].pcm_rate, cfg->channel.req_num_chan);

	ret = dmic_configure(dmic_dev, cfg);
	if (ret < 0) {
		LOG_ERR("Failed to configure the driver: %d", ret);
		return ret;
	}


for(;;){
	ret = dmic_trigger(dmic_dev, DMIC_TRIGGER_START);
	if (ret < 0) {
		LOG_ERR("START trigger failed: %d", ret);
		return ret;
	}

	

	for (int i = 0; i < block_count; ++i) {
		void *buffer;
		uint32_t size;
		int ret;
		int stopC;
		//printk("size of buffer: %d", size);

		ret = dmic_read(dmic_dev, 0, &buffer, &size, READ_TIMEOUT);
		if (ret < 0) {
			LOG_ERR("%d - read failed: %d", i, ret);
			return ret;
		}

        
		for(int j = 0; j < size; j++){
			int16_t *int_pnt;
			int_pnt = &buffer + j;
			input_data[(1600*i)+j] = (float) *int_pnt;

			stopC=(1600*i)+j;

			if(stopC == 15984){
				LOG_INF("%i",stopC);
				break;
			}
			
			
		}

			if(stopC == 15984){
				break;
			}

		//LOG_INF("Size of input data: %i", sizeof(input_data));

		//LOG_INF("%d - got buffer %p of %u bytes", i, buffer, size);
		
		k_mem_slab_free(&mem_slab, &buffer);
	}

	ret = dmic_trigger(dmic_dev, DMIC_TRIGGER_STOP);
	if (ret < 0) {
		LOG_ERR("STOP trigger failed: %d", ret);
		return ret;
	}


	LOG_INF("I am here, lol %i", ei_wrapper_get_window_size());

	LOG_INF("Size of input data: %i", sizeof(input_data));
	int err;
		/* input_data is defined in input_data.h file. */
	err = ei_wrapper_add_data(input_data,
				  ei_wrapper_get_window_size());
	if (err) {
		LOG_ERR("Cannot provide input data (err: %d)\n", err);
		LOG_ERR("Increase CONFIG_EI_WRAPPER_DATA_BUF_SIZE\n");
	}
	ei_wrapper_start_prediction(0,0);

	k_sleep(K_SECONDS(2));

	

	// return ret;

	}

	
}

void mic(float *input_data)
{
	
	const struct device *dmic_dev = DEVICE_DT_GET(DT_NODELABEL(dmic_dev));
	int ret;

	LOG_INF("DMIC sample");

	if (!device_is_ready(dmic_dev)) {
		LOG_ERR("%s is not ready", dmic_dev->name);
		return;
	}

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
			.min_pdm_clk_freq = 1000000,
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



	ret = do_pdm_transfer(dmic_dev, &cfg,   BLOCK_COUNT, input_data);
	if (ret < 0) {
		return;
	}

	


}
