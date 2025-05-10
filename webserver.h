#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>



int create_socket(int port);
int bind_socket(int server_fd, struct sockaddr_in address);
int start_listening(int server_fd);
int accept_connection(int server_fd);

//handling
void handle_request(int client_socket);
void parse_request(char *request, char *method, char *path, char *version);
int is_path_safe(char *path);

// rsponse
void send_response(int client_socket, int status, char *status_text, 
                   char *content_type, char *content, size_t content_length);
void send_error(int client_socket, int status, char *status_text, char *message);

// path handlers
void handle_static(int client_socket, char *path);
void handle_calc(int client_socket, char *path);

char* get_content_type(char *path);

void *client_thread(void *arg);

#endif