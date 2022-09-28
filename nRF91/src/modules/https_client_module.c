/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <stdlib.h>
#include <zephyr/net/socket.h>
#include <modem/nrf_modem_lib.h>
#include <zephyr/net/tls_credentials.h>
#include <modem/lte_lc.h>
#include <modem/modem_key_mgmt.h>

#include <app_event_manager.h>

#include "events/cloud_event.h"
#include "include/nvs.h"
#include <logging/log.h>

#include "cJSON.h"

#include "events/sms_event.h"

static K_SEM_DEFINE(lte_connected, 0, 1);

LOG_MODULE_REGISTER(https_client_module, CONFIG_LOG_DEFAULT_LEVEL);

#define TELEPHONE_NUMBERS_AMOUNT 5
#define TELEPHONE_ID_START 6

#define KEY_ID 2

#define HTTPS_PORT 443

#define HTTPS_HOSTNAME "inuit.ed.ntnu.no"//""

#define HTTP_HEAD  \
	"GET /number.json?token=jwvfNjYcqXgUrrtYL083SGRtfs6fXwyaQ1IP5Zhm9HQ HTTP/1.1\r\n" \
	"Host: " HTTPS_HOSTNAME "\r\n" \
	"Accept: application/json\r\n" \
	"Connection: close\r\n\r\n" 

#define HTTP_HEAD_LEN (sizeof(HTTP_HEAD) - 1)

#define HTTP_HDR_END "\r\n\r\n"

#define RECV_BUF_SIZE 4096
#define TLS_SEC_TAG 42

static const char send_buf[] = HTTP_HEAD;
static char recv_buf[RECV_BUF_SIZE];


/* Certificate for `example.com` */
static const char cert[] = {
	#include "../cert/telephone.cer"
};

BUILD_ASSERT(sizeof(cert) < KB(4), "Certificate too large");

/* Provision certificate to modem */
int cert_provision(void)
{	

	int err;
	bool exists;
	int mismatch;

	/* It may be sufficient for you application to check whether the correct
	 * certificate is provisioned with a given tag directly using modem_key_mgmt_cmp().
	 * Here, for the sake of the completeness, we check that a certificate exists
	 * before comparing it with what we expect it to be.
	 */
	err = modem_key_mgmt_exists(TLS_SEC_TAG, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN, &exists);
	if (err) {
		LOG_ERR("Failed to check for certificates err %d\n", err);
		return err;
	}

	if (exists) {
		mismatch = modem_key_mgmt_cmp(TLS_SEC_TAG,
					      MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
					      cert, strlen(cert));
		if (!mismatch) {
			LOG_INF("Certificate match\n");
			return 0;
		}

		LOG_INF("Certificate mismatch\n");
		err = modem_key_mgmt_delete(TLS_SEC_TAG, MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN);
		if (err) {
			LOG_ERR("Failed to delete existing certificate, err %d\n", err);
		}
	}

	/*  Provision certificate to the modem */
	err = modem_key_mgmt_write(TLS_SEC_TAG,
				   MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
				   cert, sizeof(cert) - 1);
	if (err) {
		LOG_ERR("Failed to provision certificate, err %d\n", err);
		return err;
	}

	return 0;
}

/* Setup TLS options on a given socket */
int tls_setup(int fd)
{
	int err;
	int verify;

	/* Security tag that we have provisioned the certificate with */
	const sec_tag_t tls_sec_tag[] = {
		TLS_SEC_TAG,
	};

#if defined(CONFIG_SAMPLE_TFM_MBEDTLS)
	err = tls_credential_add(tls_sec_tag[0], TLS_CREDENTIAL_CA_CERTIFICATE, cert, sizeof(cert));
	if (err) {
		return err;
	}
#endif

	/* Set up TLS peer verification */
	enum {
		NONE = 0,
		OPTIONAL = 1,
		REQUIRED = 2,
	};

	verify = REQUIRED;

	err = setsockopt(fd, SOL_TLS, TLS_PEER_VERIFY, &verify, sizeof(verify));
	if (err) {
		LOG_ERR("Failed to setup peer verification, err %d\n", errno);
		return err;
	}

	/* Associate the socket with the security tag
	 * we have provisioned the certificate with.
	*/
	err = setsockopt(fd, SOL_TLS, TLS_SEC_TAG_LIST, tls_sec_tag,
			 sizeof(tls_sec_tag));
	if (err) {
		LOG_ERR("Failed to setup TLS sec tag, err %d\n", errno);
		return err;
	}

	err = setsockopt(fd, SOL_TLS, TLS_HOSTNAME, HTTPS_HOSTNAME, sizeof(HTTPS_HOSTNAME) - 1);
	if (err) {
		LOG_ERR("Failed to setup TLS hostname, err %d\n", errno);
		return err;
	}
	return 0;
}

