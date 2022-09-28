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
#include <modem/sms.h>
#include "events/sms_event.h"

#include <app_event_manager.h>

#define HTTPS_PORT 443

#define HTTPS_HOSTNAME "inuit.ed.ntnu.no"

#define HTTP_HEAD                                                              \
	"POST /send HTTP/1.1\r\n"                                                  \
	"Host: " HTTPS_HOSTNAME "\r\n"                                     \
	"Content-Type: application/json\r\n" \
	"Content-Length: 46\r\n\r\n" 
	// "Content-Length: 46\r\n\r\n" \
	// "{\"phoneNumbers\":{\"PhoneNumber\":\"+4747448146\"}} \r\n"                                                

#define HTTP_HEAD_LEN (sizeof(HTTP_HEAD) - 1)

#define HTTP_HDR_END "\r\n\r\n"

#define RECV_BUF_SIZE 2048
#define TLS_SEC_TAG 42

LOG_MODULE_REGISTER(post_module, CONFIG_LOG_DEFAULT_LEVEL);



static const char send_buf[] = HTTP_HEAD;
//static const char send_buf[] = "{\"phoneNumbers\":{\"PhoneNumber\":\"+4790234817\"}}";
static char recv_buf[RECV_BUF_SIZE];

/* Certificate for `example.com` */
static const char cert[] = {
	#include "../cert/telephone.cer"
};

BUILD_ASSERT(sizeof(cert) < KB(4), "Certificate too large");

/* Provision certificate to modem */
int cert_provision_two(void)
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
		LOG_INF("Failed to check for certificates err %d\n", err);
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
			LOG_INF("Failed to delete existing certificate, err %d\n", err);
		}
	}

	LOG_INF("Provisioning certificate\n");

	/*  Provision certificate to the modem */
	err = modem_key_mgmt_write(TLS_SEC_TAG,
				   MODEM_KEY_MGMT_CRED_TYPE_CA_CHAIN,
				   cert, sizeof(cert) - 1);
	if (err) {
		LOG_INF("Failed to provision certificate, err %d\n", err);
		return err;
	}

	return 0;
}

/* Setup TLS options on a given socket */
int tls_setup_part_two(int fd)
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
		LOG_INF("Failed to setup peer verification, err %d\n", errno);
		return err;
	}

	/* Associate the socket with the security tag
	 * we have provisioned the certificate with.
	 */
	err = setsockopt(fd, SOL_TLS, TLS_SEC_TAG_LIST, tls_sec_tag,
			 sizeof(tls_sec_tag));
	if (err) {
		LOG_INF("Failed to setup TLS sec tag, err %d\n", errno);
		return err;
	}

	err = setsockopt(fd, SOL_TLS, TLS_HOSTNAME, HTTPS_HOSTNAME, sizeof(HTTPS_HOSTNAME) - 1);
	if (err) {
		LOG_INF("Failed to setup TLS hostname, err %d\n", errno);
		return err;
	}
	return 0;
}

static void add_function(char * buffer, int len)
{
	int err;
	int fd;
	char *p;
	int bytes;
	size_t off;
	struct addrinfo *res;
	struct addrinfo hints = {
		.ai_family = AF_INET,
		.ai_socktype = SOCK_STREAM,
	};

	LOG_INF("HTTPS client sample started\n\r");

#if !defined(CONFIG_SAMPLE_TFM_MBEDTLS)
	/* Provision certificates before connecting to the LTE network */
	err = cert_provision_two();
	if (err) {
		return;
	}
#endif
	err = getaddrinfo(HTTPS_HOSTNAME, NULL, &hints, &res);
	if (err) {
		LOG_INF("getaddrinfo() failed, err %d\n", errno);
		return;
	}

	((struct sockaddr_in *)res->ai_addr)->sin_port = htons(HTTPS_PORT);

	if (IS_ENABLED(CONFIG_SAMPLE_TFM_MBEDTLS)) {
		fd = socket(AF_INET, SOCK_STREAM | SOCK_NATIVE_TLS, IPPROTO_TLS_1_2);
	} else {
		fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TLS_1_2);
	}
	if (fd == -1) {
		LOG_INF("Failed to open socket!\n");
		goto clean_up;
	}

	/* Setup TLS socket options */
	err = tls_setup_part_two(fd);
	if (err) {
		goto clean_up;
	}

	LOG_INF("Connecting to %s\n", HTTPS_HOSTNAME);
	err = connect(fd, res->ai_addr, sizeof(struct sockaddr_in));
	if (err) {
		LOG_INF("connect() failed, err: %d\n", errno);
		goto clean_up;
	}
	// int total = strlen(send_buf);
	// off = 0;
	// do {
	// 	bytes = send(fd, &send_buf + off, total - off, 0);
	// 	if (bytes < 0) {
	// 		LOG_INF("send() failed, err %d\n", errno);
	// 		goto clean_up;
	// 	}
	// 	off += bytes;
	// } while (off < total);

	// char *s;
	// s = strstr(HTTP_HEAD, "\r\n\r\n");
	off = 0;
	do {
		bytes = send(fd, buffer, len - off, 0);
		if (bytes < 0) {
			LOG_INF("send() failed, err %d\n", errno);
			goto clean_up;
		}
		off += bytes;
	} while (off < len);

	LOG_INF("Sent %d bytes\n", off);
	LOG_INF("=====BEGIN=====\n %s", send_buf);

	off = 0;
	do {
		bytes = recv(fd, &recv_buf[off], RECV_BUF_SIZE - off, 0);
		if (bytes < 0) {
			LOG_INF("recv() failed, err %d\n", errno);
			goto clean_up;
		}
		off += bytes;
	} while (bytes != 0 /* peer closed connection */);

	LOG_INF("Received %d bytes\n", off);

	/* Make sure recv_buf is NULL terminated (for safe use with strstr) */
	if (off < sizeof(recv_buf)) {
		recv_buf[off] = '\0';
	} else {
		recv_buf[sizeof(recv_buf) - 1] = '\0';
	}

	/* Print HTTP response */
	p = strstr(recv_buf, "\r\n");
	if (p) {
		off = p - recv_buf;
		recv_buf[off + 1] = '\0';
		LOG_INF("\n>\t %s\n\n", recv_buf);
	}

	LOG_INF("Finished, closing socket.\n");

clean_up:
	freeaddrinfo(res);
	(void)close(fd);

	lte_lc_power_off();
}
static bool event_handler(const struct app_event_header *eh)
{
    if(is_sms_event(eh)){
        int err;
        struct sms_event *event = cast_sms_event(eh);
        if(event->type==NUMBER_STATUS){
			char message[200];
			LOG_INF("Length of number: %i", event->dyndata.size);
			int len = snprintk(message, 200, "POST /send HTTP/1.1\r\n"                                                  \
	"Host: inuit.ed.ntnu.no\r\n"                                     \
	"Content-Type: application/json\r\n" \
	"Content-Length: 46\r\n"\
	"{\"phoneNumbers\":{\"PhoneNumber\":\"+%.*s\"}}\r\n\r\n", event->dyndata.size, event->dyndata.data);
			LOG_INF("%i\r\n%.*s", len, len, message);
			add_function(message, len);
		}
	}
	return false;
}

APP_EVENT_LISTENER(post_module, event_handler);
APP_EVENT_SUBSCRIBE_EARLY(post_module, sms_event);