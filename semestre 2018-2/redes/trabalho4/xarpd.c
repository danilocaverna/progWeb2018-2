#include <errno.h>
#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/ethernet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <pthread.h>
#include <semaphore.h>
#include <err.h>

/*Cores*/
//fonte - http://ascii-table.com/ansi-escape-sequences.php
#define KNRM  "\x1B[0m"     //printf("%snormal\n", KNRM);
#define KBLK  "\x1B[30m"     //printf("%black\n", KBLK);
#define KRED  "\x1B[31m"    //printf("%sred\n", KRED);
#define KGRN  "\x1B[32m"    //printf("%sgreen\n", KGRN);
#define KYEL  "\x1B[33m"    //printf("%syellow\n", KYEL);
#define KBLU  "\x1B[34m"    //printf("%sblue\n", KBLU);
#define KMAG  "\x1B[35m"    //printf("%smagenta\n", KMAG);
#define KCYN  "\x1B[36m"    //printf("%scyan\n", KCYN);
#define KWHT  "\x1B[37m"    //printf("%swhite\n", KWHT);
/* */
#define MAX_PACKET_SIZE 65535
#define MIN_PACKET_SIZE 20
/* */
#define MAX_IFACES	64
#define MAX_IFNAME_LEN	22
#define ETH_ADDR_LEN	6

struct arp_node{
	int 			ttl;
	unsigned char	sha[6];	//sender mac - sender hardware address
	unsigned char	spa[4]; //sender ip - sender protocal address
	struct arp_node *next;
};

struct mensagem{
	unsigned char	ip[4];
	unsigned char	mac[6];
    int             ttl;
    char            *comando;
};
/* */
struct iface {
	int				sockfd;
	int				ttl;
	int				mtu;
	char			ifname[MAX_IFNAME_LEN];
	unsigned char	mac_addr[6];
	unsigned int	ip_addr;
	unsigned int	rx_pkts;
	unsigned int	rx_bytes;
	unsigned int	tx_pkts;
	unsigned int	tx_bytes;
};
/* */
struct ether_hdr {
	unsigned char	ether_dhost[6];	// Destination address
	unsigned char	ether_shost[6];	// Source address
	unsigned short	ether_type;	// Type of the payload
};
/* */
struct ip_hdr {

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	unsigned char	ip_ihl:4,
			ip_v:4;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	unsigned char	ip_ihl:4,
			ip_v:4;
#endif	
	unsigned char	ip_tos;		// Type of service
	unsigned short	ip_len;		// Datagram Length
	unsigned short	ip_id;		// Datagram identifier
	unsigned short	ip_offset;	// Fragment offset
	unsigned char	ip_ttl;		// Time To Live
	unsigned char	ip_proto;	// Protocol
	unsigned short	ip_csum;	// Header checksum
	unsigned int	ip_src;		// Source IP address
	unsigned int	ip_dst;		// Destination IP address
};
/* */
// Read RFC 826 to define the ARP struct
struct arp_hdr{
	unsigned short	htype;	//hardware type
	unsigned short	ptype;	//protocol type
	unsigned char	hlen;	//hardware address length
	unsigned char	plen;	//protocol address length
	unsigned short	opcode;	//operation
	unsigned char	sha[6];	//sender mac - sender hardware address
	unsigned char	spa[4];	//sender ip - sender protocal address
	unsigned char	tha[6];	//target mac - target hardware address
	unsigned char	tpa[4];	//target_ip - target protocol address
};      



//global var
sem_t mutex;
struct iface my_ifaces[MAX_IFACES];
struct arp_node *head_node;

//prototypes
void daemonize ();
void *handle_arp_cache();
void print_eth_address(char *s, unsigned char *eth_addr);
int	 bind_iface_name(int fd, char *iface_name);
void get_iface_info(int sockfd, char *ifname, struct iface *ifn);
void print_usage();
void doProcess(unsigned char* packet, int len);
void *read_iface(void *arg);
//xarp functions
void *answer_xarp();
void show();
void res();
void add();
void del();
void ttl();

