#include <zephyr.h>
#include <time.h>

#include <date_time.h>
#include <settings/settings.h>

#include <logging/log.h>

#include <modem/lte_lc.h>
#include <modem/modem_info.h>
#include <nrf_modem_at.h>
#include <string.h>




#define CONFIG_APP_CONNECT_RETRY_DELAY 2
#define MAIN MORTEN

enum nrf_modem_mode_t {NORMAL_MODE,FULL_DFU_MODE};

LOG_MODULE_REGISTER(MAIN, 4);

static K_SEM_DEFINE(lte_connected, 0, 1);
static K_SEM_DEFINE(date_time_ok, 0, 1);

int64_t time_container;
char time_buf[64];

static int init(void)
{
	int err;
	int modem_lib_init_result;

	

	/* Init modem */
	modem_lib_init_result = nrf_modem_lib_init(NORMAL_MODE);
	if (modem_lib_init_result) {
		LOG_ERR("Failed to initialize modem library: 0x%X", modem_lib_init_result);
		return -EFAULT;
	}
return 0;
}

static void modem_time_wait(void)
{
	int err;

//	int64_t timeNow;



	

	LOG_INF("Waiting for modem to acquire network time...");

	do {
		k_sleep(K_SECONDS(3));

		err = nrf_modem_at_cmd(time_buf, sizeof(time_buf), "AT%%CCLK?");
		if (err) {
			LOG_DBG("AT Clock Command Error %d... Retrying in 3 seconds.", err);
		}
	} while (err != 0);
	LOG_INF("Network time obtained");

}
static void lte_handler(const struct lte_lc_evt *const evt)
{
	switch (evt->type) {
	case LTE_LC_EVT_NW_REG_STATUS:
		LOG_DBG("LTE_LC_EVT_NW_REG_STATUS: %d", evt->nw_reg_status);
		if ((evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_HOME) &&
		     (evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_ROAMING)) {
			break;
		}

		LOG_DBG("Network registration status: %s",
			evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME ?
			"Connected - home network" : "Connected - roaming");
		k_sem_give(&lte_connected);
		break;
	case LTE_LC_EVT_CELL_UPDATE:
		LOG_DBG("LTE cell changed: Cell ID: %d, Tracking area: %d",
			evt->cell.id, evt->cell.tac);
		break;
	default:
		break;
	}
}
static int connect_to_lte(void)
{
	int err;

	LOG_INF("Waiting for network...");

	k_sem_reset(&lte_connected);

	err = lte_lc_init_and_connect_async(lte_handler);
	if (err) {
		LOG_ERR("Failed to init modem, error: %d", err);
		return err;
	}

	k_sem_take(&lte_connected, K_FOREVER);
	LOG_INF("Connected to LTE");
	return 0;

}

static int setup_connection(void)
{
	int err;

	/* Connect to LTE */
	err = connect_to_lte();
	if (err) {
		LOG_ERR("Failed to connect to cellular network: %d", err);
		return err;
	}
	modem_time_wait();

	return 0;
}

static int setup(void)
{
	int err;

	/* Initialize libraries and hardware */
	err = init();
	if (err) {
		LOG_ERR("Initialization failed.");
		return err;
	}

	/* Initiate Connection */
	err = setup_connection();
	if (err) {
		LOG_ERR("Connection set-up failed.");
		return err;
	}

	return 0;

}


void main(void)
{
	int err;

	int64_t unix_time_ms=1;
	// unix_time_ms=&err;
	

	LOG_INF("Get time app");

	err = setup();
	if (err) {
		LOG_ERR("Setup failed, stopping.");
		return;
	}
	

	 

	while (true)
	{
		k_sleep(K_SECONDS(5));
		nrf_modem_at_cmd(time_buf, sizeof(time_buf), "AT%%CCLK?");
		LOG_INF("time now: %s", time_buf);

		date_time_now(&unix_time_ms);

		time_t nowish = unix_time_ms/1000;

		struct tm *ptm = localtime(&nowish);


		LOG_INF("time now2: %lli", unix_time_ms);
	
		


		LOG_INF("time now22: %i", (ptm->tm_hour));
		LOG_INF("time now22: %i", (ptm->tm_min));
		LOG_INF("time nowsek: %i", (ptm->tm_sec));
		LOG_INF("time nowsek: %i", (ptm->tm_isdst));
		int summertime = (int)(time_buf[30])-48;
		LOG_INF("summertimeys: %i", summertime);

		char rawNumberStringH[10];
		char rawNumberStringM[10];
		char leadingZerro[10];
		char finalMinute[10];
		char rawNumberStringHM[20];


		if((ptm->tm_min) < 10){
			strcpy(leadingZerro, "0");
			strcat(leadingZerro, rawNumberStringM);

		}

		itoa((ptm->tm_hour), rawNumberStringH, 10);
		itoa((ptm->tm_min), leadingZerro, 10);

		strcpy(rawNumberStringHM, rawNumberStringH);
		strcat(rawNumberStringHM, leadingZerro);

		


		

		LOG_INF("Int format: %i",atoi(rawNumberStringHM));
		

	}
	
}