/* Parsing the json file */
void parse_object(cJSON *root){
	cJSON* number = NULL;
	cJSON* num = NULL;
	cJSON* number_item = NULL;
	cJSON* lastUpdated = NULL;

	int id = 0;
	int rc = 0;
	char *c;
	bool new_number = false;

	number = cJSON_GetObjectItem(root,"number");

		if(number){
			char* numb;
			LOG_INF("%i", cJSON_GetArraySize(number));
			int i = 0;
			while(i < 20){
				char buf[20];

				rc = nvs_read(&nvs, TELEPHONE_ID_START+i, &buf, 20);
				LOG_INF("HERE IS RC: %i.", rc);
				if (rc > 0) {
					LOG_INF("Length of data available: %i", rc);
					LOG_INF("Id: %d, Data: %s, Length: %i\n",
					TELEPHONE_ID_START+i, buf, strlen(buf));
					i++;
				}
				else{
					break;
				}
			}
			LOG_INF("HERE IS SIZE OF array: %i", i);
			if(i != cJSON_GetArraySize(number)){
				for(int j = 0; j < cJSON_GetArraySize(number); j++ ){
					number_item = cJSON_GetArrayItem(number, j);
					numb = cJSON_GetStringValue(number_item);

					LOG_INF("HERE IS NUMB: %s.", numb);
						
					char buf[20];

					
					for(int n = 0; n < i; n++){
						if(strcmp(numb, buf) != 0){
							rc = nvs_read(&nvs, TELEPHONE_ID_START+n, &buf, 20);
							LOG_INF("Found new number: %i", j);
							
							struct sms_event *sms_event = new_sms_event(strlen(numb));

							sms_event->type = NEW_NUMBER;

							memcpy(sms_event->dyndata.data, numb, strlen(numb));

							APP_EVENT_SUBMIT(sms_event);
							
						}
					}

					if(i > cJSON_GetArraySize(number)){
						for(int k = cJSON_GetArraySize(number); k < i; k++)
						nvs_delete(&nvs, TELEPHONE_ID_START+k);
					}
					nvs_delete(&nvs, TELEPHONE_ID_START+j);


					LOG_INF("L available: %i", rc);
					if (strlen(numb)>= 5 && strlen(numb)<= 16) {
						strcpy(buf, numb);
					
						LOG_INF("Id: %d, data: %s off length: %i \n", TELEPHONE_ID_START+j, buf, strlen(buf));
						nvs_write(&nvs, TELEPHONE_ID_START+j, &buf, strlen(buf));
					}
				}	
			}
		}
		else{
			LOG_ERR("Could not extract number from json file!");
		}

	
}


