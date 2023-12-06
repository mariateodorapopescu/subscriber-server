#include <iostream>
#include <string>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <map>
#include <algorithm>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/stat.h>
#include <netinet/tcp.h>
#include <limits.h>
#include <bits/stdc++.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_CLIENTS 10001
#define MAX_BUFFER_SIZE 1501
#define MAX_TOPIC_SIZE 51
#define MAX_MESSAGES 10001
#define MAX_TOPICS 10001
#define TOTAL_SIZE 1601

using namespace std;

typedef struct Message {
    char topic[MAX_TOPIC_SIZE]; // 51
    uint8_t data_type; // 11
    char value[MAX_BUFFER_SIZE]; // 1501
} Message;

// comanda pe care clientul TCP o primeste de la stdin  
typedef struct cmd {
    char command[12];
    char topic[MAX_TOPIC_SIZE];
    int sf;
} cmd;

// comanda pe care clientul TCP o primeste de la stdin  
typedef struct new_data {
    char command[12];
    char topic[MAX_TOPIC_SIZE];
    int sf;
    string id;
    char ip_server[16]; // assuming IPv4 address
    int port_server;
} new_data;

// comanda pe care clientul TCP o primeste de la stdin  
typedef struct new_data_wsf {
    char command[12];
    char topic[MAX_TOPIC_SIZE];
    int sf;
    string id;
    char ip_server[16]; // assuming IPv4 address
    int port_server;
} new_data_wsf;

// Struct to represent each topic
typedef struct Topic {
    char name[MAX_TOPIC_SIZE];
    vector<string> subscriber_ids;
} Topic;

