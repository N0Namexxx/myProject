#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <string>
#include <map>


#define MAX_CONN 1024

struct Client
{
	int sockfd;
	std::string name;
};


int main()
{
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("socket error");
		exit(1);
	}
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(9999);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	int ref;
	ref = bind(sockfd, (struct sockaddr*)&addr, sizeof(addr));
	if (ref < 0) {
		perror("bind error");
		exit(1);
	}
	ref = listen(sockfd, 1024);
	if (ref < 0) {
		perror("listen error");
		exit(1);
	}
	int epfd = epoll_create1(0);
	if (epfd < 0) {
		perror("epoll create error");
		exit(1);
	}
	struct epoll_event ev;
	ev.data.fd = sockfd;
	ev.events = EPOLLIN;
	ref = epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev);
	if (ref < 0) {
		perror("epoll control error");
		exit(1);
	}

	std::map<int, Client> clients;

	while (1) {
		struct epoll_event evs[MAX_CONN];
		int n = epoll_wait(epfd, evs, MAX_CONN, -1);
		if (n < 0) {
			perror("epoll wait error");
			exit(1);
		}
		for (int i = 0; i < n; i++) {
			int fd = evs[i].data.fd;
			if (fd == sockfd) {
				struct sockaddr_in client_addr;
				socklen_t client_addr_len = sizeof(client_addr);
				int client_sockfd = accept(fd, (sockaddr*)&client_addr, &client_addr_len);
				if (client_sockfd < 0) {
					perror("accept error");
					exit(1);
				}

				struct epoll_event ev_client;
				ev_client.data.fd = client_sockfd;
				ev_client.events = EPOLLIN;
				ref = epoll_ctl(epfd, EPOLL_CTL_ADD, client_sockfd, &ev_client);
				if (ref < 0) {
					perror("epoll_ctr error");
					exit(1);
				}

				printf("%s connecting...\n", inet_ntoa(client_addr.sin_addr));

				struct Client client;
				client.sockfd = client_sockfd;
				client.name = "";
				clients[client_sockfd] = client;
			}
			else {
				char buffer[1024];
				int n = read(fd, buffer, 1024);
				if (n < 0) {
					perror("read error");
					exit(1);
				}
				else
				{
					if (n == 0) {
						close(fd);
						ref = epoll_ctl(epfd, EPOLL_CTL_DEL, fd, 0);
						if (ref < 0) {
							perror("delete error");
							exit(1);
						}
						clients.erase(fd);
					}
					else
					{
						std::string msg(buffer, n);
						if (clients[fd].name == "") {
							clients[fd].name = msg;
						}
						else
						{
							std::string name = clients[fd].name;
							for (auto& e : clients) {
								if (e.first != fd) {
									write(e.first,('['+name+"]: "+msg).c_str(), sizeof(name) + sizeof(msg)+4);


								}
							}
						}
					}
				}
				std::string msg;
			}
		}
	}
	close(epfd);
	close(sockfd);
}

