#include "server.h"
#include "debug.h"
#include "flags.h"
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT 443
#define HOST "api.met.no"
#define USER_AGENT "weatherapp/0.1 https://github.com/Scarcy/Weather"
#define ACCEPT "application/json;charset=utf-8"
#define CHUNK_SIZE 4096
static int sockfd;
static struct sockaddr_in serv_addr;
static struct hostent *server;
static SSL_CTX *global_ctx;
static SSL *global_ssl;
location *locations;

char *split_response_string(unsigned char *http_response);
char *allocate_response_string(unsigned char *http_response);
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

  debugprint("Connected to Host");
  return EXIT_SUCCESS;
}

int send_request(enum City city) {
  debugprint("Start of send_request\n");
  char *request = malloc(sizeof(char) * 1024);
  sprintf(request,
          "GET /weatherapi/locationforecast/2.0/compact?lat=%.4f&lon=%.4f "
          "HTTP/1.1\r\nHost: %s\r\nUser-Agent: %s\r\n\r\n",
          locations[city].latitude, locations[city].longitude, HOST,
          USER_AGENT);

  location loc = locations[city];
  debugprint(request);
  if (write(sockfd, request, strlen(request)) < 0) {
    perror("Error writing to socket");
    free(request);
    return EXIT_FAILURE;
  }
  debugprint("Request sent\n");
  char *response = malloc(sizeof(char) * 20000);
  ssize_t bytes_read = read(sockfd, response, 20000);
  if (bytes_read < 0) {
    perror("Error reading from socket");
    free(request);
    free(response);
    return EXIT_FAILURE;
  }
  debugprint(response);
  free(request);
  free(response);
  return EXIT_SUCCESS;
};

