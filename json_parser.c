#include "json_parser.h"
#include <cjson/cJSON.h>
#include <locale.h> // For setlocale
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// Global Variables to be freed when cleanup
#define MAX_DAYS 15
#define MAX_HOURS 24

#define END_OF_DAY                                                             \
  (weather_data) { .time = -1, .air_temperature = -999.0 }
#define END_OF_ARRAY                                                           \
  (weather_data) { .time = -2, .air_temperature = -999.0 }

cJSON *root;
char *jsonString;
weather_data weather_data_array[MAX_DAYS][MAX_HOURS];
size_t total_days = 1;

char *trimQuotes(char *str);
int write_json_to_file(cJSON *json);
void json_cleanup();
int parse_weather_data();
void print_weather_data();
static char *convert_time_format_string(char *time);
struct tm convert_time_format(char *time);

int json_parse(char *jsonstring) {
  printf("Start of json_parse\n");
  root = cJSON_Parse(jsonstring);
  if (root == NULL) {
    const char *error_ptr = cJSON_GetErrorPtr();
    if (error_ptr != NULL) {
      fprintf(stderr, "Error before: %s\n", error_ptr);
    }
    cJSON_Delete(root);
    return EXIT_FAILURE;
  }

  // int result = write_json_to_file(root);
  parse_weather_data();
  return EXIT_SUCCESS;
}

int parse_weather_data() {
  cJSON *properties = cJSON_GetObjectItemCaseSensitive(root, "properties");
  cJSON *timeseries =
      cJSON_GetObjectItemCaseSensitive(properties, "timeseries");
  if (!cJSON_IsArray(timeseries)) {
    fprintf(stderr, "Error: timeseries is not an array\n");
    cJSON_Delete(root);
    return EXIT_FAILURE;
  }

  cJSON *entry;
  int i = 0;
  int j = 0;
  struct tm tm_temp = {0};
  tm_temp.tm_year = -1;
  cJSON_ArrayForEach(entry, timeseries) {
    cJSON *time = cJSON_GetObjectItemCaseSensitive(entry, "time");
    cJSON *data = cJSON_GetObjectItemCaseSensitive(entry, "data");
    cJSON *instant = cJSON_GetObjectItemCaseSensitive(data, "instant");
    cJSON *details = cJSON_GetObjectItemCaseSensitive(instant, "details");

    struct tm tm_time = convert_time_format(cJSON_Print(time));

    if (tm_temp.tm_year == -1) {
      tm_temp = tm_time;
    }
    // If we get to the next day, go to the next row in the array
    // This makes [i] the array for days and [j] the array for hours
    if (tm_time.tm_yday != tm_temp.tm_yday) {
      weather_data_array[i][j] = END_OF_DAY; // Add end of day marker
      i++;
      j = 0;
      tm_temp = tm_time;
    }

    weather_data_array[i][j].time = convert_time_format(cJSON_Print(time));
    weather_data_array[i][j].air_temperature =
        cJSON_GetObjectItemCaseSensitive(details, "air_temperature")
            ->valuedouble;
    j++;
  }

  // Add end of array marker for last element added
  if (j > 0) {
    weather_data_array[i][j] = END_OF_DAY;
  }

  total_days = i + 1;
  weather_data_array[total_days][0] = END_OF_ARRAY;

  return EXIT_SUCCESS;
}

static char *convert_time_format_string(char *time) {
  time = trimQuotes(time);
  struct tm tm_time = {0};
  // Convert ISO 8601 time to struct tm
  char *result = strptime(time, "%Y-%m-%dT%H:%M:%S", &tm_time);

  if (result == NULL) {
    printf("Error converting time\n");
    return NULL;
  }

  char buffer[80];

  // Convert struct tm to readable time string
  strftime(buffer, sizeof(buffer), "%c", &tm_time);

  free(time);
  return strdup(buffer);
}

struct tm convert_time_format(char *time) {
  time = trimQuotes(time);
  struct tm tm_time = {0};
  // Convert ISO 8601 time to struct tm
  char *result = strptime(time, "%Y-%m-%dT%H:%M:%S", &tm_time);

  if (result == NULL) {
    printf("Error converting time\n");
    exit(EXIT_FAILURE);
  }

  free(time);
  return tm_time;
}

int write_json_to_file(cJSON *json) {
  FILE *fp;
  fp = fopen("weather.json", "w");
  if (fp == NULL) {
    perror("Error opening file");
    return EXIT_FAILURE;
  }
  jsonString = cJSON_Print(json);
  if ((fprintf(fp, "%s", jsonString)) <= 0) {
    perror("Error writing to file");
    return EXIT_FAILURE;
  };
  fclose(fp);
  return EXIT_SUCCESS;
}
char *trimQuotes(char *str) {
  char temp[25];
  int i = 0;
  int k = 0;
  while (str[0] != '\0') {
    if (str[i] == '\0') {
      break;
    }
    if (str[i] != '"') {
      temp[k] = str[i];
      k++;
    }
    i++;
  }
  return strdup(temp);
}
void print_weather_data() {
  setlocale(LC_ALL, ""); // Set the locale to support Unicode characters
  for (int day = 0; day < total_days - 1; day++) {
    printf("\n");
    for (int hour = 0;; hour++) {
      if (weather_data_array[day][hour].time.tm_year ==
          END_OF_DAY.time.tm_year) {
        break;
      }
      // Convert struct tm to readable time string
      char hour_string[80];
      strftime(hour_string, sizeof(hour_string), "%c",
               &weather_data_array[day][hour].time);

      if (hour == 0) {
        printf("Time: %s\n", hour_string);
      }
      if (hour % 5 == 0) {
        printf("\n");
      }

      printf("%02d:%02d : ", weather_data_array[day][hour].time.tm_hour,
             weather_data_array[day][hour].time.tm_min);
      printf("%.1lf ℃ \t", weather_data_array[day][hour].air_temperature);
    }
    printf("\n");
  }
}
void json_cleanup() {
  cJSON_Delete(root);
  free(jsonString);
}