int main(int argc, char *argv[]) {
    int rc;
    // Disable buffering of stdout
    setvbuf(stdout, NULL, _IONBF, MAX_BUFFER_SIZE);

    // Check input parameters
    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " <id_client> <server_address> <port>" << endl;
        exit(1);
    }

    // Parse port number
    int port = atoi(argv[3]);
    if (port == 0) {
        cerr << "Invalid port" << endl;
        exit(1);
    }

    // Create TCP socket
    int tcp_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_socket_fd < 0) {
        cerr << "Error creating TCP socket." << endl;
        exit(1);
    }

	// Disable Nagle's Algorithm
    int enable = 1;
    if (setsockopt(tcp_socket_fd, IPPROTO_TCP, TCP_NODELAY, (char *)&enable, sizeof(int)) < 0) {
        cerr << "Error disabling Nagle's Algorithm" << endl;
        exit(1);
    }

    // Initialize TCP address
    struct sockaddr_in tcp_address;
    tcp_address.sin_family = AF_INET;
    tcp_address.sin_port = htons(port);
	rc = inet_aton(argv[2], &tcp_address.sin_addr);
    if (rc <= 0) {
        cerr << "Invalid address" << endl;
        exit(1);
    }


    // Connect to server
	rc = connect(tcp_socket_fd, (struct sockaddr*)&tcp_address, sizeof(tcp_address));
    if (rc < 0) {
        cerr << "Error connecting" << endl;
        exit(1);
    }

    // Create select set for polling
    fd_set read_fds;
    FD_ZERO(&read_fds);
	FD_SET(STDIN_FILENO, &read_fds);
    FD_SET(tcp_socket_fd, &read_fds);
    
    int fd_max = max(tcp_socket_fd, STDIN_FILENO);

    // Initialize message buffer
    char buffer[TOTAL_SIZE];

    while (true) {
        // Reset select set for polling
        fd_set tmp_fds = read_fds;
	rc = select(fd_max + 1, &tmp_fds, NULL, NULL, NULL);
        if (rc < 0) {
            cerr << "Error in select" << endl;
            exit(1);
        }

        // Check for stdin input
        if (FD_ISSET(STDIN_FILENO, &tmp_fds)) {
            memset(buffer, 0, TOTAL_SIZE);
            if (fgets(buffer, TOTAL_SIZE - 1, stdin) == NULL) {
                cerr << "Error reading from stdin" << endl;
                exit(1);
            }

                buffer[strlen(buffer) - 1] = '\0';
            
            // and now we parse the data received
            cmd myCmd;
            rc = sscanf(buffer, "%s %s %d", myCmd.command, myCmd.topic, &myCmd.sf);
            if (strcmp(buffer, "exit") == 0) {
                break;
            } else if (strcmp(myCmd.command, "subscribe") == 0) {
                new_data newdata;
                strcpy(newdata.command, myCmd.command);
                strcpy(newdata.topic, myCmd.topic);
                newdata.sf = myCmd.sf;
		char test[32];
		strcpy(test, argv[1]);
		string myStr = test;
                newdata.id = myStr;
                strcpy(newdata.ip_server, argv[2]);
                newdata.port_server = port;
                rc = send(tcp_socket_fd, &newdata, sizeof(new_data), 0);
                if (rc < 0) {
                    cerr << "Send error" << endl;
                    exit(1);
                }
		if (rc == 0) {
			break;
		}
                cout << "Subscribed to topic." << endl;
            } else if (strcmp(myCmd.command, "unsubscribe") == 0) {
                new_data_wsf newdata;
                // ca sa ma asigur ca parsarea merge bine, am mai facut un tip de date
                // special folosit la unsubscribe und enu avem sf =))
                strcpy(newdata.command, myCmd.command);
                strcpy(newdata.topic, myCmd.topic);
               	char test[32];
		strcpy(test, argv[1]);
		string myStr = test;
                newdata.id = myStr;
                strcpy(newdata.ip_server, argv[2]);
                newdata.port_server = port;
                rc = send(tcp_socket_fd, &newdata, sizeof(new_data_wsf), 0);
                if (rc < 0) {
                    cerr << "Send error" << endl;
                    exit(1);
                }
                cout << "Unubscribed from topic." << endl;
            } 
        } else if (FD_ISSET(tcp_socket_fd, &tmp_fds)) {
           // Handle new TCP connection
                memset(buffer, 0, TOTAL_SIZE);
                rc = recv(tcp_socket_fd, buffer, TOTAL_SIZE, 0);
                if (rc < 0) {
                    cerr << "Recv error" << endl;
                    exit(1);
                }
		if (rc == 0) {
			break;
		}
                if (rc > 0) {
                    // a primit mesaj de la server
                    // eu in server l-am trimis ca char pe cel de exit deci da
                    // hai sa zicem ca e ok
                    if (strncmp(buffer, "exit", 4)) {
                        break;
                    } else {
                        Message info;
                        rc = sscanf(buffer, "%s %u %s", info.topic,
                        &info.data_type, info.value);
                        if (info.data_type == 0) {
                            // Variables needed to process the data
                            uint8_t sign;
                            uint32_t value;
                            // Copying the bytes in the variables
                            memcpy(&sign, info.value, 1);
                            memcpy(&value, info.value + 1, 4);
                            // Depending on the sign variable, set the final value recieved
                            // considering it's endianess
                            if (sign == 1) {
                                value = - ntohl(value);
                            }
                            else {
                                value = ntohl(value);
                            } 
                            // Printing the recieved data in an appropiate format
                            cout << inet_ntoa(tcp_address.sin_addr) << ":" << tcp_address.sin_port << " - " << info.topic <<
                            "INT" << " - " << value << endl;
                        }
                        else if (info.data_type == 1) {
                            // Variables needed to process the data
                            uint16_t value;
                            float actual_value;

                            // Copying the bytes in the variables and adjusing the floating
                            // point by dividing to 100
                            memcpy(&value, info.value, sizeof(uint16_t));
                            actual_value = (float)ntohs(value) / 100;

                            // Printing the recieved data in an appropiate format
                            cout << inet_ntoa(tcp_address.sin_addr) << ":" << tcp_address.sin_port << " - " << info.topic <<
                            "SHORT_REAL" << " - " << value << endl;
                        }
                        else if (info.data_type == 2) {
                            uint8_t sign;
                            uint32_t value;
                            uint8_t power_negative_ten;
                            float actual_value;

                            memcpy(&sign, info.value, 1);
                            memcpy(&value, info.value + 1, 4);
                            memcpy(&power_negative_ten, info.value + 5, 1);

                            actual_value = ntohl(value);

                            if (sign == 1) {
                                actual_value = - actual_value;
                            }

                            for (int j = 0; j < power_negative_ten; j++) {
                                actual_value /= 10.00;
                            }

                            // Printing the recieved data in an appropiate format
                            cout << inet_ntoa(tcp_address.sin_addr) << ":" << tcp_address.sin_port << " - " << info.topic <<
                            "FLOAT" << " - " << value << endl;
                        }
                        else if (info.data_type == 3) {
                            // Printing the recieved data in an appropiate format
                        cout << inet_ntoa(tcp_address.sin_addr) << ":" << tcp_address.sin_port << " - " << info.topic <<
                            "STRING" << " - " << info.value << endl;
                        }
                        else {
                            // In case the server transmitted some other kind of message, print
                            // it and the metadata of the message
                            cout << inet_ntoa(tcp_address.sin_addr) << ":" << tcp_address.sin_port << " - " << info.topic
                            << info.value << endl;

                        }
                    }
                }
            }
        }
    // Closing the socket and exiting the program
    close(tcp_socket_fd);
    return 0;
}
