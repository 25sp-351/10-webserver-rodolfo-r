#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include "webserver.h"

int main(int argc, char *argv[]) {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int port = 80;
    int opt;
    
    // Parse command line arguments
    while ((opt = getopt(argc, argv, "p:")) != -1) {
        switch (opt) {
            case 'p':
                port = atoi(optarg);
                break;
            default:
                fprintf(stderr, "Usage: %s [-p port]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }
    
    // Create socket
    server_fd = create_socket(port);
    
    // Setup address structure
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    
    // Bind and listen
    bind_socket(server_fd, address);
    start_listening(server_fd);
    
    printf("Server listening on port %d\n", port);
    
    signal(SIGINT, SIG_IGN);
    
    while (1) {
        client_socket = accept_connection(server_fd);
        if (client_socket >= 0) {
            pthread_t thread;
            int *client_sock = malloc(sizeof(int));
            *client_sock = client_socket;
            
            if (pthread_create(&thread, NULL, client_thread, client_sock) != 0) {
                perror("Thread creation failed");
                free(client_sock);
                close(client_socket);
            } else {
                pthread_detach(thread);
            }
        }
    }
    
    close(server_fd);
    return 0;
}