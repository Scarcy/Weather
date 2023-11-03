#include <time.h>

typedef struct {
  struct tm time;
  float air_temperature;
} weather_data;
// Parse a JSON string into a JSON object.
// Returns
int json_parse(char* json_string);
void json_cleanup();
void print_weather_data();
