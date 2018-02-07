#include <stdio.h>
#include <stdlib.h>
#include <string.h>  //Requires by memset
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"

#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "tcpip_adapter.h"

#include <lwip/sockets.h>
#include <lwip/sys.h>
#include <lwip/api.h>
#include <lwip/netdb.h>

#define MY_WIFI_SSID	"YourWifiSSID"
#define MY_WIFI_PASS	"YourPassword"
#define STATIC_IP		"192.168.1.222"
#define SUBNET_MASK		"255.255.255.0"
#define GATE_WAY		"192.168.1.1"
#define DNS_SERVER		"8.8.8.8"

#define ENABLE_STATIC_IP

static const char *TAG = "espressif";	//TAG for debug

static EventGroupHandle_t wifi_event_group;

static wifi_config_t wifi_config = { .sta = { .ssid = MY_WIFI_SSID, .password =
		MY_WIFI_PASS, .bssid_set = 0 } };

const int CONNECTED_BIT = BIT0;

int state = 0;
int LED_PIN = 25;
// Build http header
const static char http_html_hdr[] =
		"HTTP/1.1 200 OK\r\nContent-type: text/html\r\n\r\n";

// Build http body
const static char http_index_hml[] =
		"<html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\
		<title>Control</title><style>body{background-color:lightblue;font-size:24px;}</style></head>\
		<body><h1>Control</h1><a href=\"high\">ON</a><br><a href=\"low\">OFF</a></body></html>";

// Has to implement this event_handler for WIFI station connection
static esp_err_t event_handler(void *ctx, system_event_t *event) {
	switch (event->event_id) {
	case SYSTEM_EVENT_STA_START:
		ESP_ERROR_CHECK(esp_wifi_connect())	;
		break;
	case SYSTEM_EVENT_STA_GOT_IP:
		xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);

		//Just for print out IP information
		tcpip_adapter_ip_info_t ip;
		tcpip_adapter_dns_info_t dns;
		memset(&ip, 0, sizeof(tcpip_adapter_ip_info_t));
		memset(&dns, 0, sizeof(tcpip_adapter_dns_info_t));
		ESP_LOGI(TAG, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~")
		;
		if (tcpip_adapter_get_ip_info(ESP_IF_WIFI_STA, &ip) == 0) {
			ESP_LOGI(TAG, "IP Address..........: "IPSTR, IP2STR(&ip.ip));
			ESP_LOGI(TAG, "Subnet Mask.........: "IPSTR, IP2STR(&ip.netmask));
			ESP_LOGI(TAG, "Default Gateway.....: "IPSTR, IP2STR(&ip.gw));
		}
		if (tcpip_adapter_get_dns_info(ESP_IF_WIFI_STA, TCPIP_ADAPTER_DNS_MAIN,
				&dns) == 0) {
			ESP_LOGI(TAG, "DNS Server..........: "IPSTR,
					IP2STR(&dns.ip.u_addr.ip4));
		}
		ESP_LOGI(TAG, "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~")
		;

		break;
	case SYSTEM_EVENT_STA_DISCONNECTED:

		switch (event->event_info.disconnected.reason)
		{

			case WIFI_REASON_AUTH_EXPIRE:
				ESP_LOGE(TAG, "WIFI_REASON_AUTH_EXPIRE");
				break;

			case WIFI_REASON_NOT_AUTHED:
				ESP_LOGE(TAG, "WIFI_REASON_NOT_AUTHED");
				break;

			case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
				ESP_LOGE(TAG, "WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT");
				break;

			case WIFI_REASON_NO_AP_FOUND:
				ESP_LOGE(TAG, "WIFI_REASON_NO_AP_FOUND");
				break;
			case WIFI_REASON_AUTH_FAIL:
				ESP_LOGE(TAG, "WIFI_REASON_AUTH_FAIL");
				break;

			case WIFI_REASON_HANDSHAKE_TIMEOUT:
				ESP_LOGE(TAG, "WIFI_REASON_HANDSHAKE_TIMEOUT");
				break;

			default:
				ESP_LOGE(TAG, "WiFi station disconnected, REASON CODE: %d", event->event_info.disconnected.reason );

		}

		xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
		//Try to reconnect Wifi again
		//if(event->event_info.disconnected.reason != WIFI_REASON_NO_AP_FOUND
		//	&& event->event_info.disconnected.reason != WIFI_REASON_AUTH_FAIL
		//	&& event->event_info.disconnected.reason != WIFI_REASON_NOT_AUTHED)
		//{

			ESP_ERROR_CHECK(esp_wifi_connect());
		//}

		break;
		default:
			break;
	}
	return ESP_OK;
}

