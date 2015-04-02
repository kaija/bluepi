#include <stdio.h>
#include "dumper.h"
#include "log.h"

void beacon_raw_dump(unsigned char *data, int len)
{
	//filter beacon
	if(len != 69) return;
	int i = 0;
//#ifdef DUMPALL
#if 0
	for(i = 0; i< len; i++){
		if((i+1)%16 == 1){
			printf("\n");
		}
		printf("%02X ", data[i]);
	}
#else
	printf("UUID:");
	for(i=0; i<16; i++){
		printf("%02X ", data[i+47]);
	}
	printf("  Signal %d", data[68]);
	printf("\n");
#endif
	fflush(stdout);
}


int beacon_filter(unsigned char *data, int len)
{
	int i = 0;
	unsigned char uuid[16];
	char rssi;
	if(len == 69){
		for(i=0; i<16; i++){
			uuid[i] = data[i+47];
		}
		rssi = data[68];
		printf("RSSI %d\n",rssi );
	}
}

void beacon_socket_close(int sk)
{
	if(sk > 0) close(sk);
}
int beacon_socket_create()
{
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if(fd < 0){
		LOG_ERROR("Socket create failure %d(%s)\n", errno, strerror(errno));
		return fd;
	}
	//Default enable nonblock / reuseaddr and set send / recv timeout
	//khttp_socket_nonblock(fd, 1);
	beacon_socket_reuseaddr(fd, 1);
	return fd;
}

int beacon_socket_nonblock(int fd, int enable)
{
	unsigned long on = enable;
	int ret = ioctl(fd, FIONBIO, &on);
	if(ret != 0){
		LOG_WARN("Set socket nonblock failure %d(%s)\n", errno, strerror(errno));
	}
	return ret;
}

int beacon_socket_reuseaddr(int fd, int enable)
{
    int ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&enable, sizeof(enable));
    if(ret != 0){
        LOG_WARN("Set socket reuseaddr failure %d(%s)\n", errno, strerror(errno));
    }
    return ret;
}

int beacon_socket_connect(int sk, char *addr, int port)
{
	struct addrinfo hints;
	struct addrinfo *result;
	memset(&hints, 0, sizeof(hints));
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_family = AF_INET;
	int res = 0;
    char port_str[16];
    sprintf(port_str, "%d", port);
    if((res = getaddrinfo(addr, port_str, &hints, &result)) != 0){
        LOG_ERROR("DNS lookup failure. getaddrinfo: %s\n", gai_strerror(res));
        goto err;
    }
	struct sockaddr_in  serv_addr;
	serv_addr.sin_addr = ((struct sockaddr_in *)result->ai_addr)->sin_addr;
	serv_addr.sin_port = htons(port);
	if(connect(sk, result->ai_addr, result->ai_addrlen)!= 0) {
		goto err1;
	}
	LOG_INFO("Connected to %s:%d\n", addr, port);
	freeaddrinfo(result);
	return 0;
err1:
	freeaddrinfo(result);
err:
	return -1;
}

int run()
{
	char addr[] = "localhost";
	int port = 10839;
	int sk = beacon_socket_create();
	if(sk < 0 ){
		return -1;
	}
	beacon_socket_nonblock(sk, 0);
	beacon_socket_reuseaddr(sk, 1);
	if(beacon_socket_connect(sk, addr, port) != 0){
		LOG_ERROR("Connect to server %s:%d failure\n", addr, port);
		return -1;
	}
	int start = 1;
	while(start){
		fd_set fs;
		FD_ZERO(&fs);
		FD_SET(sk ,&fs);
		int ret = select(sk + 1, &fs, NULL, NULL, NULL);
		if(ret > 0 ){
			//Assume socket receive the data
			if(FD_ISSET(sk, &fs)){
				char buf[2048];
				int len = recv(sk, buf, 2048, 0);
				if(len > 0){
					//LOG_DEBUG("Get %d byte from socket\n", len);
					beacon_raw_dump(buf, len);
				}else{
					beacon_socket_close(sk);
					start = 0;
				}
			}
		}else if(ret == 0){
			//select timeout
		}else{
			LOG_ERROR("Disconnect from hcibus\n");
		}
	}
	LOG_INFO("Disconnect from server\n");
	return 0;
}

int main()
{
	while(1){
		run();
		sleep(5);
	}
    return 0;
}
