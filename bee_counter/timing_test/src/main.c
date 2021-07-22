/* 
Dank code for the dank people @ nordicsemi
*/

#include <zephyr.h>
#include <sys/printk.h>
#include <timing/timing.h>
#include <stdio.h>

timing_t start_time, end_time;
unsigned long milliseconds_spent;
unsigned long two_seconds;

unsigned long current_time(timing_t *start_time, timing_t *end_time){ 
    *end_time = timing_counter_get(); 
	unsigned long current_time_ns = timing_cycles_to_ns(timing_cycles_get(start_time, end_time)) * 0.000001; //Converts to ms
	return current_time_ns; 
}

void main(void){
	timing_init();
	timing_start();
	while(1){
		start_time = timing_counter_get();
		k_busy_wait(1000000);
		milliseconds_spent = current_time(&start_time, &end_time);
		k_busy_wait(1000000);
		two_seconds = current_time(&start_time, &end_time);
		printf("Milliseconds spent: %ld \n", milliseconds_spent);
		printf("Two seconds: %ld \n", two_seconds);
	}
}