int send_ssl_request(enum City city, char **response) {
  debugprint("Start of send_request\n");
  char *request = malloc(sizeof(char) * 1024);
  sprintf(request,
          "GET /weatherapi/locationforecast/2.0/compact.json?lat=%.4f&lon=%.4f "
          "HTTP/1.1\r\nHost: %s\r\nAccept: %s\r\nUser-Agent: %s\r\nConnection: "
          "close\r\n\r\n",
          locations[city].latitude, locations[city].longitude, HOST, ACCEPT,
          USER_AGENT);

  location loc = locations[city];
  debugprint(request);
  int bytes_written = SSL_write(global_ssl, request, strlen(request));
  if (bytes_written < 0) {
    perror("Error writing to socket");
    ERR_print_errors_fp(stderr);
    free(request);
    return EXIT_FAILURE;
  }
  debugprint("Request sent\n");

  unsigned char *largeBuffer = NULL;
  size_t totalBytesRead = 0;
  size_t bufferOffset = 0;

  while (1) { // Infinite loop to keep reading until we have all data
    // Reallocate memory to hold the next chunk of data
    unsigned char *temp = realloc(largeBuffer, totalBytesRead + CHUNK_SIZE);
    if (temp == NULL) {
      perror("Failed to realloc memory");
      exit(EXIT_FAILURE);
    }
    largeBuffer = temp;

    // Read data into the buffer at the current offset
    int bytes_read =
        SSL_read(global_ssl, largeBuffer + bufferOffset, CHUNK_SIZE);

    if (bytes_read > 0) {
      // Successfully read `bytes_read` bytes
      totalBytesRead += bytes_read;
      bufferOffset += bytes_read; // Move the buffer offset for the next read

      // Do any additional check here to see if you've received all the data,
      // such as looking for a closing tag in a JSON string, etc.

    } else if (bytes_read == 0) {
      // Connection was closed by peer
      debugprint("Connection was closed by peer.\n");
      break;
    } else {
      // An error occurred
      printf("SSL_read failed. Error: %d.\n",
             SSL_get_error(global_ssl, bytes_read));
      break;
    }
  }

  // NULL-terminate the buffer, assuming it's a string
  largeBuffer[totalBytesRead] = '\0';
  // printf("Response: %s\n", largeBuffer);
  free(request);
  *response = split_response_string(largeBuffer);
  free(largeBuffer);
  return EXIT_SUCCESS;
}
int send_ssl_request_coordinates(float latitude, float longitude,
                                 char **response) {
  debugprint("Start of send_request\n");
  char *request = malloc(sizeof(char) * 1024);
  sprintf(request,
          "GET /weatherapi/locationforecast/2.0/compact.json?lat=%.4f&lon=%.4f "
          "HTTP/1.1\r\nHost: %s\r\nAccept: %s\r\nUser-Agent: %s\r\nConnection: "
          "close\r\n\r\n",
          latitude, longitude, HOST, ACCEPT, USER_AGENT);

  debugprint(request);
  int bytes_written = SSL_write(global_ssl, request, strlen(request));
  if (bytes_written < 0) {
    perror("Error writing to socket");
    ERR_print_errors_fp(stderr);
    free(request);
    return EXIT_FAILURE;
  }
  debugprint("Request sent\n");

  unsigned char *largeBuffer = NULL;
  size_t totalBytesRead = 0;
  size_t bufferOffset = 0;

  while (1) { // Infinite loop to keep reading until we have all data
    // Reallocate memory to hold the next chunk of data
    unsigned char *temp = realloc(largeBuffer, totalBytesRead + CHUNK_SIZE);
    if (temp == NULL) {
      perror("Failed to realloc memory");
      exit(EXIT_FAILURE);
    }
    largeBuffer = temp;

    // Read data into the buffer at the current offset
    int bytes_read =
        SSL_read(global_ssl, largeBuffer + bufferOffset, CHUNK_SIZE);

    if (bytes_read > 0) {
      // Successfully read `bytes_read` bytes
      totalBytesRead += bytes_read;
      bufferOffset += bytes_read; // Move the buffer offset for the next read

      // Do any additional check here to see if you've received all the data,
      // such as looking for a closing tag in a JSON string, etc.

    } else if (bytes_read == 0) {
      // Connection was closed by peer
      debugprint("Connection was closed by peer.\n");
      break;
    } else {
      // An error occurred
      printf("SSL_read failed. Error: %d.\n",
             SSL_get_error(global_ssl, bytes_read));
      break;
    }
  }

  // NULL-terminate the buffer, assuming it's a string
  largeBuffer[totalBytesRead] = '\0';
  // printf("Response: %s\n", largeBuffer);
  free(request);
  *response = split_response_string(largeBuffer);
  free(largeBuffer);
  return EXIT_SUCCESS;
}
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
  debugprint("Initializing SSL\n");
  SSL_library_init();
  SSL_load_error_strings();
  OpenSSL_add_all_algorithms();

  const SSL_METHOD *method = TLS_client_method();
  global_ctx = SSL_CTX_new(method);

  if (global_ctx == NULL) {
    perror("Error creating SSL context");
    ERR_print_errors_fp(stderr);
    return EXIT_FAILURE;
  }
  debugprint("SSL initialized\n");
  return EXIT_SUCCESS;
}
int connect_SSL() {
  debugprint("Connecting SSL\n");
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
  debugprint("Connected with %s encryption");
  debugprint((char *)SSL_get_cipher(global_ssl));
  return EXIT_SUCCESS;
}

char *split_response_string(unsigned char *http_response) {
  char *header_end = strstr((char *)http_response, "\r\n\r\n");

  if (header_end) {
    header_end += 4;

    char *json_payload = strdup(header_end);
    return json_payload;
  } else {
    printf("Error splitting response string\n");
    return NULL;
  }
}
// SEEMINGLY REDUNTANT SINCE STRDUP DOES THE SAME THING, I THINK
// char *allocate_response_string(unsigned char *http_response) {
//   const char *content_length_header = "Content-Length: ";
//   char *found = strstr((char *)http_response, content_length_header);
//   int content_length = 0;
//
//   if (found) {
//     sscanf(found + strlen(content_length_header), "%d", &content_length);
//     printf("Content-Length: %d\n", content_length);
//   } else {
//     printf("Content-Length wasn't found, so we can't allocate memory for the
//     "
//            "response string\n");
//     exit(EXIT_FAILURE);
//   }
//
//   char *response = malloc(sizeof(char) * content_length);
//   response = split_response_string(http_response);
//   return response;
// }
void server_cleanup() {
  debugprint("Cleaning up server\n");
  free(locations);
  close(sockfd);
  if (global_ssl != NULL) {
    SSL_shutdown(global_ssl);
    SSL_free(global_ssl);
    SSL_CTX_free(global_ctx);
  }
};
