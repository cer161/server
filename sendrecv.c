#define _POSIX_C_SOURCE 200112L
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>

#define BUFSIZE 128

int main(int argc, char **argv)
{
	struct addrinfo hints, *info_list, *info;
	int error;
	int sock;
	int bytes;
	char buf[BUFSIZE+1];
	
	if (argc != 3) {
		printf("Usage: %s [host] [port]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	error = getaddrinfo(argv[1], argv[2], &hints, &info_list);
	if (error) {
		fprintf(stderr, "%s\n", gai_strerror(error));
		exit(EXIT_FAILURE);
	}

	
	for (info = info_list; info != NULL; info = info->ai_next) {
		sock = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
		
		if (sock < 0) continue;
		
		if (connect(sock, info->ai_addr, info->ai_addrlen) == 0) {
			break;
		}

		close(sock);
	}
	if (info == NULL) {
		fprintf(stderr, "Could not connect to %s:%s\n", argv[1], argv[2]);
		exit(EXIT_FAILURE);
	}
	
	freeaddrinfo(info_list);

	// send message(s) to server
	char *msg = "GET\n4\ndays\n";   // too long
	int len = strlen(msg);
	
	bytes = write(sock, msg, len);
	// FIXME: should check how many bytes were written
	//   and re-send any remaining bytes
	
	
// 	write(sock, "GET\n6\nh", 7);
// 	sleep(5);
// 	write(sock, "ello\n", 5);
	
	while ((bytes = read(sock, buf, BUFSIZE)) > 0) {
		buf[bytes] = '\0';
		printf("Got %d bytes: |%s|\n", bytes, buf);
	}
	
	close(sock);  // pedantic: should check if this failed somehow

	return EXIT_SUCCESS;
}
