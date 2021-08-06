#include <queue.h>
#include "skel.h"
#include <stdio.h>
#include <string.h>

// entry for route table
struct route_table{
	uint32_t prefix;
	uint32_t next_hop;
	uint32_t mask;
	int interface;
} __attribute__((packed));

// entry for an ARP table
struct arp_entry {
	__u32 ip;
	uint8_t mac[6];
};

// declaration of rtable and ARP table
struct route_table *rtable;
int rtable_size;
struct arp_entry *arp_table;
int arp_table_len;

// function for parsing rtable
int parsing_rtable(char *file_name, struct route_table *rtable) {
	FILE* f = fopen(file_name, "r");
	int nr = 0;
	char line[50], prefix[25], next_hop[25], mask[25];
	int interface;

	while(fgets(line, 50, f) != NULL) {
		sscanf(line, "%s %s %s %d", prefix, next_hop, mask, &interface);
		rtable[nr].prefix = inet_addr(prefix);
		rtable[nr].next_hop = inet_addr(next_hop);
		rtable[nr].mask = inet_addr(mask);
		rtable[nr].interface = interface;
		++nr;
	}

	fclose(f);

	return nr;
}

// function for comparing used for quicksort
int comparator(const void* p1, const void* p2) {
	struct route_table* p = (struct route_table*) p1;
	struct route_table* q = (struct route_table*) p2;

	if (p->prefix == q->prefix) {
		if (p->mask > q->mask) {
			return 1;
		} else if (p->mask < q->mask){
			return -1;
		} else {
			return 0;
		}
	} else if (p->prefix > q->prefix) {
		return 1;
	}

	return -1;
}

// get best route using binary search
struct route_table* get_best_route(__u32 dest_ip,
 				int left, int right) {

	int mid;

	while (left <= right) {
		mid = (left + right) / 2;

		if ((rtable[mid].mask & dest_ip) == rtable[mid].prefix) {
			return &rtable[mid];
		}

		if ((rtable[mid].mask & dest_ip) > rtable[mid].prefix) {
			left = mid + 1;
		} else{
			right = mid - 1;
		}

	}

	return NULL;
}

// function to return the entry of the ARP table
struct arp_entry *get_arp_entry(__u32 ip) {

	for (int i = 0; i < arp_table_len; ++i) {
		if (ip == arp_table[i].ip) {
			return &arp_table[i];
		}
	}
    return NULL;
}

