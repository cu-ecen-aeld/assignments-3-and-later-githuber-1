#include <syslog.h>
#include <unistd.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <arpa/inet.h>

volatile sig_atomic_t stop = 0;

void handle_signal(int signo)
{
    if (signo == SIGINT || signo == SIGTERM)
    {
        stop = 1;
    }   
}

int main(int argc, char *argv[])
{
    int run_as_daemon = 0;

    // Check command-line args
    if (argc == 2 && strcmp(argv[1], "-d") == 0) {
        run_as_daemon = 1;
    }

    // register signal handlers
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    // Open syslog for later use
    openlog("aesdsocket", 0, LOG_USER);

    // File name for storing and reading back socket data
    const char *filename = "/var/tmp/aesdsocketdata";

    // Opens a SOCK_STREAM bound to port 9000, failing and returning 
    // -1 if any of the socket connection steps fail
    struct addrinfo hints, *res;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    // Get address info for port 9000
    if (getaddrinfo(NULL, "9000", &hints, &res) != 0)
    {
        perror("getaddrinfo");
        return -1;
    }

    // Create IPV4, TCP socket file descriptor
    int server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_fd == -1)
    {
        perror("socket");
        return -1;
    }
    printf("server socket fd %d\n", server_socket_fd);

    int one = 1;
    if (setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int)) == -1)
    {
        perror("setsockopt");
        printf("setsockopt failed\n");
        close(server_socket_fd);
        return -1;
    }

    // Bind the fd to the address for port 9000
    if (bind(server_socket_fd,
             (struct sockaddr *) res->ai_addr, 
             res->ai_addrlen) != 0)
    {
        perror("bind");
        close(server_socket_fd);
        freeaddrinfo(res);
        return -1;       
    }

    // freeaddrinfo after bind or we'll have a memory link
    freeaddrinfo(res);

    if (run_as_daemon)
    {
        pid_t pid = fork();
        if (pid < 0)
        {
            // Fork failed
            perror("fork");
            close(server_socket_fd);
            exit(EXIT_FAILURE);
        }
        if (pid > 0)
        {
            // Parent can exit
            exit(EXIT_SUCCESS);
        }
        
        if (setsid() < 0)
        {
            perror("setsid");
            exit(EXIT_FAILURE);
        }

        // Redirect fd to /dev/null
        fclose(stdin);
        fclose(stdout);
        fclose(stderr);
        open("/dev/null", O_RDONLY); // stdin
        open("/dev/null", O_WRONLY); // stdout
        open("/dev/null", O_RDWR);   // stderr
    }

    struct sockaddr client_addr;
    socklen_t client_len = sizeof(client_addr);

    // Listens for and accepts a connection
    if (listen(server_socket_fd, 10) == -1)
    {
        perror("listen");
        close(server_socket_fd);
        return -1;
    }
    printf("Waiting for clients on port 9000\n");
    
    while (!stop)
    {
        int client_fd = accept(server_socket_fd,
                                        &client_addr,
                                        &client_len);
        if (client_fd == -1)
        {
            perror("accept");
            close(server_socket_fd);
            return -1;
        }

        // get client address
        char ip_str[INET_ADDRSTRLEN];
        void *addr;
        struct sockaddr_in *ipv4 = (struct sockaddr_in *)&client_addr;
        addr = &(ipv4->sin_addr);

        inet_ntop(AF_INET, addr, ip_str, sizeof(ip_str));

        // Logs message to the syslog "Accepted connection from XXXX," where XXXX is 
        // the IP of the connected client
        printf("Accepted connection from %s\n", ip_str);
        syslog(LOG_INFO, "Accepted connection from %s", ip_str);

        // Buffer to hold entire packet
        char *packet_buf = NULL;
        size_t packet_buf_size = 0;

        // Buffer to hold individual reads
        char read_buf[1024];
        ssize_t bytes_read;
        
        // Read from socket until newline or oversized packet
        int packet_finished = 0;
        while (!packet_finished && (bytes_read = recv(client_fd,
                                                      read_buf,
                                                      sizeof(read_buf),
                                                      0)) > 0)
        {
            char *temp = realloc(packet_buf, packet_buf_size + bytes_read + 1);
            if (!temp)
            {
                perror("Malloc failed, discarding packet");
                free(packet_buf);
                packet_buf = NULL;
                break;
            }

            packet_buf = temp;
            memcpy(packet_buf + packet_buf_size, read_buf, bytes_read);
            packet_buf_size += bytes_read;

            // Detect end of packagt
            if (strchr(read_buf, '\n') != NULL)
            {
                packet_finished = 1;
            }
        }

        // If reads are successful, packet_buf will not be null here
        if (packet_buf)
        {
            // append data to file /var/tmp/aesdsocketdata, 
            // creates this file if it doesn't exist
            int log_fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (log_fd == -1)
            {
                perror("open log file");
                close(client_fd);
                return -1;
            }

            printf("Packet buf exists after reads\n");
            // FILE *file = fopen(filename, "a");
            // if (!file)
            // {
            //     perror("fopen");
            //     printf("fileopen failed\n");
            // } 
            // else
            // {
            printf("Writing file\n");
            printf("Packet content (length %zu):\n%.*s\n", 
                    packet_buf_size, (int)packet_buf_size, packet_buf);
                // fwrite(packet_buf, 1, packet_buf_size, file);
                // fclose(file);
            // }
            write(log_fd, packet_buf, packet_buf_size);
            free(packet_buf);
            close(log_fd);

            // Write everything from the file to the client
            int read_fd = open(filename, O_RDONLY);
            if (read_fd == -1)
            {
                perror("open read file");
            }
            else
            {
                char send_buf[1024];
                ssize_t bytes_to_send;
                while ((bytes_to_send = read(read_fd, send_buf, sizeof(send_buf))) > 0)
                {
                    ssize_t sent = 0;
                    while (sent < bytes_to_send)
                    {
                        ssize_t n = send(client_fd, send_buf + sent, bytes_to_send - sent, 0);
                        if (n == -1)
                        {
                            perror("send");
                            break;
                        }
                        sent += n;
                    }
                }
                if (bytes_to_send == -1)
                {
                    perror("reading from file");
                }
                close(read_fd);
            }
        }

        printf("Closed connection from %s\n", ip_str);
        syslog(LOG_INFO, "Closed connection from %s", ip_str);
        close(client_fd);
        
        // Received newlines indicate the end of a receive stream, and should result
        // in an append to /var/tmp/aesdsocketdata file
        // can use string handling functions

        // returns full content of /var/tmp/aesdsocketdata to the client as soon as received
        // packet is complete.
        // Open file for read
    }

    close(server_socket_fd);
    remove(filename);
    return 0;

    // length of packet will be shorter than available heap size, handle malloc() failures
    // with errors and discard over length packets
}