int main(int argc, char** argv) {
	int		i, sockfd, aux_thread;
	pthread_t threads[MAX_IFACES], cache_arp_thread, answer_xarp_thread;
	
	if (argc < 2) print_usage();
	daemonize();
	head_node = (struct arp_node *) malloc(sizeof(struct arp_node));
	head_node->next = NULL;
	sem_init(&mutex, 0, 1);

	for (i = 1; i < argc; i++) {
		sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));  
		if(sockfd < 0) {
			fprintf(stderr, "ERROR: %s\n", strerror(errno));
			exit(1);
		}
		
		if (bind_iface_name(sockfd, argv[i]) < 0) {
			perror("Server-setsockopt() error for SO_BINDTODEVICE");
			printf("%s\n", strerror(errno));
			close(sockfd);
			exit(1);
		}
		get_iface_info(sockfd, argv[i], &my_ifaces[i-1]);
	}

	for (i = 0; i < argc-1; i++) {
		print_eth_address(my_ifaces[i].ifname, my_ifaces[i].mac_addr);
		printf("\n");
		// Create one thread for each interface. Each thread should run the function read_iface.
		aux_thread = pthread_create(&threads[i], NULL, read_iface, &(my_ifaces[i]));
		if (aux_thread){
			printf("thread erro num: %d\n", aux_thread);
			exit(-1);
		}
	}

	aux_thread = pthread_create(&cache_arp_thread, NULL, handle_arp_cache, NULL);
	if (aux_thread){
		printf("thread handle_arp_cache erro num: %d\n", aux_thread);
		exit(-1);
	}

	aux_thread = pthread_create(&answer_xarp_thread, NULL, answer_xarp, NULL);
	if (aux_thread){
		printf("thread answer_xarp_thread erro num: %d\n", aux_thread);
		exit(-1);
	}

	pthread_exit(NULL);
	return 0;
}



// (para finalizar os deamons)
void daemonize(){
    if (fork() != 0) 
	    exit(1);
}
void *handle_arp_cache(){
	struct arp_node *node;
	struct arp_node *node_pai;
	int i;
	while(1){
		sem_wait(&mutex);
		node = head_node->next;
		node_pai = head_node;
		i = 0;

		while(node != NULL){
			i++;
			node->ttl--;
			if(node->ttl == 0){
				node_pai->next = node->next;
				free(node);
				node = NULL;
				if(node_pai != NULL)
					node = node_pai->next;
			}
			else if(node->ttl > 0){
				node_pai = node_pai->next;
				node = node->next;
			}
			else
				node->ttl++;
		}
		if(i > 0)
		printf("nodes: %d\n", i);

		sem_post(&mutex);
		sleep(1);
	}
}
// Print an Ethernet address
void print_eth_address(char *s, unsigned char *eth_addr){
	printf("%s %02X:%02X:%02X:%02X:%02X:%02X", s,
	       eth_addr[0], eth_addr[1], eth_addr[2],
	       eth_addr[3], eth_addr[4], eth_addr[5]);
}
// Bind a socket to an interface
int bind_iface_name(int fd, char *iface_name){
	return setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, iface_name, strlen(iface_name));
}
void get_iface_info(int sockfd, char *ifname, struct iface *ifn){
	struct ifreq s;
	
	strcpy(s.ifr_name, ifname);
	if (0 == ioctl(sockfd, SIOCGIFHWADDR, &s)) {
		memcpy(ifn->mac_addr, s.ifr_addr.sa_data, ETH_ADDR_LEN);
		ifn->sockfd = sockfd;
		strcpy(ifn->ifname, ifname);
	} else {
		perror("Error getting MAC address");
		exit(1);
	}
}
// Print the expected command line for the program
void print_usage(){
	printf("\n%sxarpd %s<interface> %s[%s<interface> %s...]\n", KBLU, KGRN, KNRM, KGRN, KNRM);
	exit(1);
}
// Break this function to implement the ARP functionalities.
void doProcess(unsigned char* packet, int len){
	if(!len || len < MIN_PACKET_SIZE)
		return;
	int i;
	struct ether_hdr* eth = (struct ether_hdr*) packet;
	struct arp_hdr* arp;
	struct arp_hdr *rec_arp;
	struct arp_node *node;
	struct arp_node *node_pai;
	
	if(htons(0x0806) == eth->ether_type) {
		// ARP
		//link https://www.tldp.org/LDP/nag/node78.html arp -a lista a tabela arp
		arp = (struct arp_hdr *) (packet + 14);//+14 ignora header ethernet
		sem_wait(&mutex);
		node = head_node->next;
		node_pai = head_node;

		//salvando na tabela
		while(node != NULL && memcmp(node->sha, arp->sha, (int) sizeof(unsigned char) * 6) != 0 && memcmp(node->spa, arp->spa, (int) sizeof(unsigned char) * 4) != 0){
			node_pai = node_pai->next;
			node = node->next;
		}
		if(node == NULL){
			node = (struct arp_node *) malloc(sizeof(struct arp_node));
			node_pai->next = node;
			node->next = NULL;
			strncpy(node->sha, arp->sha, (int) sizeof(unsigned char) * 6);
			strncpy(node->spa, arp->spa, (int) sizeof(unsigned char) * 4);
		}
		node->ttl = 10;

		sem_post(&mutex);
	}
	// Ignore if it is not an ARP packet
}
// This function should be one thread for each interface.
void *read_iface(void *arg){
	struct iface *ifn = (struct iface*)arg;
	socklen_t	saddr_len;
	struct sockaddr	saddr;
	unsigned char	*packet_buffer;
	int		n;
	
	saddr_len = sizeof(saddr);	
	packet_buffer = malloc(MAX_PACKET_SIZE);
	if (!packet_buffer) {
		printf("\nCould not allocate a packet buffer\n");		
		exit(1);
	}
	
	while(1) {
		n = recvfrom(ifn->sockfd, packet_buffer, MAX_PACKET_SIZE, 0, &saddr, &saddr_len);
		if(n < 0) {
			fprintf(stderr, "ERROR: %s\n", strerror(errno));
			exit(1);
		}
		doProcess(packet_buffer, n);
	}
}







