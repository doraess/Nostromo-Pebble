#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

#include "http.h"
#include "httpcapture.h"
#include "util.h"
#include "weather_layer.h"
#include "time_layer.h"
#include "link_monitor.h"
#include "config.h"

#define MY_UUID { 0x91, 0x41, 0xB6, 0x28, 0xBC, 0x89, 0x49, 0x8E, 0xB1, 0x47, 0x04, 0x9F, 0x49, 0xC0, 0x99, 0xAD }

PBL_APP_INFO(MY_UUID,
             "Nostromo Weather", "Alberto Martín de la Torre", // Modification of "Roboto Weather" by Martin Rosinski
             1, 71, /* App version */
             RESOURCE_ID_IMAGE_MENU_ICON,
             APP_INFO_WATCH_FACE);

#define TIME_FRAME      (GRect(0, 2, 144, 162))
#define DATE_FRAME      (GRect(0, 55, 144, 25))
#define INDICATORS_FRAME      (GRect(0, 75, 144, 30))
#define LINK_FRAME      (GRect(0, 0, 30, 30))
#define ERROR_FRAME      (GRect(25, 0, 30, 30))
#define BATTERY_FRAME      (GRect(50, 0, 30, 30))
#define BATTERY_LEVEL_FRAME      (GRect(75, 7, 30, 30))
#define UPDATE_FRAME      (GRect(80, 7, 60, 30))

// POST variables
#define WEATHER_KEY_LATITUDE 1
#define WEATHER_KEY_LONGITUDE 2
#define WEATHER_KEY_UNIT_SYSTEM 3
	
// Received variables
#define WEATHER_ICON_KEY 1
#define WEATHER_TEMP_HUM_KEY 2
#define WEATHER_STREET_KEY 5
#define WEATHER_CITY_KEY 6
	
#define WEATHER_HTTP_COOKIE 1949327671
#define TIME_HTTP_COOKIE 1131038282
	
#define MAKE_SCREENSHOT 1

Window window;          /* main window */
TextLayer date_layer;   /* layer for the date */
TimeLayer time_layer;   /* layer for the time */
TextLayer updated_layer;   /* layer for last update */
TextLayer indicators_layer;   /* layer for last update */
TextLayer battery_level_layer;   /* layer for battery level */

BmpContainer icon_error; 
BmpContainer icon_link;
BmpContainer icon_battery;

GFont font_date;        /* font for date */
GFont font_hour;        /* font for hour */
GFont font_minute;      /* font for minute */


char *days[]={
  "Lun",
  "Mar",
  "Mie",
  "Jue",
  "Vie",
  "Sab",
  "Dom"
};

char *months[]={
  "Ene",
  "Feb",
  "Mar",
  "Abr",
  "May",
  "Jun",
  "Jul",
  "Ago",
  "Sep",
  "Oct",
  "Nov",
  "Dic"
};

int32_t battery_level = 0; 

//Weather Stuff
static int our_latitude, our_longitude;
static bool located = false;
bool on_error = false;

WeatherLayer weather_layer;

void request_weather();

void failed(int32_t cookie, int http_status, void* context) {
			
	char buffer [50];
	if (cookie == 1) {
		snprintf(buffer, 50, "[%4d] Send error", http_status);
		//http_log(buffer);
	}
	else if (cookie == 2) {
		snprintf(buffer, 50, "[%4d] Recv error", http_status);
		//http_log(buffer);
	}
	
	link_monitor_handle_failure(http_status);
	
	//Re-request the location and subsequently weather on next minute tick
	located = false;
}

