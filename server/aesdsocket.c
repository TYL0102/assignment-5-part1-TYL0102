#include<stdbool.h>
#include<string.h>
#include<sys/socket.h>
#include<unistd.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<syslog.h>
#include<signal.h>
#include<fcntl.h>

#define PORT 9000
#define DATA_FILE "/var/tmp/aesdsocketdata"
#define BUFFER_SIZE 1024

static volatile bool running = true;

void signal_handler(int signal) {
    running = false;
    syslog(LOG_INFO, "Caught signal, exiting");
}

int main(int argc, char* argv[]) {

    // check daemon mode
    bool daemon_mode = false;
    if (argc > 1 && strcmp(argv[1], "-d") == 0) {
        daemon_mode = true;
    }

    // open system log
    openlog("aesdsocket", LOG_PID | LOG_CONS, LOG_USER);

    // set signal handler
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = signal_handler;
    if (sigaction(SIGINT, &sa, NULL) == -1 || sigaction(SIGTERM, &sa, NULL)) {
        syslog(LOG_ERR, "Failed to set signal handler");
        closelog();
        return -1;
    }

    // open server socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        syslog(LOG_ERR, "Socket creation failed");
        closelog();
        return -1;
    }

    // set socket option
    int option = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) == -1) {
        syslog(LOG_ERR, "Setsockopt failed");
        close(server_fd);
        closelog();
        return -1;
    }

    // bind socket
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        syslog(LOG_ERR, "Bind failed");
        close(server_fd);
        closelog();
        return -1;
    }

    // fork if daemon mode is active
    if (daemon_mode) {
        pid_t pid = fork();
        if (pid == -1) {
            syslog(LOG_ERR, "Fork failed");
            close(server_fd);
            closelog();
            return -1;
        }
        if (pid != 0) {
            close(server_fd);
            closelog();
            return 0;
        }
        if (setsid() == -1) {
            syslog(LOG_ERR, "Setsid failed");
            close(server_fd);
            closelog();
            return -1;
        }
        if (chdir("/") == -1) {
            syslog(LOG_ERR, "Chdir failed");
            close(server_fd);
            closelog();
            return -1;
        }
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
    }

    // set socket listen
    if (listen(server_fd, 16) == -1) {
        syslog(LOG_ERR, "Listen failed");
        close(server_fd);
        closelog();
        return -1;
    }

    while (running) {

        // accept connection from client
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd == -1) {
            if (!running) {
                break;
            }
            syslog(LOG_ERR, "Accept failed");
            continue;
        }

        // log client ip
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        syslog(LOG_INFO, "Accepted connection from %s", client_ip);
        
        // open file
        int file_fd = open(DATA_FILE, O_RDWR | O_CREAT | O_APPEND, 0644);
        if (file_fd == -1) {
            syslog(LOG_ERR, "Failed to open %s", DATA_FILE);
            close(client_fd);
            continue;
        }

        // write recieved data 
        char received_buffer[BUFFER_SIZE], read_buffer[BUFFER_SIZE];
        ssize_t bytes_received, bytes_read;
        while ((bytes_received = recv(client_fd, received_buffer, BUFFER_SIZE - 1, 0)) > 0) {
            received_buffer[bytes_received] = '\0';

            // if no new line char, write directly
            char *newline = strchr(received_buffer, '\n');
            if (!newline) {
                if (write(file_fd, received_buffer, bytes_received) != bytes_received) {
                    syslog(LOG_ERR, "Write to %s failed", DATA_FILE);
                    break;
                }
                continue;
            }

            // write data before new line char
            size_t chunk_size = newline - received_buffer + 1;
            if (write(file_fd, received_buffer, chunk_size) != (ssize_t)chunk_size) {
                syslog(LOG_ERR, "Write to %s failed", DATA_FILE);
                break;
            }

            // seek to begin of the file
            if (lseek(file_fd, 0, SEEK_SET) == -1) {
                syslog(LOG_ERR, "Lseek to start failed");
                break;
            }

            // send file data
            while ((bytes_read = read(file_fd, read_buffer, BUFFER_SIZE)) > 0) {
                if (send(client_fd, read_buffer, bytes_read, 0) != bytes_read) {
                    syslog(LOG_ERR, "Send failed");
                    break;
                }
            }

            // check read result
            if (bytes_read == -1) {
                syslog(LOG_ERR, "Read failed");
                break;
            }

            // seek to end of the file
            if (lseek(file_fd, 0, SEEK_END) == -1) {
                syslog(LOG_ERR, "Lseek to end failed");
                break;
            }

            // write data after new line char
            size_t bytes_remaining = bytes_received - chunk_size;
            if (bytes_remaining > 0) {
                memmove(received_buffer, newline + 1, bytes_remaining);
                received_buffer[bytes_remaining] = '\0';
                if (write(file_fd, received_buffer, bytes_remaining) != bytes_remaining) {
                    syslog(LOG_ERR, "Write to %s failed", DATA_FILE);
                    break;
                }
            }

        }

        if (bytes_received == -1) {
            syslog(LOG_ERR, "Recv failed");
        }

        close(file_fd);
        close(client_fd);
        syslog(LOG_INFO, "Closed connection from %s", client_ip);

    }

    // close socket and exit
    close(server_fd);
    closelog();
    unlink(DATA_FILE);
    return 0;

}