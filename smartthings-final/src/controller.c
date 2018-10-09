/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Flora License, Version 1.1 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://floralicense.org/license/
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <tizen.h>
#include <service_app.h>
#include <string.h>
#include <stdlib.h>
#include <Ecore.h>

#include "st_things.h"
#include "log.h"
#include "resource/resource_infrared_motion_sensor.h"
#include "resource/resource_led.h"
#include "sensor-data.h"

#define JSON_NAME "device_def.json"
#define SENSOR_MOTION_URI "/capability/motionSensor/main/0"
#define SENSOR_MOTION_KEY "value"
#define SENSOR_LED_URI "/capability/switch/main/0"
#define SENSOR_LED_KEY "power"
#define SENSOR_LED_INIT "off"

#define LED_ON "on"
#define LED_OFF "off"

#define USE_ST_SDK

typedef struct app_data_s {
	Ecore_Timer *getter_motion;
	sensor_data *motion_data;
	sensor_data *led_data;
} app_data;

static app_data *g_ad = NULL;

static Eina_Bool __change_motion_sensor_data(void *data)
{
	uint32_t value = 0;

	// Get value from motion sensor
	int ret = resource_read_infrared_motion_sensor(46, &value);

	if (ret != 0) _E("Cannot read sensor value");

	sensor_data_set_bool(g_ad->motion_data, value);

	_D("Detected motion value is: %d", value);

	// Notify observers of the Motion sensor resource
	st_things_notify_observers(SENSOR_MOTION_URI);

	return ECORE_CALLBACK_RENEW;
}

static int __change_led_data(void *data, char *state) {
	int ret = 0;
	app_data *ad = data;

	retv_if(!ad, -1);
	retv_if(!ad->led_data, -1);

	sensor_data_set_string(g_ad->led_data, state, strlen(state));

	if (0 == strcmp(state, LED_ON)) {
		ret = resource_write_led(130, 1);
	} else {
		ret = resource_write_led(130, 0);
	}

	retv_if(ret != 0, -1);

	// Notify observers of the LED resource
	st_things_notify_observers(SENSOR_LED_URI);

	return 0;
}

static bool __handle_get_request_on_motion (st_things_get_request_message_s* req_msg, st_things_representation_s* resp_rep)
{
	if (req_msg->has_property_key(req_msg, SENSOR_MOTION_KEY)) {
		bool value = false;

		sensor_data_get_bool(g_ad->motion_data, &value);

		// Update the response representation about the Motion sensor property which is sent to the client
		resp_rep->set_bool_value(resp_rep, SENSOR_MOTION_KEY, value);

		_D("Value : %d", value);

		return true;
	} else {
		_E("not supported property");

		return false;
	}
}

static bool __handle_get_request_on_led (st_things_get_request_message_s* req_msg, st_things_representation_s* resp_rep)
{
	if (req_msg->has_property_key(req_msg, SENSOR_LED_KEY)) {
		const char *str = NULL;

		sensor_data_get_string(g_ad->led_data, &str);

		if (!str) {
			str = SENSOR_LED_INIT;
		}

		// Update the response representation about the LED property which is sent to the client
		resp_rep->set_str_value(resp_rep, SENSOR_LED_KEY, str);

		_D("Power : %s", str);

		return true;
	} else {
		_E("not supported property");

		return false;
	}
}

static bool __handle_set_request_on_led (st_things_set_request_message_s* req_msg, st_things_representation_s* resp_rep)
{
	int ret = 0;
	char *str = NULL;

	if (req_msg->rep->get_str_value(req_msg->rep, SENSOR_LED_KEY, &str)) {
		retv_if(!str, false);

		_D("set [%s:%s] == %s", SENSOR_LED_URI, SENSOR_LED_KEY, str);

		// Update the response representation about the LED property which is sent to the client
		resp_rep->set_str_value(resp_rep, SENSOR_LED_KEY, str);

		// Turn on LED light
		ret = __change_led_data(g_ad, strdup(str));

		retv_if(ret != 0, false);
	} else {
		_E("cannot get a string value");

		return false;

	}

	free(str);

	return true;
}

void gathering_start(void *data)
{
	app_data *ad = data;
	ret_if(!ad);

	if (ad->getter_motion)
		ecore_timer_del(ad->getter_motion);

	ad->getter_motion = ecore_timer_add(1.0f, __change_motion_sensor_data, ad);

	if (!ad->getter_motion) {
		_E("Failed to add infrared motion getter timer");
	}

	return;
}

static bool handle_reset_request(void)
{
	_D("Received a request for RESET.");
	return false;
}

static void handle_reset_result(bool result)
{
	_D("Reset %s.\n", result ? "succeeded" : "failed");
}

static bool handle_ownership_transfer_request(void)
{
	_D("Received a request for Ownership-transfer.");
	return true;
}

static void handle_things_status_change(st_things_status_e things_status)
{
	_D("Things status is changed: %d\n", things_status);

	if (things_status == ST_THINGS_STATUS_REGISTERED_TO_CLOUD)
		ecore_main_loop_thread_safe_call_async(gathering_start, g_ad);
}

