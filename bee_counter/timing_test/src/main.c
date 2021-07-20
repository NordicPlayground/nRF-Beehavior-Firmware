/* 
Dank code for the dank people @ nordicsemi
*/

#include <zephyr.h>
#include <sys/printk.h>
#include <timing/timing.h>
#include <device.h>

//Lag denne timer greia ferdig, litt stress men ok


struct timer{
	uint64_t start_time;
	uint64_t current_time;
	int time_in_ns;
};

void main(void){
		timing_init();
    	timing_start();
		start_time = timing_counter_get();
		printk("Time after initilization: %lld\n", start_time);
		k_busy_wait(1000000);
		current_time = timing_counter_get();
		real_end_time = timing_cycles_to_ns(timing_cycles_get(&start_time, &current_time));
		printk("Time after 1 second: %d\n", real_end_time);
		k_busy_wait(1000000);
		current_time = timing_counter_get();
		real_end_time = timing_cycles_to_ns(timing_cycles_get(&start_time, &current_time));
		printk("Time after 2 seconds: %d\n", real_end_time);
}