int main(int argc, char *argv[])
{
	packet m;
	int rc;

	init(argc - 2, argv + 2);

	rtable = malloc(sizeof(struct route_table) * 100000);
	DIE(rtable == NULL, "memory");
	rtable_size = parsing_rtable(argv[1], rtable);

	// qsort for sorting rtable using compare_function for binary search
	qsort(rtable, rtable_size, sizeof(struct route_table), comparator);

	arp_table = malloc(sizeof(struct arp_entry) * 10);
	DIE(arp_table == NULL, "memory");
	arp_table_len = 0;

	struct queue* queue = queue_create();

	while (1) {
		rc = get_packet(&m);
		DIE(rc < 0, "get_message");
		
		// headers from the packet
		struct ether_header *eth_hdr = (struct ether_header *)m.payload;
		struct iphdr *ip_hdr = (struct iphdr *)(m.payload + sizeof(struct ether_header));
		struct icmphdr *icmp_hdr = parse_icmp(m.payload);
		struct arp_header *arp_hdr = parse_arp(m.payload);

		// IP and ICMP
		if (eth_hdr->ether_type == htons(ETHERTYPE_IP)) {
			if (icmp_hdr != NULL) {
				// checking if the packet is addressed for the router
				if (ip_hdr->daddr == inet_addr(get_interface_ip(m.interface))) {
					// if the packet is ICMP echo request
					if (icmp_hdr->type == ICMP_ECHO && icmp_hdr->code == 0) {
						// send ICMP echo reply
						send_icmp(ip_hdr->saddr, ip_hdr->daddr, eth_hdr->ether_dhost,
						 eth_hdr->ether_shost, ICMP_ECHOREPLY, 0, m.interface, htons(getpid()), 0);
						continue;
					}
				}
			}
			// check ttl 
			if (ip_hdr->ttl <= 1) {
				// sending ICMP time exceeded
				send_icmp_error(ip_hdr->saddr, ip_hdr->daddr, eth_hdr->ether_dhost,
				 eth_hdr->ether_shost, ICMP_TIME_EXCEEDED, 0, m.interface);
				continue;
			}

			// wrong checksum
			if (ip_checksum(ip_hdr, sizeof(*ip_hdr)) != 0) {
				continue;
			}

			// find best route and send ICMP error message if not found
			struct route_table *best_route = get_best_route(ip_hdr->daddr, 0, rtable_size - 1);
			if (best_route == NULL) {
				send_icmp_error(ip_hdr->saddr, ip_hdr->daddr, eth_hdr->ether_dhost,
				 eth_hdr->ether_shost, ICMP_DEST_UNREACH, 0, m.interface);
				continue;
			}

			// computing new checksum and updating the packet
			ip_hdr->ttl--;
			ip_hdr->check = 0;
			ip_hdr->check = ip_checksum(ip_hdr, sizeof(struct iphdr));

			// if there is no MAC addr in the ARP table
			struct arp_entry *entry = get_arp_entry(ip_hdr->daddr);
			if (entry == NULL) {
				//send ARP request as broadcast
				struct ether_header *new_eth_hdr_1 = malloc(sizeof(struct ether_header));
				get_interface_mac(best_route->interface, new_eth_hdr_1->ether_shost);
				hwaddr_aton("ff:ff:ff:ff:ff:ff:ff", new_eth_hdr_1->ether_dhost);
				new_eth_hdr_1->ether_type = htons(ETHERTYPE_ARP);
				send_arp(best_route->next_hop, inet_addr(get_interface_ip(best_route->interface)),
				 	new_eth_hdr_1, best_route->interface, htons(ARPOP_REQUEST));

				// put a copy of the packet in the queue
				packet p;
				memcpy(&p, &m, sizeof(packet));
				queue_enq(queue, &p);
			}  else{
				// forwarding the updated packet
				get_interface_mac(best_route->interface, eth_hdr->ether_shost);
				memcpy(eth_hdr->ether_dhost, entry->mac, 6);
				send_packet(best_route->interface, &m);
			}
			continue;
		}

		// ARP
		if (eth_hdr->ether_type ==  htons(ETHERTYPE_ARP)) {
			if (arp_hdr != NULL) {
				if (arp_hdr->tpa == inet_addr(get_interface_ip(m.interface))) {
					// router receives an ARP request
					if (arp_hdr->op == htons(ARPOP_REQUEST)) {
						struct ether_header *new_eth_hdr = malloc(sizeof(struct ether_header));
						memcpy(new_eth_hdr->ether_dhost, eth_hdr->ether_shost, 6);
						get_interface_mac(m.interface, new_eth_hdr->ether_shost);
						new_eth_hdr->ether_type = eth_hdr->ether_type;

						// sending ARP reply 
						send_arp(arp_hdr->spa, arp_hdr->tpa, new_eth_hdr, 
							m.interface, htons(ARPOP_REPLY));
					}
				}

				// router receives an ARP reply
				if (arp_hdr->op == htons(ARPOP_REPLY)) {
					// updating the ARP table
					if (get_arp_entry(arp_hdr->spa) == NULL) {
						arp_table[arp_table_len].ip = arp_hdr->spa;
						memcpy(arp_table[arp_table_len].mac, arp_hdr->sha, 6);
						arp_table_len++;
					}

					// forwarding the packets stored in the queue if there are any
					if (!queue_empty(queue)) {
						packet packet_aux = *(packet*)queue_deq(queue);
						struct ether_header *eth_hdr_pack = (struct ether_header *)packet_aux.payload;
						struct iphdr *ip_hdr_p = (struct iphdr *)(packet_aux.payload 
							+ sizeof(struct ether_header));
						struct route_table* best_r = get_best_route(ip_hdr_p->daddr, 0, rtable_size - 1);
						memcpy(eth_hdr_pack->ether_dhost, arp_hdr->sha , 6);
						get_interface_mac(best_r->interface, eth_hdr_pack->ether_shost);
						send_packet(best_r->interface, &packet_aux);
					}
				}
			}
				
		}	
	}
}
