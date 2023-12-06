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
    char topic[MAX_TOPIC_SIZE]; 
    uint8_t data_type;
    char value[MAX_BUFFER_SIZE]; 
} Message;

// comanda pe care clientul TCP o primeste de la stdin  
typedef struct cmd {
    char command[12];
    char topic[MAX_TOPIC_SIZE];
    int sf;
} cmd;

// topic
typedef struct Topic {
    char name[MAX_TOPIC_SIZE];
    vector<string> subscriber_ids;
} Topic;

// Structura pentru un client TCP
typedef struct Client {
    string id;
    int socket_fd;
    vector<Message> messages;
    int active;
    vector<Topic> topics;
} Client;

int main(int argc, char *argv[]) {
    char buffer[TOTAL_SIZE];
    int rc;

    // trebuie sa dezactivez afisarea
    setvbuf(stdout, NULL, _IONBF, MAX_BUFFER_SIZE);

    // Verificare parametrii de intrare
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <port>" << endl;
        exit(1);
    }

    // Parsare numar port
    int port = atoi(argv[1]);
    if (port == 0) {
        cerr << "Port error" << endl;
        exit(1);
    }

    // Creare socket UDP
    int udp_socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket_fd < 0) {
        cerr << "Error creating UDP socket." << endl;
        exit(1);
    }

    // Setare optiuni socket UDP
    int broadcast = 1;
    if (setsockopt(udp_socket_fd, SOL_SOCKET, SO_BROADCAST, &broadcast,
    sizeof(broadcast)) < 0) {
        cerr << "Error setting UDP socket options." << endl;
        exit(1);
    }

    int reuseudp = 1;
    if (setsockopt(udp_socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuseudp,
    sizeof(reuseudp)) < 0) {
        cerr << "setsocketopt error" << endl;
        exit(1);
    }

    // Initializare adresa UDP
    struct sockaddr_in udp_address;
    udp_address.sin_family = AF_INET;
    udp_address.sin_port = htons(port);
    udp_address.sin_addr.s_addr = INADDR_ANY;

    // Asociere adresa UDP cu socketul
    if (bind(udp_socket_fd, (struct sockaddr *)&udp_address,
    sizeof(udp_address)) < 0) {
        cerr << "Error binding UDP socket." << endl;
        exit(1);
    }

    // Creare socket TCP
    int tcp_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_socket_fd < 0) {
        cerr << "Error creating TCP socket." << endl;
        exit(1);
    }

    int reusetcp = 1;
    if (setsockopt(tcp_socket_fd, SOL_SOCKET, SO_REUSEADDR, &reusetcp,
    sizeof(reusetcp)) < 0) {
        cerr << "Error setting socket." << endl;
        exit(1);
    }

    // Initializare adresa TCP
    struct sockaddr_in tcp_address;
    tcp_address.sin_family = AF_INET;
    tcp_address.sin_port = htons(port);
    tcp_address.sin_addr.s_addr = INADDR_ANY;

    // Asociere adresa TCP cu socketul
    if (bind(tcp_socket_fd, (struct sockaddr *)&tcp_address,
    sizeof(tcp_address)) < 0) {
        cerr << "Error binding TCP socket." << endl;
        exit(1);
    }

    // Ascultare conexiuni TCP
    if (listen(tcp_socket_fd, MAX_CLIENTS) < 0) {
        cerr << "Error listening for TCP connections." << endl;
        exit(1);
    }

    // Initializare vector de clienti TCP si vector de topicuri
    // ca sa pot tine evidenta clientilor inscrisi la topicul respectiv
    vector<Client> tcp_clients;
    vector<Topic> topics;

    // Initializare set de socketi pentru functia select
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(udp_socket_fd, &read_fds);
    FD_SET(tcp_socket_fd, &read_fds);
    FD_SET(STDIN_FILENO, &read_fds);
    int max_fd = max(udp_socket_fd, tcp_socket_fd);

    while (true) {
    // ajuta la select
    fd_set tmp_fds = read_fds;

    // se seteaza toti clientii tcp
    for (auto& cli : tcp_clients) {
        FD_SET(cli.socket_fd, &tmp_fds);
    }

    // select
    if (select(max_fd + 1, &tmp_fds, NULL, NULL, NULL) == -1) {
        cerr << "Error in select" << endl;
        exit(1);
    }

    fflush(stdout); // prioritate server -> cat mai liber, cat mai putine date stocate =)

    // verific toate fd-urile daca au date in ele
        for (int i = 0; i <= max_fd; i++) {
            if (!FD_ISSET(i, &tmp_fds)) {
                continue;
            }

            // verific daca e ceva de la stdin
            if (i == 0) {
                
                memset(buffer, 0, TOTAL_SIZE);
                if (fgets(buffer, TOTAL_SIZE - 1, stdin) == NULL) {
                    cerr << "Error reading from stdin" << endl;
                    exit(1);
                }
                buffer[strlen(buffer) - 1] = '\0';

                // execut comanda de iesire
                if (strncmp(buffer, "exit", 4) == 0) {
                    // trimit "exit" la clienti
                    for (auto& cli : tcp_clients) {
                        if (send(cli.socket_fd, "exit", 4, 0) < 0) {
                            cerr << "Error sending exit command to client" << endl;
                            exit(1);
                        }
			// inchid sock client
                        close(cli.socket_fd); 
                    }

                    // sterg toate topicurile
                    topics.clear();

                    // inchid sock-uri server
                    close(tcp_socket_fd);
                    close(udp_socket_fd);
                    return 0;
                }
            }

            // verific ce a primit de la udp
            else if (i == udp_socket_fd) {
                memset(buffer, 0, TOTAL_SIZE);

                struct sockaddr_in udp_client_address;
                socklen_t udp_client_address_len = sizeof(udp_client_address);
                memset((char*)&udp_client_address, 0, sizeof(udp_client_address));

                // primire mesaj udp
                int n = recvfrom(udp_socket_fd, buffer, sizeof(buffer), 0,
                        (struct sockaddr *)&udp_client_address, &udp_client_address_len);

                if (n < 0) {
                    cerr << "Error receiving UDP message" << endl;
                    continue;
                }
                buffer[n] = '\0';

                // trebuie sa-l parsez
               Message info;
                        rc = sscanf(buffer, "%s %u %s", info.topic,
                        &info.data_type, info.value);
			char tosend[TOTAL_SIZE];
                        if (info.data_type == 0) {
                      		// vad daca are semn + valoarea finala de calculat
                            uint8_t sign;
                            uint32_t value;
                            // parsez value-ul in functie de ce date trebuie sa am
                            memcpy(&sign, info.value, 1);
                            memcpy(&value, info.value + 1, 4);
                            // vad daca e poz sau neg
                            if (sign == 1) {
                                value = - ntohl(value);
                            }
                            else {
                                value = ntohl(value);
                            } 
                            sprintf(tosend, "%s:%d - %s - INT - %s", inet_ntoa(tcp_address.sin_addr), tcp_address.sin_port, info.topic, info.value);
                        }
                        else if (info.data_type == 1) {
                            // ibidem ca mai sus
                            uint16_t value;
                            float actual_value;

                            // doar ca aici vrea /100
                            memcpy(&value, info.value, sizeof(uint16_t));
                            actual_value = (float)ntohs(value) / 100;
				sprintf(tosend, "%s:%d - %s - SHORT_REAL - %s", inet_ntoa(tcp_address.sin_addr), tcp_address.sin_port, info.topic, info.value);
                        }
                        else if (info.data_type == 2) {
			// pe aceeasi idee
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
			sprintf(tosend, "%s:%d - %s - FLOAT - %s", inet_ntoa(tcp_address.sin_addr), tcp_address.sin_port, info.topic, info.value);
                        }
                        else if (info.data_type == 3) {
                            sprintf(tosend, "%s:%d - %s - STRING - %s", inet_ntoa(tcp_address.sin_addr), tcp_address.sin_port, info.topic, info.value);
                        }
                        else {
                            sprintf(tosend, "%s:%d - %s - %s", inet_ntoa(tcp_address.sin_addr), tcp_address.sin_port, info.topic, info.value);
                        }

                // trimit ce am pasrat si am formatat frumos la client ca sa afiseze, daca e activ
                for (auto& cli : tcp_clients) {
                    // caut daca e acel topic la fiecare client
                    for (auto& topic : cli.topics) {
                        if (strcmp(topic.name, info.topic) == 0) {
                            // vad daca e activ ca sa daus send, altfel 
                            if (cli.active == 1) {
				// totusi, inca nu-mi dau seama cum e cu active sau nu, desiii...
                                if (send(cli.socket_fd, &tosend, sizeof(tosend), 0) < 0) {
                                    perror("Error sending message");
                                    continue;
                                }
                            } else {
                                // altfel, pun mesajul brut in lista de mesaje
				// -> bine, aici as fi putut sa pun mesajul deja formatat
                                cli.messages.push_back(info);
                            }
                        }
                    }
                }

            } else if (i == tcp_socket_fd) {
		        memset(buffer, 0, TOTAL_SIZE);
                int store_fwd = -1; 
                // aici verific daca are sf activ sau nu, initial nu-l pun nici asa, nici asa
                // i just tried something, chit ca nu am dus pana la capat
		// verific conexiune tcp
                struct sockaddr_in client_addr;
                socklen_t client_addr_len = sizeof(client_addr);
                int new_fd = accept(tcp_socket_fd, (struct sockaddr*)&client_addr, &client_addr_len);
                if (new_fd == -1) {
                    cerr << "Error accepting new TCP connection" << endl;
                    continue;
                }
                FD_SET(new_fd, &read_fds);
                if (new_fd > max_fd) { 
                     max_fd = new_fd;
                }
                // dezactivez algo Neagle
                int enable = 1;
                setsockopt(tcp_socket_fd, IPPROTO_TCP, TCP_NODELAY, (char *)&enable, sizeof(int));
		
                // i-am dat clientului ca mai intai de tot sa trimita
                // argumentele date in executabil la pornire, gen id, ip, port
                // dar am nevoie numai de id =))
                        int n = recv(new_fd, buffer, sizeof(buffer), 0);
                string cliid = buffer;
                        if (n <= 0) {
                            cerr << "Recv error" << endl;
                            close(new_fd);
                            FD_CLR(new_fd, &read_fds);
                            for (auto it = tcp_clients.begin(); it != tcp_clients.end(); ++it) {
                                if (it->socket_fd == new_fd) {
                                    tcp_clients.erase(it);
                                    break;
                                }
                            }
                            exit(1);
                        }

                // da se poate sti ca acest client nu e unul nou?
                        int brk = 0; // pe post de break
                        string the_id; // id-ul clientului
                        int if_active = 0; // daca e activ, plecam de la presupunerea ca nu este
                        for (auto& cli : tcp_clients) {
                            if (cliid == cli.id) {
                                brk = 1;
                                if_active = cli.active;
                            }
                        }
                        if (brk == 0) {
                            // daca nu l-a gasit
                            // face unul nou
                            Client new_client;
                                new_client.id = cliid; 
                                new_client.socket_fd = new_fd;
                                new_client.active = 1;
                            tcp_clients.push_back(new_client);
                            cout << "New client " << new_client.id.c_str() << " connected from " <<
                                    inet_ntoa(client_addr.sin_addr) << ":" <<
                                    ntohs(client_addr.sin_port) << "." << endl;
                        } else if (brk == 1 && if_active == 1) {
                            cout<< "Client " << cliid << " already connected." << endl;
                        } else if (brk == 1 && if_active == 0) {
                    cout << "Client " << the_id.c_str() << "disconnected."<<endl;
                            // inseamna ca a fost deconectat??
                            // reconectare
                            // il face activ si trimite datele din el
                            for (auto& cli : tcp_clients) {
                                if (cli.id == cliid) {
                                    cli.active = 1;
                                    for (int j = 0; j < cli.messages.size(); j++) {
                                        Message current_message = cli.messages[j];
                                        rc = send(cli.socket_fd, &current_message, sizeof(Message), 0);
                                        if (rc < 0) {
                                            cerr << "Send error" << endl;
                                            exit(1);
                                        }
                                    }
                                }
                            }
                        }
                
                // abia acum vede care e faza cu subscribe si unsubscribe
                memset(buffer, 0, TOTAL_SIZE);
                n = recv(new_fd, buffer, sizeof(buffer), 0);
                       if (n <= 0) {
                            cerr << "Recv error" << endl;
                            close(new_fd);
                            FD_CLR(new_fd, &read_fds);
                            for (auto it = tcp_clients.begin(); it != tcp_clients.end(); ++it) {
                                if (it->socket_fd == new_fd) {
                                    tcp_clients.erase(it);
                                    break;
                                }
                            }
                            exit(1);
                        }
                // mai simplu
                // doar asta
		if (strncmp(buffer, "exit", 4) == 0) {
			for (auto& cli : tcp_clients) {
                            if (cliid == cli.id) {
                               cli.active = 0; 
				// daca clientul isi da exit inseamna ca devine inactiv
                            }
                        }
			cout<< "Client " << cliid << " disconnected." << endl;
		}
                cmd message;
                rc = sscanf(buffer, "%s %s %d",
                            message.command, message.topic, &message.sf);
                if (strcmp(message.command, "subscribe") == 0) {
                    store_fwd = message.sf;
                    int found = -1;
                    for (int j = 0; j < topics.size(); j++) {
                        if (strcmp(message.topic, topics[j].name) == 0) {
                            found = j;
                            break;
                        }
                    }
                    // caut sa vad daca exista topicul
                    if (found == -1) {
                        // daca nu, creez eu unul si-l pun in vectorul de topic-uri
                        Topic new_topic;
                        strcpy(new_topic.name, message.topic);
                        new_topic.subscriber_ids.push_back(cliid);
                        topics.push_back(new_topic);
                        for (auto& cli : tcp_clients) {
                            if (cli.socket_fd == new_fd) {
                                cli.topics.push_back(new_topic);
                                break;
                            }
                        }
                    } else {
                        // altfel, pun id client la lista de subscriberi la topic
			// si actualizez lista de topicuri ale acelui client
                        topics[found].subscriber_ids.push_back(cliid);
                        for (auto& cli : tcp_clients) {
                            if (cli.socket_fd == new_fd) {
                                cli.topics.push_back(topics[found]);
                                break;
                            }
                        }
                    }
                } else if (strcmp(message.command, "unsubscribe") == 0) {
                    // daca e unsubscribe
                    int found = -1;
                    for (int j = 0; j < topics.size(); j++) {
                        if (strcmp(message.topic, topics[j].name) == 0) {
                            found = j;
                            break;
                        }
                    }
                    // verific sa vad daca exista topicul
                    if (found != -1) {
                        // daca exista, o sterge, atfel nu face nimic =))
                        int oki = -1;
                        for (int j = 0; j < topics[found].subscriber_ids.size(); j++) {
                            if (topics[found].subscriber_ids[j] == cliid) {
                                oki = j;
                                break;
                            }
                        }
                        if (oki != -1) {
                            // sterg clientul din vect de id de clienti ai vect de topicuri =)))
                            topics[found].subscriber_ids.erase(topics[found].subscriber_ids.begin() + oki);
                            // dar si topicul din lista de topicuri ale clientului
                            for (auto& cli : tcp_clients) {
                                int okok = -1;
                                for (int k = 0; k < cli.topics.size(); k++) {
                                    if (cli.topics[k].name == message.topic) {
                                        okok = k;
                                        break;
                                    }
                                }
                                if (okok != -1) {
                                cli.topics.erase(cli.topics.begin() + okok);
                                break;
                                }
                            }
                        }
                    }
                } 
            }
        }
    }
    // Inchidere socket TCP
    close(tcp_socket_fd);
    close(udp_socket_fd);
    return 0;
}
