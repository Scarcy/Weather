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
  if (init != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }
  // if (connect_SSL() != EXIT_SUCCESS) {
  //   printf("Error initializing SSL socket\n");
  //   return EXIT_FAILURE;
  // }

  send_request(SARPSBORG);
  server_cleanup();
  return EXIT_SUCCESS;
}
