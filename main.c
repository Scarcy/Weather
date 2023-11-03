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
int tflag = 0;
int dflag = 0;
// End Global Variables

// Function Prototypes

// Handle the command line parameters
int handle_params();
// Function to cleanup the program. Closes the socket and frees memory.
void cleanup();

int main(int argc, char *argv[]) {
  int init = init_socket();
  char *response;
  if (init != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }
  if (connect_SSL() != EXIT_SUCCESS) {
    printf("Error initializing SSL socket\n");
    return EXIT_FAILURE;
  }

  // send_request(SARPSBORG);
  int status = send_ssl_request(SARPSBORG, &response);
  printf("Status: %d\n", status);
  if (status == EXIT_SUCCESS) {
    printf("Before json_parse\n");
    json_parse(response);
    free(response);
  }
  print_weather_data();
  server_cleanup();
  json_cleanup();
  return EXIT_SUCCESS;
}
