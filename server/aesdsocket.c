#include <stdlib.h>
#include <stdio.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <stdbool.h>
#include <fcntl.h>

#define PATH "/var/tmp/aesdsocketdata"
#define BUF_SIZE 512
#define MTU_SIZE 512

int sock_fd, client_fd, o_fd;
bool running;

void *get_in_addr(struct sockaddr *sa)
{
	/* Beej's Guide to Network Programming */
	if (sa->sa_family == AF_INET)
		return &(((struct sockaddr_in*)sa)->sin_addr);
	else
		return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void fappend(char *out, size_t out_len) {
	if (write(o_fd, out, out_len) < 0) {
		syslog(LOG_ERR, "write: %s\n", strerror(errno));
	}
}

void fsend() {
	lseek(o_fd, 0, SEEK_SET);
	char chunk[MTU_SIZE];
	size_t pos = 0;
	ssize_t bytes_out;
	while (bytes_out = read(o_fd, chunk, MTU_SIZE)) {
		if (bytes_out == -1)
			syslog(LOG_ERR, "read: %s\n", strerror(errno));
		if (bytes_out <= 0)
			break;
		if (send(client_fd, chunk, bytes_out, 0) == -1) {
			syslog(LOG_ERR, "send: %s\n", strerror(errno));
			return;
		}
	}
}

void bufexpand(char **buf, size_t *buf_size, size_t buf_content) {
	*buf_size *= 2;
	char *new = malloc(*buf_size);
	if (new == NULL) {
		syslog(LOG_ERR, "malloc: %s\n",	strerror(errno));
		exit(EXIT_FAILURE);
	}
	memcpy(new, *buf, buf_content);
	free(*buf);
	*buf = new;
}





void signal_handler(int signal_number){
	
	syslog(LOG_DEBUG, " Caught signal, exiting");
	running = false;
	remove("/var/tmp/aesdsocketdata");
	close(sock_fd); 
	
}

int main(int argc, char *argv[]){
	
	openlog("syslog",LOG_PID | LOG_CONS ,LOG_USER);

	bool is_daemon = false;
	
	if (argc > 1 && strcmp(argv[1], "-d") == 0){
		    			is_daemon = true;
		  			}
		  			
	struct addrinfo hints, *res;
	
	hints.ai_family = AF_UNSPEC; 
	hints.ai_flags = AI_PASSIVE; 
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_socktype = SOCK_STREAM;
	
	int r = getaddrinfo(NULL,"9000", &hints, &res);
	if (r != 0) {
		syslog(LOG_ERR, "getaddrinfo error: %s\n", gai_strerror(r));
		exit(-1);
		}
		
	sock_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (sock_fd == -1) {
		syslog(LOG_ERR, "socket: %s\n", strerror(errno));
		exit(-1);
		}
		
	if (bind(sock_fd, res->ai_addr, res->ai_addrlen) == -1) {
		syslog(LOG_ERR, "bind: %s\n", strerror(errno));
		exit(-1);
		}
		
	freeaddrinfo(res);

	if (listen(sock_fd, 2) == -1) {
		syslog(LOG_ERR, "listen: %s\n", strerror(errno));
		exit(-1);
		}

	o_fd = open(PATH, O_CREAT | O_RDWR | O_TRUNC | O_APPEND, 0644);
	
	if (o_fd < 0) {
		syslog(LOG_ERR, "open %s: %s\n", PATH, strerror(errno));
		exit(EXIT_FAILURE);
		}

	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);


	if (is_daemon) {
		pid_t pid = fork();
		if (pid == -1) {
			syslog(LOG_ERR, "fork: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
			}
			
		if (pid != 0) {
			fprintf(stdout, "Kicked off daemon: %d\n", pid);
			exit(EXIT_SUCCESS);
			}
			
		if (setsid() == -1) {
			syslog(LOG_ERR, "setsid: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
			}
			
		if (chdir("/") == -1) {
			syslog(LOG_ERR, "chdir: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
			}
			
		close(fileno(stdin));
		close(fileno(stdout));
		close(fileno(stderr));
	}

	struct sockaddr_storage addr;
	socklen_t addrlen;
	char addr_str[INET6_ADDRSTRLEN];

	char *buf = malloc(BUF_SIZE);
	if (buf == NULL) {
		syslog(LOG_ERR, "malloc: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}
	char *l; /* newline pointer */
	size_t buf_content; /* content size */
	size_t buf_size; /* buffer size */
	size_t bytes_in; /* bytes read */

	syslog(LOG_INFO, "%s\n", "Listening for connections");
	while (running) {
		addrlen = sizeof addr;
		client_fd = accept(sock_fd, (struct sockaddr *)&addr, &addrlen);
		if (client_fd == -1) {
			syslog(LOG_ERR, "accept: %s\n", strerror(errno));
			continue;
			}
			
		inet_ntop(addr.ss_family,
				get_in_addr((struct sockaddr *)&addr),
				addr_str, sizeof addr_str);
		syslog(LOG_INFO, "Accepted connection from %s\n", addr_str);

		l = NULL;
		buf_content = 0;
		buf_size = BUF_SIZE;
		bytes_in = -1;
		while (bytes_in != 0) {
			if (buf_content == buf_size)
				bufexpand(&buf, &buf_size, buf_content);
			bytes_in = recv(client_fd, buf+buf_content,
					buf_size - buf_content, 0);
			buf_content += bytes_in;
			l = memchr(buf, '\n', buf_content);
			if (l != NULL) {
				/* handle newline */
				l += 1;
				fappend(buf, l - buf);
				buf_content -= l - buf;
				memcpy(buf, l, buf_content);
				fsend();
			}
		}

		close(client_fd);
		syslog(LOG_INFO, "Closed connection from %s\n", addr_str);
	}

	free(buf);
	close(sock_fd);
	close(o_fd);
	if (remove(PATH) != 0)
		syslog(LOG_ERR, "remove %s: %s\n", PATH, strerror(errno));
	closelog();
	exit(EXIT_SUCCESS);
}





