static void initialise_wifi(void) {
	tcpip_adapter_init();

#ifdef ENABLE_STATIC_IP
	//For using of static IP
	tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_STA); // Don't run a DHCP client

	//Set static IP
	tcpip_adapter_ip_info_t ipInfo;
	inet_pton(AF_INET, STATIC_IP, &ipInfo.ip);
	inet_pton(AF_INET, GATE_WAY, &ipInfo.gw);
	inet_pton(AF_INET, SUBNET_MASK, &ipInfo.netmask);
	tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_STA, &ipInfo);

	//Set Main DNS server
	tcpip_adapter_dns_info_t dnsInfo;
	inet_pton(AF_INET, DNS_SERVER, &dnsInfo.ip);
	tcpip_adapter_set_dns_info(TCPIP_ADAPTER_IF_STA, TCPIP_ADAPTER_DNS_MAIN,
			&dnsInfo);
#endif

	wifi_event_group = xEventGroupCreate();
	ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT()
			;
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

	ESP_LOGI(TAG, "Setting Wifi configuration SSID %s...", wifi_config.sta.ssid);
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());

}

static void http_server_netconn_serve(struct netconn *conn) {
	struct netbuf *inbuf;
	char *buf;
	u16_t buflen;
	err_t err;

	/* Read the data from the port, blocking if nothing yet there.
	 We assume the request (the part we care about) is in one netbuf */
	err = netconn_recv(conn, &inbuf);

	if (err == ERR_OK) {
		netbuf_data(inbuf, (void**) &buf, &buflen);

		/* Is this an HTTP GET command? (only check the first 5 chars, since
		 there are other formats for GET, and we're keeping it very simple )*/
		if (buflen >= 5 && strncmp("GET ",buf,4)==0) {

			/*  sample:
			 * 	GET /l HTTP/1.1
				Accept: text/html, application/xhtml+xml, image/jxr,
				Referer: http://192.168.1.222/h
				Accept-Language: en-US,en;q=0.8,zh-Hans-CN;q=0.5,zh-Hans;q=0.3
				User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/51.0.2704.79 Safari/537.36 Edge/14.14393
				Accept-Encoding: gzip, deflate
				Host: 192.168.1.222
				Connection: Keep-Alive
			 *
			 */
			//Parse URL
			char* path = NULL;
			char* line_end = strchr(buf, '\n');
			if( line_end != NULL )
			{
				//Extract the path from HTTP GET request
				path = (char*)malloc(sizeof(char)*(line_end-buf+1));
				int path_length = line_end - buf - strlen("GET ")-strlen("HTTP/1.1")-2;
				strncpy(path, &buf[4], path_length );
				path[path_length] = '\0';
				//Get remote IP address
				ip_addr_t remote_ip;
				u16_t remote_port;
				netconn_getaddr(conn, &remote_ip, &remote_port, 0);
				printf("[ "IPSTR" ] GET %s\n", IP2STR(&(remote_ip.u_addr.ip4)),path);
			}

			/* Send the HTML header
			 * subtract 1 from the size, since we dont send the \0 in the string
			 * NETCONN_NOCOPY: our data is const static, so no need to copy it
			 */
			if(path != NULL)
			{

				if (strcmp("/high",path)==0) {
					gpio_set_level(LED_PIN,1);
				}
				if (strcmp("/low",path)==0) {
					gpio_set_level(LED_PIN,0);
				}

				free(path);
				path=NULL;
			}

			// Send HTTP response header
			netconn_write(conn, http_html_hdr, sizeof(http_html_hdr) - 1,
					NETCONN_NOCOPY);

			// Send HTML content
			netconn_write(conn, http_index_hml, sizeof(http_index_hml) - 1,
					NETCONN_NOCOPY);
		}

	}
	// Close the connection (server closes in HTTP)
	netconn_close(conn);

	// Delete the buffer (netconn_recv gives us ownership,
	// so we have to make sure to deallocate the buffer)
	netbuf_delete(inbuf);
}

static void http_server(void *pvParameters) {
	struct netconn *conn, *newconn;	//conn is listening thread, newconn is new thread for each client
	err_t err;
	conn = netconn_new(NETCONN_TCP);
	netconn_bind(conn, NULL, 80);
	netconn_listen(conn);
	do {
		err = netconn_accept(conn, &newconn);
		if (err == ERR_OK) {
			http_server_netconn_serve(newconn);
			netconn_delete(newconn);
		}
	} while (err == ERR_OK);
	netconn_close(conn);
	netconn_delete(conn);
}

void app_main() {
	ESP_ERROR_CHECK(nvs_flash_init());
	initialise_wifi();

	//GPIO initialization
	gpio_pad_select_gpio(LED_PIN);
	gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

	ESP_LOGI(TAG, "Hello world! App is running ... ...\n");

	xTaskCreate(&http_server, "http_server", 2048, NULL, 5, NULL);
}
