/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   my_mini_serv.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: glacroix <marvin@42.fr>                    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/22 13:11:59 by glacroix          #+#    #+#             */
/*   Updated: 2025/04/22 17:21:33 by glacroix         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/select.h> // fd_set in Linux
#include <netinet/ip.h>

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>


int		count = 0;
int		max_fd = 0;
int		ids[65536];

char	*msgs[65536];
char	buffer_read[1024 * 2];
char	buffer_write[1024 * 2];

fd_set read_set, write_set, current_set;

//START COPY PASTE
int extract_message(char **buf, char **msg)
{
	char	*newbuf;
	int	i;

	*msg = 0;
	if (*buf == 0)
		return (0);
	i = 0;
	while ((*buf)[i])
	{
		if ((*buf)[i] == '\n')
		{
			newbuf = calloc(1, sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
			if (newbuf == 0)
				return (-1);
			strcpy(newbuf, *buf + i + 1);
			*msg = *buf;
			(*msg)[i + 1] = 0;
			*buf = newbuf;
			return (1);
		}
		i++;
	}
	return (0);
}

char *str_join(char *buf, char *add)
{
	char	*newbuf;
	int		len;

	if (buf == 0)
		len = 0;
	else
		len = strlen(buf);
	newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
	if (newbuf == 0)
		return (0);
	newbuf[0] = 0;
	if (buf != 0)
		strcat(newbuf, buf);
	free(buf);
	strcat(newbuf, add);
	return (newbuf);
}
// END COPY-PASTE

void fatal_error()
{
		write(2, "Fatal error\n", 12);
		exit(1);
}

void send_broadcast(int author, char *msg)
{
		for (int fd = 0; fd <= max_fd; fd++)
		{
				if (FD_ISSET(fd, &write_set) && fd != author)
						send(fd, msg, strlen(msg), 0);
		}
}

void register_client(int client_fd)
{
	max_fd = client_fd > max_fd ? client_fd : max_fd;
	ids[client_fd] = count++;
	msgs[client_fd] = NULL;
	FD_SET(client_fd, &current_set);
	sprintf(buffer_write, "server: client %d just arrived\n", ids[client_fd]);
	printf("buffer: %s", buffer_write);
	send_broadcast(client_fd, buffer_write);
}

void remove_client(int client_fd)
{
		sprintf(buffer_write, "server: client %d just left\n", ids[client_fd]);
		send_broadcast(client_fd, buffer_write);
		free(msgs[client_fd]);
		FD_CLR(client_fd, &current_set);
		close(client_fd);
}

void send_msg(int fd)
{
		char *msg;
		
		while (extract_message(&(msgs[fd]), &msg))
		{
				sprintf(buffer_write, "client %d: ", ids[fd]);
				send_broadcast(fd, buffer_write);
				send_broadcast(fd, msg);
				free(msg);
		}
}

int main(int argc, char **argv)
{
		if (argc != 2)
		{
				write(2, "Wrong number of arguments\n", 26);
			  	exit(1);	
		}
		int sockfd, connfd;

		//initializing set of fds
		FD_ZERO(&current_set);

		//creating socket
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd == -1)
				fatal_error();
		FD_SET(sockfd, &current_set);
		max_fd = sockfd; //keeping track of the number of fds to watch 
						 //by default select skips 0-2
		
		//START COPY-PASTE FROM MAIN
		struct sockaddr_in servaddr;
		bzero(&servaddr, sizeof(servaddr));

		servaddr.sin_family = AF_INET;
		servaddr.sin_addr.s_addr = htonl(2130706433);
		servaddr.sin_port = htons(atoi(argv[1])); // replace 8080
		printf("Port number is: %d\n", atoi(argv[1]));

		if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)))
				fatal_error();
		if (listen(sockfd, SOMAXCONN)) 
				fatal_error();
		// END COPY-PASTE

		while (1)
		{
			read_set = write_set = current_set;
			if (select(max_fd + 1, &read_set, &write_set, NULL, NULL) < 0)
					fatal_error();
			for (int fd = 0; fd <= max_fd; fd++)
			{
					if (!FD_ISSET(fd, &read_set))
							continue;
					if (fd == sockfd)
					{
							socklen_t addr_len = sizeof(servaddr);
							int client_fd = accept(sockfd, (struct sockaddr *)&servaddr, &addr_len);
							printf("are we here|  clientFd: %d\n", client_fd);
							if (client_fd >= 0) //register client to fd set	
							{
									register_client(client_fd);
									break;
							}
					}
					else
					{
							int read_bytes =  recv(fd, buffer_read, 1000, 0);
							if (read_bytes  <= 0)
							{
									remove_client(fd);
									break;
							}
							buffer_read[read_bytes] = '\0';
							msgs[fd] = str_join(msgs[fd], buffer_read);
							send_msg(fd);
					}
			}
		}


		return 0;
}
