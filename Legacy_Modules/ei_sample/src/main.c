/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <mic.h>

LOG_MODULE_REGISTER(MAIN);

/* Edge Impulse results ready. */
static void result_ready_cb(int err)
{
	if(err){
		LOG_INF("Result ready callback error: %d", err);
		return;
	}

	const char *label;
	float value;
	size_t inx;

	while(true){
		err = ei_wrapper_get_next_classification_result(&label, &value, &inx);

		if(err){
			LOG_INF("Unable to get next classification result: %d", err);
			break;
		}
		if(inx == 2){

			LOG_INF("Woodpecker probabilaty: %f", value);
			break;
            
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
		LOG_INF("Unable to clear data: %i", err);
	}
}


static int do_pdm_transfer(const struct device *dmic_dev,
			   struct dmic_cfg *cfg,
			   size_t block_count)
{
	int ret;

	LOG_INF("Starting sampling:");

	/* Start the microphone. */
	ret = dmic_trigger(dmic_dev, DMIC_TRIGGER_START);
	if (ret < 0) {
		LOG_INF("START trigger failed: %d", ret);
		return ret;
	}
	
	for (int i = 0; i < 11; ++i) {
		void *buffer;
		uint32_t size;

		ret = dmic_read(dmic_dev, 0, &buffer, &size, READ_TIMEOUT);
		if (ret < 0) {
			LOG_INF("Read failed: %d", ret);
			break;
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
		return ret;
	}

	/* Give the microphone data to Edge Impulse. */
	ret = ei_wrapper_add_data(&audio, ei_wrapper_get_window_size());
	if (ret) {
		LOG_INF("Cannot provide input data (err: %d)\n", ret);
		LOG_INF("Increase CONFIG_EI_WRAPPER_DATA_BUF_SIZE\n");
	}
	ei_wrapper_start_prediction(0,0);

	return 0;
}




	



	void main()
	{
	int ret;
	ret = ei_wrapper_init(result_ready_cb);

	if(ret){
		LOG_ERR("EI wrapper failed to init: %i", ret);
	}
   
	
    k_sleep(K_SECONDS(15));
	
	const struct device *dmic_dev = DEVICE_DT_GET(DT_NODELABEL(dmic_dev));

	LOG_INF("DMIC module");

	if (!device_is_ready(dmic_dev)) {
		LOG_ERR("%s is not ready", dmic_dev->name);
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


	cfg.channel.req_num_chan = 2;
	cfg.channel.req_chan_map_lo =
		dmic_build_channel_map(0, 0, PDM_CHAN_LEFT) |
		dmic_build_channel_map(1, 0, PDM_CHAN_RIGHT);
	cfg.streams[0].pcm_rate = MAX_SAMPLE_RATE;
	cfg.streams[0].block_size =
		BLOCK_SIZE(cfg.streams[0].pcm_rate, cfg.channel.req_num_chan);

	ret = dmic_configure(dmic_dev, &cfg);
	if (ret < 0) {
		return;
	}


	while(true){


			ret = do_pdm_transfer(dmic_dev, &cfg, 2 * BLOCK_COUNT);
			if (ret < 0) {
				return;
			k_sleep(K_SECONDS(15));
		 }
			k_sleep(K_SECONDS(1));
	}
	}
	