void success(int32_t cookie, int http_status, DictionaryIterator* received, void* context) {
	if(cookie != WEATHER_HTTP_COOKIE) return;
	Tuple* icon_tuple = dict_find(received, WEATHER_ICON_KEY);	
	if(icon_tuple) {
		int icon = icon_tuple->value->int8;
		if(icon >= 0 && icon < 10) {
			weather_layer_set_icon(&weather_layer, icon);
		} else {
			weather_layer_set_icon(&weather_layer, WEATHER_ICON_NO_WEATHER);
		}
	}
	
	Tuple* weather_tuple = dict_find(received, WEATHER_TEMP_HUM_KEY);
	if(weather_tuple) {
		weather_layer_set_temperature(&weather_layer, weather_tuple->value->cstring);
	}
	Tuple* street_tuple = dict_find(received, WEATHER_STREET_KEY);
	if(street_tuple) {
		weather_layer_set_street(&weather_layer, street_tuple->value->cstring);
	}
	Tuple* city_tuple = dict_find(received, WEATHER_CITY_KEY);
	if(city_tuple) {
		weather_layer_set_city(&weather_layer, city_tuple->value->cstring);
	}
	
	PblTm now;
	static char now_text[] = "@00:00";
	get_time(&now);
	string_format_time(now_text, sizeof(now_text), "@%H:%M", &now);
	text_layer_set_text(&updated_layer, now_text);
    
	link_monitor_handle_success();
	
	if (!on_error) {
		layer_remove_from_parent(&icon_error.layer.layer);
	    bmp_deinit_container(&icon_error);
	}
	layer_remove_from_parent(&icon_link.layer.layer);
	bmp_deinit_container(&icon_link);
	bmp_init_container(RESOURCE_ID_INDICATOR_LINK, &icon_link);
	layer_add_child(&indicators_layer.layer, &icon_link.layer.layer);
	layer_set_frame(&icon_link.layer.layer, LINK_FRAME);
	
	http_battery_request();
}

void location(float latitude, float longitude, float altitude, float accuracy, void* context) {
	// Fix the floats
	our_latitude = latitude * 10000;
	our_longitude = longitude * 10000;
	located = true;
	request_weather();
	
	layer_remove_from_parent(&icon_error.layer.layer);
	bmp_deinit_container(&icon_error);
	layer_remove_from_parent(&icon_link.layer.layer);
	bmp_deinit_container(&icon_link);
	bmp_init_container(RESOURCE_ID_INDICATOR_LINK, &icon_link);
	layer_add_child(&indicators_layer.layer, &icon_link.layer.layer);
	layer_set_frame(&icon_link.layer.layer, LINK_FRAME);
}

void battery(int32_t status, int32_t level, void* context) {

	static char level_text[] = "00%";
	snprintf(level_text, sizeof(level_text), "%s%%", itoa(level));
	text_layer_set_text(&battery_level_layer, level_text);
	//layer_remove_from_parent(&icon_battery.layer.layer);
	//bmp_deinit_container(&icon_battery);
	battery_level = level;
	//int division = level/33;
	//switch (division){
	//	case 0:	bmp_init_container(RESOURCE_ID_BATTERY_EMPTY, &icon_battery);
	//			break;
	//	case 1:	bmp_init_container(RESOURCE_ID_BATTERY_1HALF, &icon_battery);
	//			break;
	//	case 2:	bmp_init_container(RESOURCE_ID_BATTERY_2HALF, &icon_battery);
	//			break;
	//	case 3:	bmp_init_container(RESOURCE_ID_BATTERY_FULL, &icon_battery);
	//			break;
	//	default:break;
	//	
	//}
	//layer_add_child(&indicators_layer.layer, &icon_battery.layer.layer);
	//layer_set_frame(&icon_battery.layer.layer, BATTERY_FRAME);
	layer_mark_dirty(&indicators_layer.layer);
}

void indicators_layer_update_callback(Layer *me, GContext* ctx) {
	graphics_context_set_fill_color(ctx, GColorWhite);
	graphics_context_set_stroke_color(ctx, GColorWhite);
	graphics_draw_rect(ctx, GRect(50, 10, 22, 9));
	graphics_fill_rect(ctx, GRect(52, 12, 18*battery_level/100, 5), 0, GCornerNone);
}


void reconnect(void* context) {
	located = false;
	request_weather();
}

void request_weather();

