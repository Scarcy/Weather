#include <ctype.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// Global Variables
int tflag = 0;
int dflag = 0;
int sockfd;
struct sockaddr_in serv_addr;
struct hostent *server;
// End Global Variables

// Function Prototypes

// Sends a request to the API
int api_request();
// Initializes the socket
int init_socket();
// Handle the command line parameters
int handle_params();
// Function to cleanup the program. Closes the socket and frees memory.
void cleanup();

int main(int argc, char *argv[]) { return EXIT_SUCCESS; }
