#include <time.h>
enum weather_symbolcodes {
  thunder,
  rain,
  snow,
  sleet,
  cloudy,
  fair,
  fog,
  clearsky,
  SYMBOL_CODE_COUNT
};
typedef struct {
  struct tm time;
  float air_temperature;
  float precipitation;
  char* symbol_code_12_hours;
  char* symbol_code_hourly;
} weather_data;

// Parse a JSON string into a JSON object.
// Returns
int json_parse(char* json_string);
void json_cleanup();
void print_weather_data();
