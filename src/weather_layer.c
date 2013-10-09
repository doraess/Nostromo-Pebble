#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"
#include "util.h"
#include "weather_layer.h"

static uint8_t WEATHER_ICONS[] = {
	RESOURCE_ID_ICON_CLEAR_DAY,
	RESOURCE_ID_ICON_CLEAR_NIGHT,
	RESOURCE_ID_ICON_RAIN,
	RESOURCE_ID_ICON_SNOW,
	RESOURCE_ID_ICON_SLEET,
	RESOURCE_ID_ICON_WIND,
	RESOURCE_ID_ICON_FOG,
	RESOURCE_ID_ICON_CLOUDY,
	RESOURCE_ID_ICON_PARTLY_CLOUDY_DAY,
	RESOURCE_ID_ICON_PARTLY_CLOUDY_NIGHT,
	RESOURCE_ID_ICON_ERROR
};

void weather_layer_init(WeatherLayer* weather_layer, GPoint pos) {
	layer_init(&weather_layer->layer, GRect(pos.x, pos.y, 144, 80));
	
	// Add background layer
	text_layer_init(&weather_layer->temp_layer_background, GRect(0, 10, 144, 68));
	text_layer_set_background_color(&weather_layer->temp_layer_background, GColorWhite);
	layer_add_child(&weather_layer->layer, &weather_layer->temp_layer_background.layer);
	
    // Add temperature layer
	text_layer_init(&weather_layer->temp_layer, GRect(40, 12, 104, 30));
	text_layer_set_background_color(&weather_layer->temp_layer, GColorClear);
	text_layer_set_text_alignment(&weather_layer->temp_layer, GTextAlignmentCenter);
	text_layer_set_font(&weather_layer->temp_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FUTURA_25)));
	layer_add_child(&weather_layer->layer, &weather_layer->temp_layer.layer);
	
	// Add city layer
	text_layer_init(&weather_layer->city_layer, GRect(40, 40, 104, 20));
	text_layer_set_background_color(&weather_layer->city_layer, GColorClear);
	text_layer_set_text_alignment(&weather_layer->city_layer, GTextAlignmentCenter);
	text_layer_set_font(&weather_layer->city_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_CITY_FUTURA_15)));
	layer_add_child(&weather_layer->layer, &weather_layer->city_layer.layer);
	
	// Add street layer
	text_layer_init(&weather_layer->street_layer, GRect(0, 60, 144, 20));
	text_layer_set_background_color(&weather_layer->street_layer, GColorClear);
	text_layer_set_text_alignment(&weather_layer->street_layer, GTextAlignmentCenter);
	text_layer_set_font(&weather_layer->street_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_STREET_FUTURA_12)));
	layer_add_child(&weather_layer->layer, &weather_layer->street_layer.layer);
	
	// Note absence of icon layer
	weather_layer->has_weather_icon = false;
}

void weather_layer_set_icon(WeatherLayer* weather_layer, WeatherIcon icon) {
	
	// Remove any possible existing weather icon
	if(weather_layer->has_weather_icon) {
		layer_remove_from_parent(&weather_layer->icon_layer.layer.layer);
		bmp_deinit_container(&weather_layer->icon_layer);
		weather_layer->has_weather_icon = false;
	}
	
	// Add weather icon
	bmp_init_container(WEATHER_ICONS[icon], &weather_layer->icon_layer);
	layer_add_child(&weather_layer->layer, &weather_layer->icon_layer.layer.layer);
	layer_set_frame(&weather_layer->icon_layer.layer.layer, GRect(5, 15, 40, 40));
	weather_layer->has_weather_icon = true;
}

void weather_layer_set_city(WeatherLayer* weather_layer, const char *city) {
	
	// Add weather icon
	text_layer_set_font(&weather_layer->city_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_CITY_FUTURA_15)));
	text_layer_set_text_alignment(&weather_layer->city_layer, GTextAlignmentCenter);
	memset(weather_layer->city_str, 0, 20);
	memcpy(weather_layer->city_str, city, strlen(city));
	text_layer_set_text(&weather_layer->city_layer, weather_layer->city_str);
	//text_layer_set_text(&weather_layer->city_layer, city);  
}

void weather_layer_set_street(WeatherLayer* weather_layer, const char *street) {
	
	// Add weather icon
	text_layer_set_font(&weather_layer->street_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_STREET_FUTURA_12)));
	text_layer_set_text_alignment(&weather_layer->street_layer, GTextAlignmentCenter);
	memset(weather_layer->street_str, 0, 40);
	memcpy(weather_layer->street_str, street, strlen(street));
	text_layer_set_text(&weather_layer->street_layer, weather_layer->street_str);
	//text_layer_set_text(&weather_layer->street_layer, street);  
}

void weather_layer_set_temperature(WeatherLayer* weather_layer, const char *weather) {
	
	text_layer_set_font(&weather_layer->temp_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FUTURA_25)));
	text_layer_set_text_alignment(&weather_layer->temp_layer, GTextAlignmentCenter);
	text_layer_set_text(&weather_layer->temp_layer, weather);
}

void weather_layer_deinit(WeatherLayer* weather_layer) {
	if(weather_layer->has_weather_icon)
		bmp_deinit_container(&weather_layer->icon_layer);
}