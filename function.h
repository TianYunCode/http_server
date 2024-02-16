#include "head.h"

int get_line(int sock,char* buf,int size);
void* do_http_request(void* pclient_sock);
void do_http_response(int client_sock,const char* path);
void not_found(client_sock);//404
void unimplemented(int client_sock);//501
void bad_request(int client_sock);//400
int headers(int client_sock,FILE* resource);
void cat(int client_sock,FILE* resource);
void inner_error(int client_sock);