/*
 * Copyright (c) 2017 Samsung Electronics Co., Ltd.
 *
 * Contact: Jin Yoon <jinny.yoon@samsung.com>
 *          Geunsun Lee <gs86.lee@samsung.com>
 *          Eunyoung Lee <ey928.lee@samsung.com>
 *          Junkyu Han <junkyu.han@samsung.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <unistd.h>
#include <Ecore.h>
#include <tizen.h>
#include <service_app.h>

#include "log.h"
#include "resource/resource_infrared_motion_sensor.h"
#include "resource/resource_led.h"

typedef struct app_data_s {
	Ecore_Timer *getter_timer;
} app_data;

static Eina_Bool __read_motion_write_led(void *data)
{
	uint32_t value = 0;
	int ret = -1;

	// Get value from motion sensor
	ret = resource_read_infrared_motion_sensor(46, &value);

	if (ret != 0) _E("Cannot read sensor value");

	_D("Detected motion value is: %d", value);

	// Send value to LED light
	//resource_write_led(130, value);

	colorpin = [32,33,34];   //r,g,b색상 pin의 배열
	resource_write_color_led(colorpin[], value);


	return ECORE_CALLBACK_RENEW;
}

static bool service_app_create(void *data)
{
	app_data *ad = data;

	// Create a timer to call the given function in the given period of time
	ad->getter_timer = ecore_timer_add(7.0f, __read_motion_write_led, ad);   //연속으로 비출 수 있는 넉넉한 시간인 7초로 변경

	if (!ad->getter_timer) {
		_E("Failed to add infrared motion getter timer");
		return false;
	}

	return true;
}

static void service_app_terminate(void *data)
{
	app_data *ad = (app_data *)data;


	/* Remove all resources and timers created for this app */
	// Delete timer
	if (ad->getter_timer)
		ecore_timer_del(ad->getter_timer);

	// Close infrared motion & led resources
	resource_close_infrared_motion_sensor();
	resource_close_led();

	// Free data resource
	free(ad);
}

static void service_app_control(app_control_h app_control, void *data)
{
	/*SERVICE_APP_CONTROL*/
}

static void service_app_lang_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LANGUAGE_CHANGED*/
}

static void service_app_region_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_REGION_FORMAT_CHANGED*/
}

static void service_app_low_battery(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LOW_BATTERY*/
}

static void service_app_low_memory(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LOW_MEMORY*/
}

int main(int argc, char* argv[])
{
	app_data *ad = NULL;
	int ret = 0;
	service_app_lifecycle_callback_s event_callback;
	app_event_handler_h handlers[5] = {NULL, };

	ad = calloc(1, sizeof(app_data));
	retv_if(!ad, -1);

	event_callback.create = service_app_create;
	event_callback.terminate = service_app_terminate;
	event_callback.app_control = service_app_control;

	service_app_add_event_handler(&handlers[APP_EVENT_LOW_BATTERY], APP_EVENT_LOW_BATTERY, service_app_low_battery, &ad);
	service_app_add_event_handler(&handlers[APP_EVENT_LOW_MEMORY], APP_EVENT_LOW_MEMORY, service_app_low_memory, &ad);
	service_app_add_event_handler(&handlers[APP_EVENT_LANGUAGE_CHANGED], APP_EVENT_LANGUAGE_CHANGED, service_app_lang_changed, &ad);
	service_app_add_event_handler(&handlers[APP_EVENT_REGION_FORMAT_CHANGED], APP_EVENT_REGION_FORMAT_CHANGED, service_app_region_changed, &ad);

	ret = service_app_main(argc, argv, &event_callback, ad);

	return ret;
}
