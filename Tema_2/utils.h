#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <bits/stdc++.h>

struct __attribute__((packed)) udp_msg {
    char topic[50];
    uint8_t type;
    char data[1500];
};

struct __attribute__((packed)) tcp_msg_to_send {
    char udp_cl_ip[16];
    uint16_t udp_cl_port;
    struct udp_msg msg_udp;
};

struct client {
    char id[10];
    int connected;
    char sf;
    int fd;
};

#endif