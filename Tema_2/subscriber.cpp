#include <bits/stdc++.h>
#include <iostream>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include "helpers.h"
#include "utils.h"
using namespace std;

/* function to decode the UDP message received from server */
void decode(struct udp_msg* msg_to_print, char* str) {

	/* INT message*/
    if (msg_to_print->type == 0) {
        long long n = ntohl(*((uint32_t*)(msg_to_print->data + 1)));

		/* signed/unsigned int */
    	if (msg_to_print->data[0] == 1) {
    		char sign = '-';
            sprintf(str, "%c%lld", sign, n);
        } else{
            sprintf(str, "%lld", n);
        }
    } else {
		/* SHORT_REAL message */
        if (msg_to_print->type == 1) {
            float nr_sh = ntohs(*((uint16_t*)msg_to_print->data));
            nr_sh /= 100;
            sprintf(str, "%.2f", nr_sh);
        }
		/* FLOAT message */
		else if (msg_to_print->type == 2) {
            double nr_fl = ntohl(*((uint32_t*)(msg_to_print->data + 1)));
            uint8_t p = *(uint8_t*)(msg_to_print->data + 5);
            nr_fl /= pow(10, p);

			/* signed/unsigned */
            if (msg_to_print->data[0] == 1) {
                char sign = '-';
                sprintf(str, "%c%.8g", sign, nr_fl);
            } else{
                sprintf(str, "%.8g", nr_fl);
            }
        }
		/* STRING message */
		else{
            sprintf(str, "%s", msg_to_print->data);
        }
	}
}

int main(int argc, char* argv[]) {
	/* different variables to use in implementation */
    int sockfd_tcp, n, ret, flag = 1;
	struct sockaddr_in server_addr;
	char buffer[BUFLEN];
	/* message received from server */
	struct tcp_msg_to_send msg_rcvd;

	DIE(argc < 4, "Usage: ./subscriber <ID_CLIENT> <IP_SERVER> <PORT_SERVER> \n");

	fd_set read_fds;	/* read set for select */
	fd_set tmp_fds;		/* temporary */

	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

    /* open socket for TCP */
	sockfd_tcp = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd_tcp < 0, "socket");

    /* setting address and port for the server we connect to */
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(atoi(argv[3]));
	ret = inet_aton(argv[2], &server_addr.sin_addr);
	DIE(ret == 0, "inet_aton");

    /* connecting to server */
	ret = connect(sockfd_tcp, (struct sockaddr*) &server_addr, sizeof(server_addr));
	DIE(ret < 0, "connect");
	memset(buffer, 0, BUFLEN);
    strcpy(buffer, argv[1]);

	/* send id_client to server */
    n = send(sockfd_tcp, buffer, sizeof(buffer), 0);
	DIE(n < 0, "send");

	/* deactiv Nagle alg */
	setsockopt(sockfd_tcp, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int));

	/* Adding descriptors to the read set */
	FD_SET(sockfd_tcp, &read_fds);
	FD_SET(STDIN_FILENO, &read_fds);

	/* Get max value of a descriptor */
	int fdmax = sockfd_tcp;

	/* deactivating buffering */
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	while (1) {
		tmp_fds = read_fds; 
		
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");
		
		if (FD_ISSET(STDIN_FILENO, &tmp_fds)) {

  		/* reading from stdin */
			memset(buffer, 0, BUFLEN);
			fgets(buffer, BUFLEN - 1, stdin);

			/* sending message to server */
			n = send(sockfd_tcp, buffer, sizeof(buffer), 0);
			DIE(n < 0, "send");

			/* disconnect */
			if (strcmp(buffer, "exit\n") == 0) {
				break;
			}

			/* subscribe/unsubscribe */
			char* token;
			token = strtok(buffer, " ");

			if (!strncmp(token, "subscribe", 10)) {
				printf("Subscribed to topic.\n");
			} else{
				printf("Unsubscribed from topic.\n");
			}
		} 
        else if (FD_ISSET(sockfd_tcp, &tmp_fds)) {
			/* receive message from server */
			memset(buffer, 0, BUFLEN);
			memset(&msg_rcvd, 0, sizeof(msg_rcvd));
			int n = recv(sockfd_tcp, &msg_rcvd, sizeof(msg_rcvd), 0);
			DIE(n < 0, "recv");

			/* the connection was closed */
			if (n == 0) {
				break;
			}

			/* decode message and print it */
			char str[1600];
			decode(&msg_rcvd.msg_udp, str);

			/* the type as recognized by client */
			char tp[11];
			if (msg_rcvd.msg_udp.type == 0) {
				strcpy(tp, "INT");
			} else if (msg_rcvd.msg_udp.type == 1) {
				strcpy(tp, "SHORT_REAL");
			} else if (msg_rcvd.msg_udp.type == 2) {
				strcpy(tp, "FLOAT");
			} else {
				strcpy(tp, "STRING");
			}

			printf("%s:%d - %s - %s - %s\n", msg_rcvd.udp_cl_ip, msg_rcvd.udp_cl_port, 
							msg_rcvd.msg_udp.topic, tp, str);
		}
	}

	/* close sockets */
	close(sockfd_tcp);

return 0;    
}
