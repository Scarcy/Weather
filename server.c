#include "server.h"
#include <netdb.h>
#include <netinet/in.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
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
static SSL_CTX *global_ctx;
static SSL *global_ssl;
location *locations;

void static create_locations();
int static init_SSL();

int init_socket() {
  create_locations();

  // AF_INET = IPv4, SOCK_STREAM = TCP, 0 = TCP
  if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
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

  if ((connect(sockfd, (const struct sockaddr *)&serv_addr, sizeof(serv_addr)) <
       0)) {
    perror("Error connecting");
    return EXIT_FAILURE;
  };

  printf("Connected to %s\n", HOST);
  return EXIT_SUCCESS;
}

int send_request(enum City city) {
  printf("Start of send_request\n");
  char *request = malloc(sizeof(char) * 1024);
  sprintf(request,
          "GET /weatherapi/locationforecast/2.0/compact?lat=%.4f&lon=%.4f "
          "HTTP/1.1\r\nHost: %s\r\nUser-Agent: %s\r\n\r\n",
          locations[city].latitude, locations[city].longitude, HOST,
          USER_AGENT);

  location loc = locations[city];
  printf("Request string: %s\n", request);
  if (write(sockfd, request, strlen(request)) < 0) {
    perror("Error writing to socket");
    free(request);
    return EXIT_FAILURE;
  }
  printf("Request sent\n");
  char *response = malloc(sizeof(char) * 20000);
  ssize_t bytes_read = read(sockfd, response, 20000);
  if (bytes_read < 0) {
    perror("Error reading from socket");
    free(request);
    free(response);
    return EXIT_FAILURE;
  }
  printf("Response: %s\n", response);
  free(request);
  free(response);
  return EXIT_SUCCESS;
};

void static create_locations() {
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
int static init_SSL() {
  printf("Initializing SSL\n");
  SSL_library_init();
  SSL_load_error_strings();
  OpenSSL_add_all_algorithms();

  const SSL_METHOD *method = SSLv23_client_method();
  global_ctx = SSL_CTX_new(method);

  if (global_ctx == NULL) {
    perror("Error creating SSL context");
    ERR_print_errors_fp(stderr);
    return EXIT_FAILURE;
  }
  printf("SSL initialized\n");
  return EXIT_SUCCESS;
}
int connect_SSL() {
  printf("Connecting SSL\n");
  if (init_SSL() != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }
  global_ssl = SSL_new(global_ctx);
  SSL_set_fd(global_ssl, sockfd);

  if (SSL_connect(global_ssl) != 1) {
    perror("Error connecting SSL");
    ERR_print_errors_fp(stderr);
    return EXIT_FAILURE;
  }
  printf("Connected with %s encryption\n", SSL_get_cipher(global_ssl));
  return EXIT_SUCCESS;
}
void server_cleanup() {
  printf("Cleaning up server\n");
  free(locations);
  close(sockfd);
  if (global_ssl != NULL) {
    SSL_shutdown(global_ssl);
    SSL_free(global_ssl);
    SSL_CTX_free(global_ctx);
  }
};
