typedef struct {
  char* location_name;
  float latitude;;
  float longitude;
} location;
enum City {
  SARPSBORG = 0,
  HALDEN,
  OSLO
};

int init_socket();
int init_SSL_socket();
int connect_SSL();
int send_request(enum City location);
int send_ssl_request(enum City location, char** response);
int send_ssl_request_coordinates(float latitude, float longitude, char** response);
void server_cleanup();