void *answer_xarp(){
	int one = 1, client_fd, n;
	struct sockaddr_in server_address, client_address;
	socklen_t client_address_lenght = sizeof(client_address);
	unsigned char *packet_buffer;
	struct mensagem *msg;

	packet_buffer = malloc(MAX_PACKET_SIZE);
	if (!packet_buffer) {
		printf("\nCould not allocate a packet buffer\n");		
		exit(1);
	}
	int server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if(server_socket < 0)
		err(1, "DESCRIPTION: Can't open socket\nERROR");

	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = INADDR_ANY;
	server_address.sin_port = htons(atoi("5058"));

	if (bind(server_socket, (struct sockaddr *) &server_address, sizeof(server_address)) == -1) {
		close(server_socket);
		err(1, "DESCRIPTION: Can't bind\nERROR");
	}

	if(listen(server_socket, 10) < 0) {
		fprintf(stderr, "ERROR: %s\n", strerror(errno));
		exit(1);
	}
	while (1) {
		client_fd = accept(server_socket, (struct sockaddr *) &client_address, &client_address_lenght);
		printf("got connection\n");

		//n = recvfrom(server_socket, packet_buffer, MAX_PACKET_SIZE, 0, (struct sockaddr *) &client_address, &client_address_lenght);
		n = recv(server_socket, packet_buffer, sizeof(packet_buffer) - 128, 0);
		if(n < 0) {
			fprintf(stderr, "ERROR: %s\n", strerror(errno));
			exit(1);
		}


		msg = (struct mensagem *) packet_buffer;
		if(strcmp(msg->comando, "show"))
			show();
		else if(strcmp(msg->comando, "res"))
			res();
		else if(strcmp(msg->comando, "add"))
			add();
		else if(strcmp(msg->comando, "del"))
			del();
		else if(strcmp(msg->comando, "ttl"))
			ttl();

		if (client_fd == -1) {
			perror("DESCRIPTION: Can't accept\nERROR");
			continue;
		}
		if (send(client_fd, packet_buffer, sizeof(packet_buffer) - 1, 0) < 0){ /*-1:'\0'*/
			perror("DESCRIPTION: Message was not sent\nERROR");
			continue;
		}
		close(client_fd);
	}
}
void show();
void res();
void add();
void del();
void ttl();