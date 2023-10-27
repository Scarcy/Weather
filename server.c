#include "server.h"
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT 80
#define HOST "api.met.no"
#define USER_AGENT "weatherapp/0.1 https://github.com/Scarcy/Weather"

static int sockfd;
static struct sockaddr_in serv_addr;
static struct hostent *server;

location *locations;

void create_locations() {
  locations = malloc(sizeof(location) * 3);
  // Høyås: 59.274690869472174, 11.06301547334504
  // Halden: 59.12924749999774, 11.353318284133278
  // Oslo: 59.912726, 10.746092

  locations[0].location_name = "Sarpsborg";
  locations[0].latitude = 59.2746;
  locations[0].longitude = 11.0630;
  locations[1].location_name = "Halden";
  locations[1].latitude = 59.1292;
  locations[1].longitude = 11.3533;
  locations[2].location_name = "Oslo";
  locations[2].latitude = 59.9127;
  locations[2].longitude = 10.7461;
}
int init_socket() {
  // AF_INET = IPv4, SOCK_STREAM = TCP, 0 = TCP
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0) < 0)) {
    perror("Error opening socket");
    return EXIT_FAILURE;
  };
  if ((server = gethostbyname(HOST)) == NULL) {
    herror("Error getting hostname");
    return EXIT_FAILURE;
  };

  memset((char *)&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  memcpy((char *)&serv_addr.sin_addr.s_addr, (char *)server->h_addr,
         server->h_length);
  serv_addr.sin_port = htons(PORT);

  if ((connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)) {
    perror("Error connecting");
    return EXIT_FAILURE;
  };
  return EXIT_SUCCESS;
}
int api_request(location local, char *response) {
  int a;
  return EXIT_SUCCESS;
};
void server_cleanup() { free(locations); };
