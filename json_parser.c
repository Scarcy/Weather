#include "json_parser.h"
#include <cjson/cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// Global Variables to be freed when cleanup

cJSON *root;
char *jsonString;
weather_data *weather_data_array;
size_t weather_data_array_size = 0;

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

  weather_data_array =
      malloc(sizeof(weather_data) * cJSON_GetArraySize(timeseries));
  cJSON *entry;
  int i = 0;
  cJSON_ArrayForEach(entry, timeseries) {
    cJSON *time = cJSON_GetObjectItemCaseSensitive(entry, "time");
    cJSON *data = cJSON_GetObjectItemCaseSensitive(entry, "data");
    cJSON *instant = cJSON_GetObjectItemCaseSensitive(data, "instant");
    cJSON *details = cJSON_GetObjectItemCaseSensitive(instant, "details");

    weather_data_array[i].time = convert_time_format(cJSON_Print(time));
    weather_data_array[i].air_temperature =
        cJSON_GetObjectItemCaseSensitive(details, "air_temperature")
            ->valuedouble;
    weather_data_array_size++;
    i++;
  }
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
  for (int i = 0; i < weather_data_array_size; i++) {
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%c", &weather_data_array[i].time);
    printf("Time: %s\n", buffer);
    printf("Hour: %d\n", weather_data_array[i].time.tm_hour);
    printf("Air Temperature: %.1lf C\n", weather_data_array[i].air_temperature);
    printf("\n");
  }
}
void json_cleanup() {
  cJSON_Delete(root);
  free(jsonString);
  free(weather_data_array);
}
