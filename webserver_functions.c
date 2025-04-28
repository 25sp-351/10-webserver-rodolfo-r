#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "webserver.h"

int create_socket(int port) {
    int server_fd;
    int opt = 1;
    
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("Setsockopt failed");
        exit(EXIT_FAILURE);
    }
    
    return server_fd;
}

int bind_socket(int server_fd, struct sockaddr_in address) {
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
    return 0;
}

int start_listening(int server_fd) {
    if (listen(server_fd, 10) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }
    return 0;
}

int accept_connection(int server_fd) {
    struct sockaddr_in client_addr;
    int addrlen = sizeof(client_addr);
    int new_socket;
    
    if ((new_socket = accept(server_fd, (struct sockaddr *)&client_addr, (socklen_t*)&addrlen)) < 0) {
        perror("Accept failed");
        return -1;
    }
    
    return new_socket;
}

void parse_request(char *request, char *method, char *path, char *version) {
    // extract the first line
    char *line_end = strstr(request, "\r\n");
    if (line_end) {
        *line_end = '\0';
        sscanf(request, "%15s %1023s %15s", method, path, version);
        *line_end = '\r'; // restore
    }
}

int is_path_safe(char *path) {
    return (strstr(path, "..") == NULL);
}

char* get_content_type(char *path) {
    char *ext = strrchr(path, '.');
    if (!ext) return "application/octet-stream";
    
    if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0) 
        return "text/html";
    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) 
        return "image/jpeg";
    if (strcmp(ext, ".png") == 0) 
        return "image/png";
    if (strcmp(ext, ".ico") == 0) 
        return "image/x-icon";
    if (strcmp(ext, ".txt") == 0) 
        return "text/plain";
    if (strcmp(ext, ".css") == 0) 
        return "text/css";
    if (strcmp(ext, ".js") == 0) 
        return "application/javascript";
    
    return "application/octet-stream";
}

void send_response(int client_socket, int status, char *status_text, 
                  char *content_type, char *content, size_t content_length) {
    char header[1024];
    
    // Format HTTP response header
    sprintf(header, "HTTP/1.1 %d %s\r\nContent-Type: %s\r\nContent-Length: %zu\r\n\r\n", 
            status, status_text, content_type, content_length);
    
    // Send header
    write(client_socket, header, strlen(header));
    
    // Send content
    write(client_socket, content, content_length);
}

void send_error(int client_socket, int status, char *status_text, char *message) {
    char content[512];
    sprintf(content, "<html><body><h1>%d %s</h1><p>%s</p></body></html>", 
            status, status_text, message);
    
    send_response(client_socket, status, status_text, "text/html", 
                 content, strlen(content));
}

void handle_static(int client_socket, char *path) {
    char full_path[1024] = {0};
    struct stat file_stat;
    
    // Remove /static/ prefix and build full path
    snprintf(full_path, sizeof(full_path), "static/%s", path + 8);
    
    // Check path safety
    if (!is_path_safe(full_path)) {
        send_error(client_socket, 403, "Forbidden", "Access denied");
        return;
    }
    
    // Checks if file exists
    if (stat(full_path, &file_stat) < 0) {
        send_error(client_socket, 404, "Not Found", "File not found");
        return;
    }
    
    // Checks if it's a regular file
    if (!S_ISREG(file_stat.st_mode)) {
        send_error(client_socket, 403, "Forbidden", "Not a file");
        return;
    }
    
    // open file
    int fd = open(full_path, O_RDONLY);
    if (fd < 0) {
        send_error(client_socket, 500, "Internal Server Error", "Failed to open file");
        return;
    }
    
    // Get content type
    char *content_type = get_content_type(full_path);
    
    // Allocate buffer for file contents
    char *buffer = malloc(file_stat.st_size);
    if (!buffer) {
        close(fd);
        send_error(client_socket, 500, "Internal Server Error", "Memory allocation failed");
        return;
    }
    
    // read file
    ssize_t bytes_read = read(fd, buffer, file_stat.st_size);
    if (bytes_read != file_stat.st_size) {
        close(fd);
        free(buffer);
        send_error(client_socket, 500, "Internal Server Error", "Failed to read file");
        return;
    }
    
    close(fd);
    
    // send response
    send_response(client_socket, 200, "OK", content_type, buffer, file_stat.st_size);
    free(buffer);
}

void handle_calc(int client_socket, char *path) {
    // Skip "/calc/"
    path += 6;
    
    char operation[10] = {0};
    double num1, num2, result;
    char result_html[1024] = {0};
    
    //add, mult, div
    char *op_end = strchr(path, '/');
    if (!op_end) {
        send_error(client_socket, 400, "Bad Request", "Invalid calculation format");
        return;
    }
    
    strncpy(operation, path, op_end - path);
    operation[op_end - path] = '\0';
    
    path = op_end + 1;
    
    // extract first number
    char *num1_end = strchr(path, '/');
    if (!num1_end) {
        send_error(client_socket, 400, "Bad Request", "Invalid calculation format");
        return;
    }
    
    *num1_end = '\0';  //terminate string
    num1 = atof(path);
    *num1_end = '/';   // restore
    
    // Move to second number
    path = num1_end + 1;
    
    // Extract second number
    num2 = atof(path);
    
    // calculation
    if (strcmp(operation, "add") == 0) {
        result = num1 + num2;
        sprintf(result_html, "<html><body>%.2f + %.2f = %.2f</body></html>", 
                num1, num2, result);
    }
    else if (strcmp(operation, "mult") == 0 || strcmp(operation, "mul") == 0) {
        result = num1 * num2;
        sprintf(result_html, "<html><body>%.2f * %.2f = %.2f</body></html>", 
                num1, num2, result);
    }
    else if (strcmp(operation, "div") == 0) {
        if (num2 == 0) {
            send_error(client_socket, 400, "Bad Request", "Division by zero");
            return;
        }
        result = num1 / num2;
        sprintf(result_html, "<html><body>%.2f / %.2f = %.2f</body></html>", 
                num1, num2, result);
    }
    else {
        send_error(client_socket, 400, "Bad Request", "Unknown operation");
        return;
    }
    
    //result
    send_response(client_socket, 200, "OK", "text/html", result_html, strlen(result_html));
}

void handle_request(int client_socket) {
    char buffer[4096] = {0};
    char method[16] = {0};
    char path[1024] = {0};
    char version[16] = {0};
    
    // read request
    ssize_t bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);
    if (bytes_read <= 0) return;
    
    // ensure null termination
    buffer[bytes_read] = '\0';
    
    // Parse request
    parse_request(buffer, method, path, version);
    
    printf("Received request: %s %s %s\n", method, path, version);
    
    // Check if method is GET
    if (strcmp(method, "GET") != 0) {
        send_error(client_socket, 405, "Method Not Allowed", "Only GET method is supported");
        return;
    }
    
    // Route based on path
    if (strncmp(path, "/calc/", 6) == 0) {
        handle_calc(client_socket, path);
    }
    else if (strncmp(path, "/static/", 8) == 0) {
        handle_static(client_socket, path);
    }
    else {
        send_error(client_socket, 404, "Not Found", "Resource not found");
    }
}

void *client_thread(void *arg) {
    int client_socket = *((int *)arg);
    free(arg);
    
    handle_request(client_socket);
    close(client_socket);
    
    return NULL;
}