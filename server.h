#pragma once
typedef struct {
  char* location_name;
  float latitude;;
  float longitude;
} location;

int init_socket();
int send_request();
void server_cleanup();
