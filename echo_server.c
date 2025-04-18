#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <getopt.h>
#include <pthread.h>

#define DEFAULT_PORT 2345
#define BUFFER_SIZE 1024

typedef struct {
    int client_fd;
    int verbose;
} ThreadArgs;

// Function handling client connection
void* handleConnection(void* sock_fd_ptr) {
    ThreadArgs* args = (ThreadArgs*)sock_fd_ptr;
    int client_fd = args->client_fd;
    int verbose = args->verbose;
    
    free(args);
    
    printf("Handling connection on %d\n", client_fd);
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    
    // echo loop
    while (1) {
        // reading data
        bytes_read = read(client_fd, buffer, BUFFER_SIZE - 1);
        
        if (bytes_read <= 0) {
            break;
        }
        
        buffer[bytes_read] = '\0';
        
        // verbose
        if (verbose) {
            printf("Received: %s", buffer);
            // Add newline if not present
            if (bytes_read > 0 && buffer[bytes_read - 1] != '\n') {
                printf("\n");
            }
        }
        
        write(client_fd, buffer, bytes_read);
    }
    
    close(client_fd);
    printf("Done with connection %d\n", client_fd);
    
    return NULL;
}

// main
int main(int argc, char *argv[]) {
    int socket_fd;
    struct sockaddr_in socket_address;
    struct sockaddr_in client_address;
    socklen_t client_address_len = sizeof(client_address);
    int port = DEFAULT_PORT;
    int verbose = 0;
    int opt;
    
    while ((opt = getopt(argc, argv, "p:v")) != -1) {
        switch (opt) {
            case 'p':
                port = atoi(optarg);
                break;
            case 'v':
                verbose = 1;
                break;
            default:
                fprintf(stderr, "Usage: %s [-p port] [-v]\n", argv[0]);
                return 1;
        }
    }
    
    // TCP socket
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        perror("socket");
        return 1;
    }
    
    // Set socket option to reuse address
    int reuse = 1;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("setsockopt");
        return 1;
    }
    
    memset(&socket_address, 0, sizeof(socket_address));
    socket_address.sin_family = AF_INET;
    socket_address.sin_addr.s_addr = htonl(INADDR_ANY);
    socket_address.sin_port = htons(port);
    
    printf("Binding to port %d\n", port);
    
    int returnval;
    returnval = bind(
        socket_fd,
        (struct sockaddr*)&socket_address,
        sizeof(socket_address));
    
    if (returnval < 0) {
        perror("bind");
        return 1;
    }
    
    returnval = listen(socket_fd, 5);
    if (returnval < 0) {
        perror("listen");
        return 1;
    }
    
    printf("Threaded echo server started on port %d\n", port);
    if (verbose) {
        printf("Verbose mode enabled\n");
    }
    
    while (1) {
        pthread_t thread;
        int* client_fd_buf = malloc(sizeof(int));
        
        // Accept connection
        *client_fd_buf = accept(
            socket_fd,
            (struct sockaddr*)&client_address,
            &client_address_len);
        
        if (*client_fd_buf < 0) {
            perror("accept");
            free(client_fd_buf);
            continue;
        }
        
        printf("Client connected: %s:%d\n", 
               inet_ntoa(client_address.sin_addr), 
               ntohs(client_address.sin_port));
               
        // Prepare thread arg.
        ThreadArgs* args = malloc(sizeof(ThreadArgs));
        args->client_fd = *client_fd_buf;
        args->verbose = verbose;
        free(client_fd_buf); 
        
        // create a new thread for each client connection
        pthread_create(&thread, NULL, (void* (*)(void*))handleConnection, (void*)args);
        
        // cleans up automatically
        pthread_detach(thread);
        
        printf("Created thread for client\n");
    }
    
    close(socket_fd);
    return 0;
}