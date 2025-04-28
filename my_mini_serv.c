/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   my_mini_serv.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: glacroix <PGCL>                            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/04/23 14:17:01 by glacroix          #+#    #+#             */
/*   Updated: 2025/04/29 00:59:47 by glacroix         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>

int max_fd = 0;
int count = 0;
int ids[4096 * 16];

char *msgs[4096 * 16];
char buffer_write[1024 * 2];
char buffer_read[1024 * 2];

fd_set read_set, write_set, current_set;

//Start copy-paste from main
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
//End copy-paste from main

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

void remove_client(int fd)
{
		free(msgs[fd]);
		msgs[fd] = NULL;
		sprintf(buffer_write, "server: client %d just left\n", ids[fd]);
		send_broadcast(fd, buffer_write);
		FD_CLR(fd, &current_set);
		close(fd);

}


void register_client(int client_fd)
{
	max_fd = client_fd > max_fd ? client_fd : max_fd;
	ids[client_fd] = count++;
	msgs[client_fd] = NULL;
	sprintf(buffer_write, "server: client %d just arrived\n", ids[client_fd]);
	FD_SET(client_fd, &current_set);
	send_broadcast(client_fd, buffer_write);
}


void send_msg(int fd)
{
		char *msg;

		while (extract_message(&msgs[fd], &msg))
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
	int sockfd, client_fd;
	struct sockaddr_in servaddr, peeraddr; 

	FD_ZERO(&current_set);

	// socket create and verification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1)
			fatal_error();
	bzero(&servaddr, sizeof(servaddr)); 
	max_fd = sockfd;
	FD_SET(sockfd, &current_set);

	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(atoi(argv[1])); 
  
	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)))) 
			fatal_error();
	if (listen(sockfd, SOMAXCONN)) 	
		fatal_error();	
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
							socklen_t len = sizeof(peeraddr);
							client_fd = accept(sockfd, (struct sockaddr *)&peeraddr, &len);
							if (client_fd < 0) 
									fatal_error();
							else
							{
									register_client(client_fd);
									break;
							}

					}
					else
					{
						int read_bytes = recv(fd, buffer_read, 1000, 0);
						if (read_bytes == -1)
							   fatal_error();
						else if (read_bytes == 0)
						{
							remove_client(fd);
							break;
						}	
						else
						{
							buffer_read[read_bytes] = '\0';
							msgs[fd] = str_join(msgs[fd], buffer_read);
							send_msg(fd);
						}
					}

			}
	}
	return 0;
}
