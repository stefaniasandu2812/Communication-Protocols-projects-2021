#include <bits/stdc++.h>
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include "utils.h"
#include "helpers.h"
using namespace std;

int main(int argc, char* argv[]) {
    /* different variables to use in implementation */
    int sockfd_udp, sockfd_tcp, newsockfd, portno;
    int maxfd, ret, ok = 0, flag = 1;
    struct sockaddr_in server_addr, client_addr, udp_cl_addr;
    struct client cl;
    socklen_t clilen;
    char buff[BUFLEN];
    
    fd_set read_fds;	/* read set for select */
	fd_set tmp_fds;		/* temporary */

    /* message to be send to TCP client */
    struct tcp_msg_to_send msg_to_tcp;

    /* message received from UDP client */
    struct udp_msg* msg;

    /* map to store <topics, subscribers> */
    map<string, map<string, struct client>> topics_subs;

    /* map to store <id_client, client> */
    map<string, struct client> clients;

    /* map to store <id_client, messages_to_send> */
    map<string, queue<struct tcp_msg_to_send>> msg_holder;

    /* iterators for looping through maps */
    map<string, struct client>::iterator it;

    map<string, struct client>::iterator it2;

    map<string, map<string, struct client>>::iterator it3;

    DIE(argc < 1, "usage: ./server <PORT_NO> \n");

    portno = atoi(argv[1]);
    DIE(portno == 0, "atoi");

    /* clear the descriptor set */
    FD_ZERO(&read_fds);
    FD_ZERO(&tmp_fds);

    /* Open sockets for UDP and TCP */
    sockfd_udp = socket(AF_INET, SOCK_DGRAM, 0);
    DIE(sockfd_udp == -1, "Open UDP socket failed!\n");

    sockfd_tcp = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sockfd_tcp == -1, "Open TCP socket failed!\n");

    /* Setting the address for server to listen on the given port number */
    memset((char *) &server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(portno);
	server_addr.sin_addr.s_addr = INADDR_ANY;

    /* Binding sockets to server address */
    int b1 = bind(sockfd_udp, (struct sockaddr*) &server_addr, sizeof(server_addr));
    DIE(b1 < 0, "Error binding socket UDP!\n");

    int b2 = bind(sockfd_tcp, (struct sockaddr*) &server_addr, sizeof(server_addr));
    DIE(b2 < 0, "Error binding socket TCP!\n");

    ret = listen(sockfd_tcp, 5);
    if (ret < 0) {
        perror("Error listening!\n");
        exit(1);
    }
    DIE(ret < 0, "Error listening!\n");

    /* deactivating Nagle alg */
	setsockopt(sockfd_tcp, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));

    /* Adding descriptors to the read set */
    FD_SET(sockfd_tcp, &read_fds);
    FD_SET(sockfd_udp, &read_fds);
    FD_SET(STDIN_FILENO, &read_fds);

    /* Get max value of a descriptor */
    maxfd = sockfd_tcp;

    /* deactivating buffering */
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    while(ok != 1) {
        tmp_fds = read_fds;

        ret = select(maxfd + 1, &tmp_fds, NULL, NULL, NULL);
        DIE(ret < 0, "error select!\n");

        for (int i = 0; i <= maxfd; ++i) {
            if (FD_ISSET(i, &tmp_fds)) {
                if (i == STDIN_FILENO) {
                    /* reading from stdin */
                    memset(buff, 0, BUFLEN);
                    fgets(buff, BUFLEN - 1, stdin);

                    /* stopping the server */
                    if (strcmp(buff, "exit\n") == 0) {
                        ok = 1;
                        break;
                    }
                }

                /* receiving messages from UDP client */
                if (i == sockfd_udp) {
                    socklen_t socklen = sizeof(udp_cl_addr);
                    memset(buff, 0, BUFLEN);

                    int r = recvfrom(sockfd_udp, buff, BUFLEN, 0, 
                                (struct sockaddr*) &udp_cl_addr, &socklen);
		            DIE(r == -1, "Error receiving data");

                    msg = (struct udp_msg*)buff;
                    
                    /* the topic is already created */
                    if (topics_subs.count(msg->topic) > 0) {
                        /* there are no clients subscribed to this topic */
                        if ((topics_subs[msg->topic]).empty() == 1) {
                            continue;
                        } else{
                            /* there are clients subscribed and the server
                                has to send the message */
                            memset(&msg_to_tcp, 0, sizeof(msg_to_tcp));

                            /* copy of the UDP client's address and port */
                            strcpy(msg_to_tcp.udp_cl_ip, inet_ntoa(udp_cl_addr.sin_addr));
                            msg_to_tcp.udp_cl_port = ntohs(udp_cl_addr.sin_port);

                            /* copy the UDP message, as send, to the TCP 
                                message to be sent */
                            memcpy(&msg_to_tcp.msg_udp, msg, sizeof(struct udp_msg));

                            /* iterate through topics map */
                            for (it2 = topics_subs[msg->topic].begin(); 
                                    it2 != topics_subs[msg->topic].end(); it2++) {
                                
                                /* send message if client connected */
                                if (it2->second.connected == 1) {
                                    send(it2->second.fd, &msg_to_tcp, sizeof(msg_to_tcp), 0);
                                } else{
                                    /* add message to the queue to be
                                        sent when client reconnects */
                                    if (strcmp(&it2->second.sf, "1") == 0) {
                                        msg_holder[it2->second.id].push(msg_to_tcp);
                                    }
                                }
                            }
                        }
                    } else{
                        /* create topic key */
                        topics_subs[msg->topic];
                    }
                /* receive request to connect from TCP client */
                } else if (i == sockfd_tcp) {
                    clilen = sizeof(client_addr);
                    /* accept the req */
                    newsockfd = accept(sockfd_tcp, (struct sockaddr *) &client_addr, &clilen);
                    /* deactivating Nagle alg */
	                setsockopt(newsockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));

                    /* receive message */
                    memset(buff, 0, BUFLEN);
                    ret = recv(newsockfd, buff, BUFLEN, 0);
                    DIE(ret < 0, "recv");

                    char id_cl[10]; /* the id of the client connected */
                    memset(id_cl, 0, sizeof(id_cl));
                    memset(&cl, 0, sizeof(cl));
                    strcpy(id_cl, buff);

                    /* the client was connected once */
                    if (clients.count(id_cl) > 0) {
                        /* if client tries to reconnect */
                        if (clients[id_cl].connected == 0) {
                            /* update client and new socket of connection */
                            FD_SET(newsockfd, &read_fds);
                            if (newsockfd > maxfd) {
                                maxfd = newsockfd;
                            }
                            clients[id_cl].fd = newsockfd;
                            clients[id_cl].connected = 1;
                            
                            /* iterate through topics map to update client */
                            for (it3 = topics_subs.begin(); it3 != topics_subs.end(); it3++) {
                                for (it2 = topics_subs[it3->first].begin(); 
                                        it2 != topics_subs[it3->first].end(); ++it2) {
                                    if (strcmp(it2->second.id, id_cl) == 0) {
                                        it2->second.connected = 1;
                                        it2->second.fd = newsockfd;
                                    }
                                }
                            }

                            /* send messages to the clients with sf = 1
                                that were disconnected */
                            while (!msg_holder[id_cl].empty()) {
                                struct tcp_msg_to_send aux_msg;
                                aux_msg = msg_holder[id_cl].front();

                                send(newsockfd, &aux_msg, sizeof(struct tcp_msg_to_send), 0);
                                msg_holder[id_cl].pop();
                            }
                        
                        printf("New client %s connected from %s:%d.\n", id_cl, 
                                inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

                        /* another client tries to connect with an existing id */
                        } else{
                            printf("Client %s already connected.\n", id_cl);
                            close(newsockfd);
                        }
                    /* the client connects the first time */
                    } else{
                        FD_SET(newsockfd, &read_fds);
                        if (newsockfd > maxfd) {
                            maxfd = newsockfd;
                        }

                        /* create queue for client */
                        msg_holder[id_cl];

                        /* complete client members */
                        strcpy(cl.id, id_cl);
                        cl.connected = 1;
                        cl.fd = newsockfd;
                        clients.insert({id_cl, cl});

                        printf("New client %s connected from %s:%d.\n", id_cl, 
                                inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                    }
                } else {
                    /* the server receives data from one of the clients' sockets */
					memset(buff, 0, BUFLEN);
					ret = recv(i, buff, sizeof(buff), 0);
					DIE(ret < 0, "recv");

                    /* client disconnects */
                    if (!strncmp(buff, "exit", 4)) {
                        /* close the socket */
                        close(i);
                        FD_CLR(i, &read_fds);

                        /* iterate through clients map to find the id */
                        for (it = clients.begin(); it != clients.end(); it++) {
                            if (it->second.fd == i) {
                                it->second.connected = 0;
                                cout << "Client " << it->first << " disconnected." << endl;
                                break;
                            }
                        }
                        /* iterate through topics to update client */
                        for (it3 = topics_subs.begin(); it3 != topics_subs.end(); it3++) {
                            for (it2 = topics_subs[it3->first].begin(); it2 != topics_subs[it3->first].end(); ++it2) {
                                if (it2->second.fd == i) {
                                    it2->second.connected = 0;
                                }
                            }
                        }
                    /* client subscribes/unsubscribes */
                    } else{
                        char* token;
                        token = strtok(buff, " ");
                        
                        if (strncmp(token, "unsubscribe", 11) == 0) {
                            token = strtok(NULL, " ");
                            /* iterate through clients subscribed 
                                to topic and "unsubscribe" it */
                            for (it2 = topics_subs[token].begin(); it2 != topics_subs[token].end(); it2++) {
                                if (it2->second.fd == i) {
                                    topics_subs[token].erase(it2);
                                    break;
                                }
                            }
                        } else {
                            token = strtok(NULL, " ");  /* get topic */
                            char topic[50];
                            memset(topic, 0, sizeof(topic));
                            strcpy(topic, token);
                            token = strtok(NULL, " ");  /* get SF */

                            /* if topic is already created */
                            if (topics_subs.count(topic) > 0) {
                                /* iterate through clients map and
                                    add pair to the subscribed topic */
                                for (it = clients.begin(); it != clients.end(); it++) {
                                    if (it->second.fd == i) {
                                        strncpy(&it->second.sf, token, 1);
                                        topics_subs[topic].insert({it->first, it->second});
                                        break;
                                    }
                                } 
                            } else{
                                /* create topic key */
                                topics_subs[topic];
                                /* iterate through clients map and
                                    add pair to the subscribed topic */
                                for (it = clients.begin(); it != clients.end(); it++) {
                                    if (it->second.fd == i) {
                                        strncpy(&it->second.sf, token, 1);
                                        topics_subs[topic].insert({it->first, it->second});
                                        break;
                                    }
                                } 
                            }
                        }
                    }
                }   
            }
        }
    }

    /* close sockets */
    close(sockfd_tcp);
    close(sockfd_udp);

    return 0;
}
