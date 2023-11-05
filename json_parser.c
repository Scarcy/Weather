#include "json_parser.h"
#include "debug.h"
#include "libs/cJSON.h"
#include <locale.h> // For setlocale
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_DAYS 15
#define MAX_HOURS 24

#define END_OF_DAY                                                             \
  (weather_data) { .time = -1, .air_temperature = -999.0 }
#define END_OF_ARRAY                                                           \
  (weather_data) { .time = -2, .air_temperature = -999.0 }
#define END_OF_DAY_TIME -1
#define END_OF_DAY_TEMPERATURE -999.0

// GLOBAL VARIABLES
cJSON *root;
char *jsonString;
weather_data weather_data_array[MAX_DAYS][MAX_HOURS];
size_t total_days = 1;

// FUNCTION DECLARATIONS
char *trimQuotes(char *str);
int write_json_to_file(cJSON *json);
void json_cleanup();
int parse_weather_data();
void print_weather_data();
static char *convert_time_format_string(char *time);
struct tm convert_time_format(char *time);
char *format_time_to_string(int hour, int min);
char *format_temprature_to_string(double temp);
char *format_precipitation_to_string(double precipitation);
char *map_symbol_code_to_icon(char *symbol_code);

const char *SYMBOL_STRINGS[SYMBOL_CODE_COUNT] = {
    [thunder] = "🌩", [rain] = "🌧", [snow] = "⛄", [sleet] = "☃",
    [cloudy] = "☁",   [fair] = "🌤", [fog] = "🌫",  [clearsky] = "☀",
};

