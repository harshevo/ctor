/* toralize.c */
#include "toralize.h"
#include <arpa/inet.h>
#include <dlfcn.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

Req *request(struct sockaddr_in *sock2)
{
	Req *req;

	req = malloc(reqsize);
	req->vn = 4;
	req->cd = 1;

	req->dstport = sock2->sin_port;
	req->dstip = sock2->sin_addr.s_addr;

	strncpy((char *)(req->userid), USERNAME, 8);

	return req;
}

/*
char *dns_to_ip(const char *hostname)
{
	struct addrinfo hints, *res, *p;
	int status;
	char *ipstr = malloc(INET_ADDRSTRLEN);

	memset(&hints, 0, sizeof(hints));

	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	printf("%s\n", hostname);
	if ((status = getaddrinfo(hostname, NULL, &hints, &res)) != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
		free(ipstr);
		return NULL;
	}

	for (p = res; p != NULL; p = p->ai_next)
	{
		void *addr;
		const char *ipver;

		if (p->ai_family == AF_INET)
		{
			struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
			addr = &(ipv4->sin_addr);
			ipver = "IPv4";
		}
		inet_ntop(p->ai_family, addr, ipstr, INET_ADDRSTRLEN);
	}

	freeaddrinfo(res);

	return ipstr;
}
*/

int connect(int s2, const struct sockaddr *sock2, socklen_t addrlen)
{

	int s;
	struct sockaddr_in sock;
	Req *req;
	char buf[ressize];
	Res *res;
	int success;
	char tmp[512];

	int (*p)(int, const struct sockaddr *, socklen_t);

	p = dlsym(RTLD_NEXT, "connect");

	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0)
	{
		perror("socket");
		return -1;
	}

	sock.sin_family = AF_INET;
	sock.sin_port = htons(PROXYPORT);
	sock.sin_addr.s_addr = inet_addr(PROXY);

	if (p(s, (struct sockaddr *)&sock, sizeof(sock)))
	{
		perror("connect");
		return -1;
	}

	printf("Connected to Proxy Server \n");

	req = request((struct sockaddr_in *)sock2);
	write(s, req, reqsize);

	memset(buf, 0, ressize);

	if (read(s, buf, ressize) < 1)
	{
		perror("read");
		free(req);
		close(s);
		return -1;
	}

	res = (Res *)buf;
	success = (res->cd == 90);

	if (!success)
	{
		fprintf(stderr, "Unable to traverse the proxy, error code: %d\n", res->cd);

		close(s);
		free(req);
		return -1;
	}

	printf("Successfully connected through the proxy");
	dup2(s, s2);
	free(req);

	return 0;
}