/* Called by the OS once per minute. Update the time and date.
*/
void handle_minute_tick(AppContextRef ctx, PebbleTickEvent *t)
{
    /* Need to be static because pointers to them are stored in the text
    * layers.
    */
    static char date_text[] = "XXX, 00 XXX";
    static char hour_text[] = "00";
    static char minute_text[] = ":00";

    (void)ctx;  /* prevent "unused parameter" warning */
    
    if (t->units_changed & DAY_UNIT)
    {		
	    /*string_format_time(date_text,
                           sizeof(date_text),
                           "%a %d",
                           t->tick_time);*/
        snprintf(date_text, sizeof(date_text), "%s, %d %s", days[t->tick_time->tm_wday - 1], t->tick_time->tm_mday, months[t->tick_time->tm_mon]);
		
		if (date_text[4] == '0') /* is day of month < 10? */
		{
		    /* This is a hack to get rid of the leading zero of the
			   day of month
            */
            memmove(&date_text[4], &date_text[5], sizeof(date_text) - 1);
		}
        text_layer_set_text(&date_layer, date_text);
    }
	
	if (t->tick_time->tm_min == 0) vibes_double_pulse();

    if (clock_is_24h_style())
    {
        string_format_time(hour_text, sizeof(hour_text), "%H", t->tick_time);
		if (hour_text[0] == '0')
        {
            /* This is a hack to get rid of the leading zero of the hour.
            */
            memmove(&hour_text[0], &hour_text[1], sizeof(hour_text) - 1);
        }
    }
    else
    {
        string_format_time(hour_text, sizeof(hour_text), "%I", t->tick_time);
        if (hour_text[0] == '0')
        {
            /* This is a hack to get rid of the leading zero of the hour.
            */
            memmove(&hour_text[0], &hour_text[1], sizeof(hour_text) - 1);
        }
    }

    string_format_time(minute_text, sizeof(minute_text), ":%M", t->tick_time);
    time_layer_set_text(&time_layer, hour_text, minute_text);
	
	if(!located || !(t->tick_time->tm_min % 15))
	{
		//Every 15 minutes, request updated weather
		http_location_request();
		
	}
	else
	{
		//Every minute, ping the phone
		link_monitor_ping();
	}
}


/* Initialize the application.
*/
void handle_init(AppContextRef ctx)
{
    PblTm tm;
    PebbleTickEvent t;
    ResHandle res_d;
    ResHandle res_h;
    ResHandle res_m;

    window_init(&window, "Futura");
    window_stack_push(&window, true /* Animated */);
    window_set_background_color(&window, GColorBlack);

    resource_init_current_app(&APP_RESOURCES);

    res_d = resource_get_handle(RESOURCE_ID_FUTURA_18);
    res_h = resource_get_handle(RESOURCE_ID_FUTURA_CONDENSED_45);

    font_date = fonts_load_custom_font(res_d);
    font_hour = fonts_load_custom_font(res_h);
    font_minute = fonts_load_custom_font(res_h);

    time_layer_init(&time_layer, window.layer.frame);
    time_layer_set_text_color(&time_layer, GColorWhite);
    time_layer_set_background_color(&time_layer, GColorClear);
    time_layer_set_fonts(&time_layer, font_hour, font_minute);
    layer_set_frame(&time_layer.layer, TIME_FRAME);
    layer_add_child(&window.layer, &time_layer.layer);

    text_layer_init(&date_layer, window.layer.frame);
    text_layer_set_text_color(&date_layer, GColorWhite);
    text_layer_set_background_color(&date_layer, GColorClear);
    text_layer_set_font(&date_layer, font_date);
    text_layer_set_text_alignment(&date_layer, GTextAlignmentCenter);
    layer_set_frame(&date_layer.layer, DATE_FRAME);
    layer_add_child(&window.layer, &date_layer.layer);
	
	
	text_layer_init(&indicators_layer, INDICATORS_FRAME);
	text_layer_set_text_color(&indicators_layer, GColorWhite);
    text_layer_set_background_color(&indicators_layer, GColorClear);
    text_layer_set_font(&indicators_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_STREET_FUTURA_12)));
    text_layer_set_text_alignment(&indicators_layer, GTextAlignmentLeft);
    //layer_set_frame(&indicators_layer.layer, INDICATORS_FRAME);
    layer_add_child(&window.layer, &indicators_layer.layer);
    indicators_layer.layer.update_proc = &indicators_layer_update_callback;
 

	text_layer_init(&updated_layer, UPDATE_FRAME);
    text_layer_set_text_color(&updated_layer, GColorWhite);
    text_layer_set_background_color(&updated_layer, GColorClear);
    text_layer_set_font(&updated_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_STREET_FUTURA_12)));
    text_layer_set_text_alignment(&updated_layer, GTextAlignmentRight);
    //layer_set_frame(&updated_layer.layer, UPDATE_FRAME);
    layer_add_child(&indicators_layer.layer, &updated_layer.layer);
	text_layer_set_text(&updated_layer, "@00:00");
	
	text_layer_init(&battery_level_layer, BATTERY_LEVEL_FRAME);
	text_layer_set_text_color(&battery_level_layer, GColorWhite);
    text_layer_set_background_color(&battery_level_layer, GColorClear);
    text_layer_set_font(&battery_level_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_STREET_FUTURA_12)));
    text_layer_set_text_alignment(&battery_level_layer, GTextAlignmentLeft);
    //layer_set_frame(&battery_level_layer.layer, BATTERY_LEVEL_FRAME);
    layer_add_child(&indicators_layer.layer, &battery_level_layer.layer);
	
	bmp_init_container(RESOURCE_ID_INDICATOR_LINK, &icon_link);
	layer_add_child(&indicators_layer.layer, &icon_link.layer.layer);
	layer_set_frame(&icon_link.layer.layer, LINK_FRAME);
	
	bmp_init_container(RESOURCE_ID_ERROR_1008, &icon_error);
	layer_add_child(&indicators_layer.layer, &icon_error.layer.layer);
	layer_set_frame(&icon_error.layer.layer, ERROR_FRAME);
	
	//bmp_init_container(RESOURCE_ID_BATTERY_FULL, &icon_battery);
	//layer_add_child(&indicators_layer.layer, &icon_battery.layer.layer);
	//layer_set_frame(&icon_battery.layer.layer, BATTERY_FRAME);
	
	// Add weather layer
	weather_layer_init(&weather_layer, GPoint(0, 90)); //0, 100
	layer_add_child(&window.layer, &weather_layer.layer);
	
	http_register_callbacks((HTTPCallbacks){
		.failure=failed,
		.success=success,
		.reconnect=reconnect,
		.location=location,
		.battery=battery
	}
	, (void*)ctx);
	
	// Refresh time
	get_time(&tm);
    t.tick_time = &tm;
    t.units_changed = SECOND_UNIT | MINUTE_UNIT | HOUR_UNIT | DAY_UNIT;
	
	handle_minute_tick(ctx, &t);
    http_capture_init(ctx);
}

