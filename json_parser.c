#include "json_parser.h"
#include <cjson/cJSON.h>
#include <stdio.h>
#include <stdlib.h>
// Global Variables to be freed when cleanup
cJSON *root;
char *jsonString;

int write_json_to_file(cJSON *json);
void json_cleanup();
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

  int result = write_json_to_file(root);
  return EXIT_SUCCESS;
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

void json_cleanup() {
  cJSON_Delete(root);
  free(jsonString);
}
