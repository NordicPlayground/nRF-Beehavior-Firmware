
#include "mic.h"
#include <ei_wrapper.h>
#include "edge-impulse-sdk/classifier/ei_run_classifier.h"

LOG_MODULE_REGISTER(dmic_sample);

uint16_t actual_sample_rate = 0;
float audio[16000];
uint16_t audio_16[16000];

static int do_pdm_transfer(const struct device *dmic_dev,
			   struct dmic_cfg *cfg,
			   size_t block_count, float *input_data)
{
	int ret;

	// k_sleep(K_SECONDS(5));


// for(;;){

	// k_sleep(K_SECONDS(2));
	ret = dmic_trigger(dmic_dev, DMIC_TRIGGER_START);
	if (ret < 0) {
		LOG_ERR("START trigger failed: %d", ret);
		return ret;
	}

	//Need to wait atleast 100 us before the data on DOUT is valid
	k_sleep(K_USEC(200));

	for (int i = 0; i < 10; ++i) {
		void *buffer;
		uint32_t size;
	// 	int ret;

	// 	void *this_buffer = buffer + 3200*i;

		ret = dmic_read(dmic_dev, 0, &buffer, &size, READ_TIMEOUT);
		if (ret < 0) {
			LOG_ERR("read failed: %d", ret);
			return ret;
		}

		LOG_INF("%i - got buffer %p of %u bytes", i, buffer, size);

		// // k_sleep(K_SECONDS(1));



		int16_t tempInt;
		float tempFloat;
		for(int j=0; j<1600; j++){
			memcpy(&tempInt, buffer + 2*j, 2);
			tempFloat = (float)tempInt;
			audio[i*1600+j] = tempFloat;
			// memcpy(&audio[i*1600+j-15], &tempFloat, sizeof(tempFloat));
			// LOG_INF("%i", byte);
			// k_sleep(K_MSEC(2));
		}
		// LOG_INF("Copying to address: %x, in array %x", &audio_16[i*size/2], &audio_16);
		// memcpy(&audio_16[i*size/2], buffer, size);
		// LOG_INF("Size of float: %u", sizeof(float));
		// LOG_INF("%i - got buffer %p of %u bytes", i, buffer, size);

		k_mem_slab_free(&mem_slab, &buffer);
		// LOG_INF("%i - got buffer %p of %u bytes", i, buffer, size);
	}

	ret = dmic_trigger(dmic_dev, DMIC_TRIGGER_STOP);
	if (ret < 0) {
		LOG_ERR("STOP trigger failed: %d", ret);
		return ret;
	}

	// for(int i = 0; i < sizeof(audio); i++){
	// 	LOG_INF("%u", audio_16[i]);
	// 	k_sleep(K_MSEC(2));
	// }

	memcpy(input_data, &audio, 15984*sizeof(float));

	// // LOG_INF("I am here, lol %i", ei_wrapper_get_window_size());

	int err;
		/* input_data is defined in input_data.h file. */
	LOG_INF("Input_data[0]: %f", input_data[100]);
	err = ei_wrapper_add_data(&input_data[0],
				  ei_wrapper_get_window_size());
	LOG_INF("Input_data[0]: %f", input_data[100]);
	if (err) {
		LOG_ERR("Cannot provide input data (err: %d)\n", err);
		LOG_ERR("Increase CONFIG_EI_WRAPPER_DATA_BUF_SIZE\n");
	}
	ei_wrapper_start_prediction(0,0);


	

	// return ret;

	// }

	return 0;
}

void mic(float *input_data)
{
	
	const struct device *dmic_dev = DEVICE_DT_GET(DT_NODELABEL(dmic_dev));
	int ret;

	LOG_INF("DMIC sample");

	k_sleep(K_SECONDS(10));

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


	LOG_INF("PCM output rate: %u, channels: %u",
		cfg.streams[0].pcm_rate, cfg.channel.req_num_chan);

	ret = dmic_configure(dmic_dev, &cfg);
	if (ret < 0) {
		LOG_ERR("Failed to configure the driver: %d", ret);
		return;
	}

	while(true){
		ret = do_pdm_transfer(dmic_dev, &cfg, 2 * BLOCK_COUNT, input_data);
		if (ret < 0) {
			return;
		}
		k_sleep(K_SECONDS(15));
	}
}
