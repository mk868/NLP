#include "ws_worker.h"

#include <osapi.h>
#include <os_type.h>
#include <ip_addr.h>
#include <user_interface.h>

#include <user_config.h>

#include "esplogger.h"
#include "wifi_manager.h"
#include "websocket_client.h"


#include <json/jsonparse.h>
#include <json/jsontree.h>
#include <json/json.h>
#include "user_json.h"

#include <string.h>

#include "port_worker.h"


static void ICACHE_FLASH_ATTR timer_cb(void *arg);
static void start_timer();
static void stop_timer();
void status_change_callback(WebSocketClient *client, enum WebSocketClientStatus status);
void data_receive_callback(WebSocketClient *client, uint8_t opcode, char* data, unsigned int length);


static os_timer_t   _mainTimer;
static struct WebSocketClient wsClient;


static void ICACHE_FLASH_ATTR timer_cb(void *arg)
{
	log_info("timer_cb");
	if (!wifi_manager_is_internet_access()) {
		log_info("no internet, wait...");
		start_timer();
		return;
	}

	websocket_client_connect(&wsClient, WS_SERVER_HOST, WS_SERVER_PORT, WS_SERVER_PATH);
}

static void ICACHE_FLASH_ATTR start_timer() {
	os_timer_disarm(&_mainTimer);
	os_timer_setfn(&_mainTimer, (os_timer_func_t *)timer_cb, NULL);
	os_timer_arm(&_mainTimer, 3000, false);
}

static void ICACHE_FLASH_ATTR stop_timer() {
	os_timer_disarm(&_mainTimer);
}



void ICACHE_FLASH_ATTR status_change_callback(WebSocketClient *client, enum WebSocketClientStatus status) {
	log_info("status: %d", status);

	switch (status) {
	case WebSocketClientConnected:
	{
		char buff[100];
		os_sprintf(buff, "{ \"type\": \"hi\", \"id\": %d }", system_get_chip_id());
		websocket_client_send(client, buff, strlen(buff), WS_OPCODE_TEXT);
	} break;
	case WebSocketClientDnsError:
	case WebSocketClientDisconnected:
	case WebSocketConnectionTimeoutError:
	case WebSocketClientHandshakeError:
		start_timer();
		break;
	case WebSocketClientFrameParseError:
		break;

	}
}







static struct wsMessage
{
	char type[20];
	//TODO: union
	char device[20];
	bool enabled;
	bool timeSet;
	uint32 time;
};

static struct wsMessage *wsMessageBuffer = NULL;


static int ICACHE_FLASH_ATTR ws_message_set(struct jsontree_context *js_ctx, struct jsonparse_state *parser)
{
	log_info("ws_message_set");

	int type;

	os_bzero(wsMessageBuffer, sizeof(struct wsMessage));

	while ((type = jsonparse_next(parser)) != 0) {
		if (type == JSON_TYPE_PAIR_NAME) {

			if (jsonparse_strcmp_value(parser, "type") == 0) {
				jsonparse_next(parser);
				jsonparse_next(parser);
				jsonparse_copy_value(parser, wsMessageBuffer->type, sizeof(wsMessageBuffer->type));
				//os_memcpy(new_ssid, buffer, os_strlen(buffer));
			}
			else if (jsonparse_strcmp_value(parser, "device") == 0) {
				jsonparse_next(parser);
				jsonparse_next(parser);
				jsonparse_copy_value(parser, wsMessageBuffer->device, sizeof(wsMessageBuffer->device));
				//os_memcpy(new_ssid, buffer, os_strlen(buffer));
			}
			else if (jsonparse_strcmp_value(parser, "enable") == 0) {
				jsonparse_next(parser);
				jsonparse_next(parser);
				wsMessageBuffer->enabled = jsonparse_get_value_as_int(parser);
				//wsMessageBuffer->enabled = jsonparse_get_value_as_int(parser);
			}
			else if (jsonparse_strcmp_value(parser, "time") == 0) {
				jsonparse_next(parser);
				jsonparse_next(parser);
				wsMessageBuffer->time = jsonparse_get_value_as_int(parser);
				wsMessageBuffer->timeSet = true;
			}
		}
	}

	return 0;
}

static struct jsontree_callback ws_message_info_callback = JSONTREE_CALLBACK(NULL, ws_message_set);

JSONTREE_OBJECT(ws_message_info_tree,
	JSONTREE_PAIR("device", &ws_message_info_callback),
	JSONTREE_PAIR("enable", &ws_message_info_callback),
	JSONTREE_PAIR("time", &ws_message_info_callback)
);

JSONTREE_OBJECT(ws_message_info_root,
	JSONTREE_PAIR("root", &ws_message_info_tree));



void ICACHE_FLASH_ATTR data_receive_callback(WebSocketClient *client, uint8_t opcode, char* data, unsigned int length) {
	

	if (opcode == (WS_OPCODE_TEXT | WS_FIN)) {

		log_info("data(%d):: %s (%d)", opcode, data, length);



		struct wsMessage buffer;
		wsMessageBuffer = &buffer;

		struct jsontree_context js;
		jsontree_setup(&js, (struct jsontree_value *)&ws_message_info_root, json_putchar);
		json_parse(&js, data);


		log_info("dev: %s, enable: %d, time(%d): %d", buffer.device, buffer.enabled, buffer.timeSet, buffer.time);


		wsMessageBuffer = NULL;


		if (strncmp("let", buffer.type, 3) == 0) {

			Devices device = -1;

			if (strncmp(buffer.device, "REL1", 3) == 0) {
				device = REL1;
			}//TODO more/all devices

			if (device != -1) {

				if (buffer.enabled) {
					if (buffer.timeSet) {
						port_worker_enable_to_time(device, buffer.time);
					}
					else {
						port_worker_enable(device);
					}
				}
				else {
					port_worker_disable(device);
				}
			}
		}
	}
}




void ICACHE_FLASH_ATTR ws_worker_start()
{
	websocket_client_init(&wsClient);
	wsClient.dataReceiveCallback = data_receive_callback;
	wsClient.statusChangeCallback = status_change_callback;

	start_timer();
}

void ws_worker_send_state(char* name, bool enabled)
{
	if (!websocket_client_is_connected(&wsClient)) {
		return;
	}

	log_info("sending state info: %s = %d", name, enabled);
	char buff[100];
	os_sprintf(buff, "{ \"type\": \"state\", \"device\": \"%s\", \"enabled\": %d }", name, enabled);
	websocket_client_send(&wsClient, buff, strlen(buff), WS_OPCODE_TEXT);
}