static bool handle_get_request(st_things_get_request_message_s* req_msg, st_things_representation_s* resp_rep)
{
	bool ret = false;

	_D("resource_uri [%s]", req_msg->resource_uri);
	retv_if(!g_ad, false);

	if (0 == strcmp(req_msg->resource_uri, SENSOR_MOTION_URI)) {
		_D("query : %s, property: %s", req_msg->query, req_msg->property_key);

		// Call get request function for motion sensor
		ret = __handle_get_request_on_motion(req_msg, resp_rep);
	} else if (0 == strcmp(req_msg->resource_uri, SENSOR_LED_URI)) {
		_D("query : %s, property: %s", req_msg->query, req_msg->property_key);

		// Call get request function for LED
		ret = __handle_get_request_on_led(req_msg, resp_rep);
	} else {
		_E("not supported uri");
	}

	return ret;
}

static bool handle_set_request(st_things_set_request_message_s* req_msg, st_things_representation_s* resp_rep)
{
	bool ret = false;

	_D("resource_uri [%s]", req_msg->resource_uri);
	retv_if(!g_ad, false);

	if (0 == strcmp(req_msg->resource_uri, SENSOR_LED_URI)) {
		// Call set request function for LED
		ret = __handle_set_request_on_led(req_msg, resp_rep);
	} else {
		_E("not supported uri");
	}

	return ret;
}

static int __things_init(void)
{
	bool easysetup_complete = false;
	char app_json_path[128] = {'\0', };
	char *app_res_path = NULL;
	char *app_data_path = NULL;

	app_res_path = app_get_resource_path();
	if (!app_res_path) {
		_E("app_res_path is NULL!!");
		return -1;
	}

	app_data_path = app_get_data_path();
	if (!app_data_path) {
		_E("app_data_path is NULL!!");
		free(app_res_path);
		return -1;
	}

	// Specify the read-only and read-write path
	if (0 != st_things_set_configuration_prefix_path(app_res_path, app_data_path)) {
		_E("st_things_set_configuration_prefix_path() failed!!");
		free(app_res_path);
		free(app_data_path);
		return -1;
	}
	free(app_data_path);

	snprintf(app_json_path, sizeof(app_json_path), "%s%s", app_res_path, JSON_NAME);
	free(app_res_path);

	// Specify the device configuration JSON file and change the status of easysetup_complete
	if (0 != st_things_initialize(app_json_path, &easysetup_complete)) {
		_E("st_things_initialize() failed!!");
		return -1;
	}

	_D("easysetup_complete:[%d] ", easysetup_complete);

	// Register callback for handling request get (handle_get_request) and request set (handle_set_request) messages
	st_things_register_request_cb(handle_get_request, handle_set_request);
	// Register callback for reset confirmation (handle_reset_request) and reset result(handle_reset_result) functions
	st_things_register_reset_cb(handle_reset_request, handle_reset_result);
	// Register callback for getting user confirmation for ownership transfer (handle_ownership_transfer_request)
	st_things_register_user_confirm_cb(handle_ownership_transfer_request);
	// Register callback for getting notified when ST Things state changes (handle_things_status_change)
	st_things_register_things_status_change_cb(handle_things_status_change);

	return 0;
}

static int __things_deinit(void)
{
	st_things_deinitialize();
	return 0;
}

static int __things_start(void)
{
	st_things_start();
	return 0;
}

static int __things_stop(void)
{
	st_things_stop();
	return 0;
}

static bool service_app_create(void *user_data)
{
	app_data *ad = user_data;

	// Declare new sensor data for Motion data
	ad->motion_data = sensor_data_new(SENSOR_DATA_TYPE_BOOL);

	if (!ad->motion_data)
		return false;

	// Declare new sensor data for LED data
	ad->led_data = sensor_data_new(SENSOR_DATA_TYPE_STR);

	if (!ad->led_data)
		return false;

	sensor_data_set_string(g_ad->led_data, SENSOR_LED_INIT, strlen(SENSOR_LED_INIT));

	if (__things_init())
		return false;

	return true;
}

static void service_app_control(app_control_h app_control, void *user_data)
{
	__things_start();
}

static void service_app_terminate(void *user_data)
{
	app_data *ad = (app_data *)user_data;

	// Delete ecore timer
	if (ad->getter_motion)
		ecore_timer_del(ad->getter_motion);

	// Stop and deinitialize things
	__things_stop();
	__things_deinit();

	// Turn off LED light with __set_led()
	__change_led_data(ad, LED_OFF);

	// Free sensor Motion & LED data
	sensor_data_free(ad->motion_data);
	sensor_data_free(ad->led_data);

	// Close Motion and LED resources
	resource_close_infrared_motion_sensor();
	resource_close_led();

	// Free app data
	free(ad);

	FN_END;
}

int main(int argc, char *argv[])
{
	app_data *ad = NULL;
	service_app_lifecycle_callback_s event_callback;

	ad = calloc(1, sizeof(app_data));
	retv_if(!ad, -1);

	g_ad = ad;

	event_callback.create = service_app_create;
	event_callback.terminate = service_app_terminate;
	event_callback.app_control = service_app_control;

	return service_app_main(argc, argv, &event_callback, ad);
}

