#ifndef WEATHER_LAYER_H
#define WEATHER_LAYER_H

typedef struct {
	Layer layer;
	BmpContainer icon_layer;
	TextLayer city_layer;
	TextLayer street_layer;
	TextLayer temp_layer;
	TextLayer temp_layer_background;
	bool has_weather_icon;
	char weather_str[10];
	char city_str[20];
	char street_str[40];
} WeatherLayer;

typedef enum {
	WEATHER_ICON_CLEAR_DAY = 0,
	WEATHER_ICON_CLEAR_NIGHT,
	WEATHER_ICON_RAIN,
	WEATHER_ICON_SNOW,
	WEATHER_ICON_SLEET,
	WEATHER_ICON_WIND,
	WEATHER_ICON_FOG,
	WEATHER_ICON_CLOUDY,
	WEATHER_ICON_PARTLY_CLOUDY_DAY,
	WEATHER_ICON_PARTLY_CLOUDY_NIGHT,
	WEATHER_ICON_NO_WEATHER,
	WEATHER_ICON_APP_ERROR,
	WEATHER_ICON_INET_ERROR,
	WEATHER_ICON_LINK_ERROR,
	WEATHER_ICON_COUNT,
} WeatherIcon;

void weather_layer_init(WeatherLayer* weather_layer, GPoint pos);
void weather_layer_deinit(WeatherLayer* weather_layer);
void weather_layer_set_icon(WeatherLayer* weather_layer, WeatherIcon icon);
void weather_layer_set_temperature(WeatherLayer* weather_layer, const char *weather);
void weather_layer_set_city(WeatherLayer* weather_layer, const char *city);
void weather_layer_set_street(WeatherLayer* weather_layer, const char *street);

#endif