/* Shut down the application
*/
void handle_deinit(AppContextRef ctx)
{
    fonts_unload_custom_font(font_date);
    fonts_unload_custom_font(font_hour);
    fonts_unload_custom_font(font_minute);
	
	weather_layer_deinit(&weather_layer);
	
	bmp_deinit_container(&icon_error);
	bmp_deinit_container(&icon_link);
	bmp_deinit_container(&icon_battery);
}


/********************* Main Program *******************/

void pbl_main(void *params)
{
    PebbleAppHandlers handlers =
    {
        .init_handler = &handle_init,
        .deinit_handler = &handle_deinit,
        .tick_info =
        {
            .tick_handler = &handle_minute_tick,
            .tick_units = MINUTE_UNIT
        },
		.messaging_info = {
			.buffer_sizes = {
				.inbound = 256,
				.outbound = 256,
			}
		}
    };

    http_capture_main(&handlers);

    app_event_loop(params, &handlers);
}

void request_weather() {
	if(!located) {
		http_location_request();
		return;
	}
	// Build the HTTP request
	DictionaryIterator *body;
	HTTPResult result = http_out_get("http://doraess.no-ip.org:3000", false, WEATHER_HTTP_COOKIE, &body);
	if(result != HTTP_OK) {
		weather_layer_set_icon(&weather_layer, WEATHER_ICON_NO_WEATHER);
		return;
	}
	dict_write_int32(body, WEATHER_KEY_LATITUDE, our_latitude);
	dict_write_int32(body, WEATHER_KEY_LONGITUDE, our_longitude);
	dict_write_cstring(body, WEATHER_KEY_UNIT_SYSTEM, UNIT_SYSTEM);
	// Send it.
	if(http_out_send() != HTTP_OK) {
		weather_layer_set_icon(&weather_layer, WEATHER_ICON_NO_WEATHER);
		return;
	}
	
}