#include "debug.h"
#include "flags.h"
#include "json_parser.h"
#include "server.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
// Global Variables

// Handle the command line parameters
int handle_params(int argc, char *argv[]);
// Function to cleanup the program. Closes the socket and frees memory.
void cleanup();

int main(int argc, char *argv[]) {
  handle_params(argc, argv);

  int init = init_socket();
  char *response;
  if (init != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }
  if (connect_SSL() != EXIT_SUCCESS) {
    printf("Error initializing SSL socket\n");
    return EXIT_FAILURE;
  }
  int status;
  // send_request(SARPSBORG);
  if (sarp_flag) {
    status = send_ssl_request(SARPSBORG, &response);
  } else if (coordinate_flag) {
    status = send_ssl_request_coordinates(clatitude, clongitude, &response);
  } else {
    status = send_ssl_request(SARPSBORG, &response);
  }
  printf("Status: %d\n", status);
  if (status == EXIT_SUCCESS) {
    debugprint("Before json_parse\n");
    json_parse(response);
    free(response);
  }
  print_weather_data();
  server_cleanup();
  json_cleanup();
  return EXIT_SUCCESS;
}

int handle_params(int argc, char *argv[]) {
  int c;

  while ((c = getopt(argc, argv, "dc:s")) != -1) {
    switch (c) {
    case 'd':
      debug_flag = 1;
      break;
    case 's':
      sarp_flag = 1;
      break;
    case 'c':;
      char *token = strtok(optarg, ",");
      if (token != NULL) {
        clatitude = atof(token);
        token = strtok(NULL, ",");
        if (token != NULL) {
          clongitude = atof(token);
        } else {
          fprintf(stderr, "Invalid coordinates: Missing Longitude\n");
          return 1;
        }
      } else {
        fprintf(stderr, "Invalid coordinates: Missing Latitude\n");
        return 1;
      }
      coordinate_flag = 1;
      break;
    case '?':
      if (optopt == 'c') {
        fprintf(stderr, "Option -%c requires an argument.\n", optopt);
      } else if (isprint(optopt)) {
        fprintf(stderr, "Unknown option '-%c'.\n", optopt);
      } else {
        fprintf(stderr, "Unknown option character '\\x%x'.\n", optopt);
        return 1;
      }
    default:
      abort();
    }
  }
  return 0;
}