void https_client_fn(void)
{	
	int err;
	char *json_buf;
	char *q;
	size_t off;
	int fd;
	int bytes;
	struct addrinfo *res;
	struct addrinfo hints = {
		.ai_family = AF_INET,
		.ai_socktype = SOCK_STREAM,
	};

	ssize_t ret = 0;
	int rc = 0, cnt = 0, cnt_his = 0;
	char buf[20];
    const struct device *nvs_dev;

    nvs_dev = FLASH_AREA_DEVICE(DEVICE_LABEL);
    if(!device_is_ready(nvs_dev)){
        LOG_ERR("Error: Device not ready\n");
        return; 
    }
    nvs_init(&nvs, nvs_dev->name);

	LOG_INF("HTTPS client sample started\n\r");

#if !defined(CONFIG_SAMPLE_TFM_MBEDTLS)
	/* Provision certificates before connecting to the LTE network */
	LOG_INF("Provision cert\n\r");
	err = cert_provision();
	LOG_INF("Provision cert done\n\r");
	if (err) {
		LOG_ERR("Provision cert failed");
		return;
	}
#endif

	LOG_INF("Waiting for network.. ");
    k_sem_take(&lte_connected, K_FOREVER);
	err = lte_lc_init_and_connect();
	if (err) {
		LOG_ERR("Failed to connect to the LTE network, err %d\n", err);
		return;
	}
	// k_sleep(K_SECONDS(15));
	LOG_INF("OK\n");

	err = getaddrinfo(HTTPS_HOSTNAME, NULL, &hints, &res);
	
	if (err) {
		LOG_ERR("getaddrinfo() failed, err %d\n", errno);
		return;
	}

	((struct sockaddr_in *)res->ai_addr)->sin_port = htons(HTTPS_PORT);

	if (IS_ENABLED(CONFIG_SAMPLE_TFM_MBEDTLS)) {
		fd = socket(AF_INET, SOCK_STREAM | SOCK_NATIVE_TLS, IPPROTO_TLS_1_2);
	
	} else {
		fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TLS_1_2);

	}
	if (fd == -1) {
		LOG_ERR("Failed to open socket!\n");
		goto clean_up;
	}

	/* Setup TLS socket options */
	err = tls_setup(fd);
	if (err) {
		goto clean_up;
	}

	LOG_INF("Connecting to %s\n", HTTPS_HOSTNAME);
	err = connect(fd, res->ai_addr, sizeof(struct sockaddr_in));
	if (err) {
		LOG_ERR("connect() failed, err: %d\n", errno);
		goto clean_up;
	}

	off = 0;
	do {
		bytes = send(fd, &send_buf[off], HTTP_HEAD_LEN - off, 0);
		if (bytes < 0) {
			LOG_ERR("send() failed, err %d\n", errno);
			goto clean_up;
		}
		off += bytes;
	} while (off < HTTP_HEAD_LEN);

	LOG_INF("Sent %d bytes\n", off);


	off = 0;
	do {

		bytes = recv(fd, &recv_buf[off], RECV_BUF_SIZE - off, 
		0);

		if (bytes < 0) {
			LOG_ERR("recv() failed, err %d\n", errno);
			goto clean_up;
		}
		off += bytes;
	} while (bytes != 0 /* peer closed connection */);

	LOG_INF("Received %d bytes\n", off);
	LOG_INF("Received GET:\n==============\n%s\n============\n", recv_buf);


	/* Make sure recv_buf is NULL terminated (for safe use with strstr) */
	if (off < sizeof(recv_buf)) {
		recv_buf[off] = '\0';
	} else {
		recv_buf[sizeof(recv_buf) - 1] = '\0';
	}
	
	/* Print HTTP response */
	json_buf = strstr(recv_buf, "\r\n\r\n");
	if (json_buf) {
		json_buf +=4;
	}

	LOG_INF("\n>\t %s\n\n", json_buf);
	LOG_INF("Finished, closing socket.\n");

	char *json_string = json_buf;
	cJSON *root = cJSON_Parse(json_string);
	LOG_INF("Parsed");
	parse_object(root);
	// LOG_INF("REAL EYES REALIZE REAL LIES: %i", cJSON_GetArraySize(request_json));



	// char *s;
	// LOG_INF("TELEPHONEEEEEEEEEEE");
	// s = strstr(recv_buf, parameter);   
	// LOG_INF("%s",s);

	// if (s) {
	// 	LOG_INF("TELEPHONEEEEEEEEEEE: %i, %i", strlen(s), sizeof(parameter));
	// 	char temp_buf[1024];
	// 	LOG_INF("%i", sizeof(temp_buf));
	
	// 	memcpy(&temp_buf, s + sizeof(parameter) +1, 30);
	// 	LOG_INF("TELEPHONEEEEEEEEEEE");

	// 	const char *PATTERN1 = ":\"";
	// 	const char *PATTERN2 = "\"";

	// 	char *target = NULL;
   	// 	char *start, *end;

	// 	if ( start = strstr( temp_buf, PATTERN1 ) )
	// 	{
	// 		start += strlen( PATTERN1 );
	// 		if ( end = strstr( start, PATTERN2 ) )
	// 		{
	// 			target = ( char * )malloc( end - start + 1 );
	// 			memcpy( target, start, end - start );
	// 			LOG_INF("TELEPHONEEEEEEEEEEE");
	// 			target[end - start] = '\0';
	// 		}
	// 	}

	// 	if ( target ) LOG_INF( "\n>\t %s\n", target );
		
	// 	int numbers = 0;

	// 	LOG_INF("-----------------\n");
	// 	LOG_INF("NVS SIMPLE SAMPLE\n");
	// 	LOG_INF("-----------------\n");		

	// 	bool new_number = true;
	// 	for(int i=0;i<50;i++){

	// 		rc = nvs_read(&nvs, TELEPHONE_ID_START+i, &buf, strlen(buf));

	// 		if (rc > 0) {
	// 			LOG_INF("Length of data available: %i", rc);
	// 			numbers++;
	// 			/* item was found, show it */
	// 			LOG_INF("Id: %d, Data: %.*s\n",
	// 			TELEPHONE_ID_START+i, rc, buf);
	// 		}
	// 		if(p = strstr(buf, target)){
	// 			LOG_INF("Number exists already: %i", i);
	// 			new_number = false;
	// 			break;
	// 		}
	// 	}
	// 	if(new_number){

	// 		strcpy(buf, target);
	// 		if(strlen(buf)>= 5 && strlen(buf)<= 16){
	// 			LOG_INF("Id: %d not found, adding data: %s off length: %i \n", TELEPHONE_ID_START+numbers, buf, strlen(buf));
	// 			nvs_write(&nvs, TELEPHONE_ID_START+numbers, &buf, strlen(buf));
	// 		}
	// 		else{
    //         LOG_WRN("The following number is either too short or too long: %s. It will not be added.", buf);
	// 		}
	// 	}	
		
	// 	free(target);

	// }
	// else
	// {
	// 	LOG_INF("String not found\n");  
	// }
	// while(true){
	// 	k_sleep(K_SECONDS(15));
	// }

