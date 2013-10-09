#include "link_monitor.h"
#include "config.h"
#include "weather_layer.h"
#include "pebble_os.h"
#include "pebble_app.h"

#define ERROR_FRAME      (GRect(25, 0, 30, 30))
	
enum LinkStatus
{
	LinkStatusUnknown,
	LinkStatusFailed,
	LinkStatusOK,
};
	
enum LinkStatus __linkStatus = LinkStatusUnknown;
	
extern TextLayer date_layer;
extern WeatherLayer weather_layer;
extern TextLayer indicators_layer;
extern BmpContainer icon_error; 
extern BmpContainer icon_link;
extern BmpContainer icon_battery;

	
void link_monitor_ping()
{
	//Sending ANY message to the phone would do
	http_time_request();
	psleep(200);
	//http_battery_request();
}

void link_monitor_handle_failure(int error)
{
	layer_remove_from_parent(&icon_error.layer.layer);
	bmp_deinit_container(&icon_error);
	//Add weather icon
	//bmp_init_container(RESOURCE_ID_INDICATOR_LINK, &error_layer.layer);
	
	switch(error)
	{
		case 1002: //Timeout
			//Considered a link failure
			//weather_layer_set_icon(&weather_layer, WEATHER_ICON_APP_ERROR);
		    //text_layer_set_text(&weather_layer.street_layer, "Tiempo de espera agotado");
		    bmp_init_container(RESOURCE_ID_ERROR_1002, &icon_error);
			layer_add_child(&indicators_layer.layer, &icon_error.layer.layer);
			layer_set_frame(&icon_error.layer.layer, ERROR_FRAME);
			break;
		
		case 1004: //Send rejected
			//Considered a link failure
			//weather_layer_set_icon(&weather_layer, WEATHER_ICON_LINK_ERROR);
		    //text_layer_set_text(&weather_layer.street_layer, "Envio rechazado");
			//text_layer_set_text(&debug_layer, itoa(error));
			bmp_init_container(RESOURCE_ID_ERROR_1004, &icon_error);
			layer_add_child(&indicators_layer.layer, &icon_error.layer.layer);
			layer_set_frame(&icon_error.layer.layer, ERROR_FRAME);
			break;
		
		case 1008: //Watchapp not running
			//Considered a link failure
			//weather_layer_set_icon(&weather_layer, WEATHER_ICON_LINK_ERROR);
		    //text_layer_set_text(&weather_layer.street_layer, "iPhone no conectado");
			//text_layer_set_text(&debug_layer, itoa(error));
			bmp_init_container(RESOURCE_ID_ERROR_1008, &icon_error);
			layer_add_child(&indicators_layer.layer, &icon_error.layer.layer);
			layer_set_frame(&icon_error.layer.layer, ERROR_FRAME);
			layer_remove_from_parent(&icon_link.layer.layer);
			bmp_deinit_container(&icon_link);
			break;
		
		case 1064: //APP_MSG_BUSY
			//These are more likely to specify a temporary error than a lost watch
			//weather_layer_set_icon(&weather_layer, WEATHER_ICON_APP_ERROR);
		    //text_layer_set_text(&weather_layer.street_layer, "Conexion ocupada");
			//text_layer_set_text(&debug_layer, itoa(error));
			return;
		
		case 1128: //APP_MSG_BUSY
			//These are more likely to specify a temporary error than a lost watch
			//weather_layer_set_icon(&weather_layer, WEATHER_ICON_APP_ERROR);
		    //text_layer_set_text(&weather_layer.street_layer, "Buffer overflow");
			//text_layer_set_text(&debug_layer, itoa(error));
		    // Remove any possible existing weather icon
			bmp_init_container(RESOURCE_ID_ERROR_1128, &icon_error);
			layer_add_child(&indicators_layer.layer, &icon_error.layer.layer);
			layer_set_frame(&icon_error.layer.layer, ERROR_FRAME);
			return;
		
		case HTTP_INVALID_BRIDGE_RESPONSE + 1000:
			//The phone may have no internet connection, but the link should be fine
			//weather_layer_set_icon(&weather_layer, WEATHER_ICON_INET_ERROR);
		    //text_layer_set_text(&weather_layer.street_layer, "Sin Internet");
			//text_layer_set_text(&debug_layer, itoa(error));
			bmp_init_container(RESOURCE_ID_ERROR_BRIDGE, &icon_error);
			layer_add_child(&indicators_layer.layer, &icon_error.layer.layer);
			layer_set_frame(&icon_error.layer.layer, ERROR_FRAME);
			return;
		
	}
	
	//text_layer_set_text(&weather_layer.city_layer, itoa(error));
	//text_layer_set_text(&weather_layer.temp_layer, "Error");
		    
	if(__linkStatus == LinkStatusOK)
	{
		//The link has just failed, notify the user
		// Vibe pattern: ON, OFF, ON, ...
		static const uint32_t const segments[] = { 150, 100, 150, 100, 300 };
		VibePattern pat = {
			.durations = segments,
			.num_segments = ARRAY_LENGTH(segments),
		};
	
		vibes_enqueue_custom_pattern(pat);
	}
	
	__linkStatus = LinkStatusFailed;
}

void link_monitor_handle_success()
{
	if(__linkStatus == LinkStatusFailed)
	{
		//Notify the user of reconnection
		vibes_short_pulse();
	}
	
	__linkStatus = LinkStatusOK;
}