int json_parse(char *jsonstring) {
  debugprint("Start of json_parse\n");
  root = cJSON_Parse(jsonstring);
  if (root == NULL) {
    const char *error_ptr = cJSON_GetErrorPtr();
    if (error_ptr != NULL) {
      fprintf(stderr, "Error before: %s\n", error_ptr);
    }
    cJSON_Delete(root);
    return EXIT_FAILURE;
  }
  // Write the json to a file. Useful for debugging.
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

    cJSON *next_12_hours =
        cJSON_GetObjectItemCaseSensitive(data, "next_12_hours");
    cJSON *next_12_hours_summary =
        cJSON_GetObjectItemCaseSensitive(next_12_hours, "summary");

    cJSON *next_1_hours =
        cJSON_GetObjectItemCaseSensitive(data, "next_1_hours");
    cJSON *next_1_hours_summary =
        cJSON_GetObjectItemCaseSensitive(next_1_hours, "summary");

    cJSON *next_1_hours_details =
        cJSON_GetObjectItemCaseSensitive(next_1_hours, "details");
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

    cJSON *symbol_code_12_hours =
        cJSON_GetObjectItemCaseSensitive(next_12_hours_summary, "symbol_code");
    cJSON *symbol_code_1_hours =
        cJSON_GetObjectItemCaseSensitive(next_1_hours_summary, "symbol_code");

    cJSON *precipitation_amount = cJSON_GetObjectItemCaseSensitive(
        next_1_hours_details, "precipitation_amount");

    weather_data_array[i][j].time = convert_time_format(cJSON_Print(time));
    weather_data_array[i][j].air_temperature =
        cJSON_GetObjectItemCaseSensitive(details, "air_temperature")
            ->valuedouble;

    if (symbol_code_12_hours != NULL && cJSON_IsString(symbol_code_12_hours)) {
      weather_data_array[i][j].symbol_code_12_hours = strdup(
          cJSON_GetObjectItemCaseSensitive(next_12_hours_summary, "symbol_code")
              ->valuestring);
    }
    if (symbol_code_1_hours != NULL && cJSON_IsString(symbol_code_1_hours)) {
      weather_data_array[i][j].symbol_code_hourly = strdup(
          cJSON_GetObjectItemCaseSensitive(next_1_hours_summary, "symbol_code")
              ->valuestring);
    }

    if (precipitation_amount != NULL && cJSON_IsNumber(precipitation_amount)) {
      weather_data_array[i][j].precipitation =
          precipitation_amount->valuedouble;
    } else {
      debugprint("No precipitation data\n");
    }
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
void print_hline() {
  printf(
      "\n---------------------------------------------------------------------"
      "------------------\n");
}
void print_weather_data() {
  setlocale(LC_ALL, ""); // Set the locale to support Unicode characters
  for (int day = 0; day < total_days - 1; day++) {
    printf("\n");
    for (int hour = 0;; hour += 8) {
      if (weather_data_array[day][hour].time.tm_year ==
          END_OF_DAY.time.tm_year) {
        break;
      }
      // Convert struct tm to readable time string
      char hour_string[80];
      strftime(hour_string, sizeof(hour_string), "%c",
               &weather_data_array[day][hour].time);

      if (hour == 0) {
        printf("Time: %s - ", hour_string);
        char *symbol_icon = map_symbol_code_to_icon(
            weather_data_array[day][hour].symbol_code_12_hours);
        printf("%s - ", weather_data_array[day][hour].symbol_code_12_hours);
        printf("%s\n", symbol_icon);
        free(symbol_icon);
      }

      // if (hour % 5 == 0) { // Old way of limiting the amount per line
      //   printf("\n");
      // }

      // Hacky fix to stop printing the next days weather data
      // Should find a better way to do this
      if (hour > 0 && weather_data_array[day][hour].time.tm_hour == 0) {
        break;
      }

      for (int time_header = hour; time_header < hour + 8; time_header++) {
        if (weather_data_array[day][time_header].air_temperature ==
            END_OF_DAY_TEMPERATURE) {
          break;
        }
        char *hour_string = format_time_to_string(
            weather_data_array[day][time_header].time.tm_hour,
            weather_data_array[day][time_header].time.tm_min);

        printf("%-10s", hour_string);
        free(hour_string);
      }
      printf("\n");

      for (int weather_data = hour; weather_data < hour + 8; weather_data++) {
        if (weather_data_array[day][weather_data].air_temperature ==
            END_OF_DAY_TEMPERATURE) {
          break;
        }
        char *temp_string = format_temprature_to_string(
            weather_data_array[day][weather_data].air_temperature);
        printf("%-12s", temp_string);
        free(temp_string);
      }
      printf("\n");
      for (int precipitation = hour; precipitation < hour + 8;
           precipitation++) {

        if (weather_data_array[day][precipitation].air_temperature ==
            END_OF_DAY_TEMPERATURE) {
          break;
        }

        char *precip_string = format_precipitation_to_string(
            weather_data_array[day][precipitation].precipitation);

        printf("%-10s", precip_string);
        free(precip_string);
      }
      printf("\n\n");
    }
    print_hline();
  }
}
// Need to free the returned string
char *format_time_to_string(int hour, int min) {
  char *hour_string = malloc(sizeof(char) * 6);
  snprintf(hour_string, sizeof(hour_string), "%02d:%02d", hour, min);
  return hour_string;
}
char *format_temprature_to_string(double temp) {
  char *temp_string = malloc(sizeof(char) * 10);
  snprintf(temp_string, 10, "%.1lf %s", temp, "℃");
  return temp_string;
}
char *format_precipitation_to_string(double precipitation) {
  char *precip_string = malloc(sizeof(char) * 10);
  snprintf(precip_string, 10, "%.1lf %s", precipitation, "mm");
  return precip_string;
}
int containsWord(const char *str, const char *word) {
  return strstr(str, word) != NULL;
}

char *map_symbol_code_to_icon(char *symbol_code) {
  if (containsWord(symbol_code, "clearsky")) {
    return strdup(SYMBOL_STRINGS[clearsky]);
  } else if (containsWord(symbol_code, "fair")) {
    return strdup(SYMBOL_STRINGS[fair]);
  } else if (containsWord(symbol_code, "cloudy")) {
    return strdup(SYMBOL_STRINGS[cloudy]);
  } else if (containsWord(symbol_code, "snow")) {
    return strdup(SYMBOL_STRINGS[snow]);
  } else if (containsWord(symbol_code, "thunder")) {
    return strdup(SYMBOL_STRINGS[thunder]);
  } else if (containsWord(symbol_code, "rain")) {
    return strdup(SYMBOL_STRINGS[rain]);
  } else if (containsWord(symbol_code, "sleet")) {
    return strdup(SYMBOL_STRINGS[sleet]);
  } else if (containsWord(symbol_code, "fog")) {
    return strdup(SYMBOL_STRINGS[fog]);
  } else {
    return strdup(SYMBOL_STRINGS[clearsky]);
  }
}
void json_cleanup() {
  cJSON_Delete(root);
  free(jsonString);
}