clean_up:
	freeaddrinfo(res);
	(void)close(fd);

}

static bool event_handler(const struct app_event_header *eh)
{
    if(is_cloud_event_abbr(eh)){
        struct cloud_event_abbr *event = cast_cloud_event_abbr(eh);
        if(event->type==LTE_CONNECTED){
            LOG_DBG("LTE is connected? That's coolio.");
            //sms_setup_fn();
            k_sem_give(&lte_connected);
            return false;
        }
        return false;
    }
	if(is_sms_event(eh)){
        int err;
        struct sms_event *event = cast_sms_event(eh);
        if(event->type==NUMBER_STATUS){
			char message[200];
			LOG_INF("Length of number: %i", event->dyndata.size);
			int len = snprintk(message, 200, "POST /send HTTP/1.1\r\n"                                                  \
				"Host: inuit.ed.ntnu.no\r\n"                                     \
				"Content-Type: application/json\r\n" \
				"Content-Length: %i\r\n\r\n"\
				"{\"phoneNumbers\":{\"PhoneNumber\":\"+%.*s\", \"setSubscriberStatus\":1}}\r\n\r\n", 61 + event->dyndata.size, event->dyndata.size, event->dyndata.data); //, event->subScribe
			LOG_INF("%i\r\n%.*s", len, len, message);
			int off = 0;
			int fd;
			int bytes;
			struct addrinfo *res;
			
			struct addrinfo hints = {
				.ai_family = AF_INET,
				.ai_socktype = SOCK_STREAM,
			};

			
			err = getaddrinfo(HTTPS_HOSTNAME, NULL, &hints, &res);
			
			if (err) {
				LOG_ERR("getaddrinfo() failed, err %d\n", errno);
				return;
			}

			((struct sockaddr_in *)res->ai_addr)->sin_port = htons(HTTPS_PORT);

			if (IS_ENABLED(CONFIG_SAMPLE_TFM_MBEDTLS)) {
				fd = socket(AF_INET, SOCK_STREAM | SOCK_NATIVE_TLS, IPPROTO_TLS_1_2);
			
			} else {
				fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TLS_1_2);

			}
			if (fd == -1) {
				LOG_ERR("Failed to open socket!\n");
				// goto clean_up;
			}

			/* Setup TLS socket options */
			err = tls_setup(fd);
			if (err) {
				// goto clean_up;
			}


			LOG_INF("Connecting LOOOOOOOOOOL to %s\n", HTTPS_HOSTNAME);
			err = connect(fd, res->ai_addr, sizeof(struct sockaddr_in));
			do {
				bytes = send(fd, &message[off], len - off, 0);
				if (bytes < 0) {
					LOG_ERR("send() failed, err %d\n", errno);
					// goto clean_up;
				}
				off += bytes;
			} while (off < len);

			LOG_INF("Sent %d bytes\n", off);

			// off = 0;
			// do {
			// 	bytes = send(fd, &send_post[off], HTTP_HEAD_LEN_POST - off, 0);
			// 	if (bytes < 0) {
			// 		LOG_ERR("send_post() failed, err %d\n", errno);
			// 		goto clean_up;
			// 	}
			// 	off += bytes;
			// } while (off < HTTP_HEAD_LEN_POST);

			// LOG_INF("Sent message %d bytes\n", off);
			// LOG_INF("Sent message:\n====start=====\n\n %s \n\n====end=====", send_post);


			off = 0;
			do {

				bytes = recv(fd, &recv_buf[off], RECV_BUF_SIZE - off, 
				0);

				if (bytes < 0) {
					LOG_ERR("recv() failed, err %d\n", errno);
					// goto clean_up;
				}
				off += bytes;
			} while (bytes != 0 /* peer closed connection */);

			LOG_INF("Received %d bytes\n", off);
			LOG_INF("Received POST:\n==============\n%s\n============\n", recv_buf);

			freeaddrinfo(res);
			(void)close(fd);
		}
	}
	return false;
}

K_THREAD_DEFINE(https_client, RECV_BUF_SIZE,
		https_client_fn, NULL, NULL, NULL,
		K_HIGHEST_APPLICATION_THREAD_PRIO, 0, 0);

APP_EVENT_LISTENER(https_module, event_handler);
APP_EVENT_SUBSCRIBE_EARLY(https_module, cloud_event_abbr);
APP_EVENT_SUBSCRIBE_EARLY(https_module, sms_event);