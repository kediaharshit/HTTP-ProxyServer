/*
Author :- Harshit Kedia
Roll No:- CS17B103
references:- Problem statement, section 5,6
			 GeeksforGeeks
*/

#include "proxy_parse.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <arpa/inet.h> 

int main(int argc, char * argv[]) {
	int PORT = 8080;
	if(argc == 2)
		PORT = atoi(argv[1]);
	else
		printf("No port specified, using default of 8080\n");

	int server_fd, new_socket,valread;
	struct sockaddr_in address;
	int opt = 1;
	int addrlen = sizeof(address);
	char buffer[1024] = {0};
	//create socket object
	if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
	{
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}	
	if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
	{
		perror("setsockopt"); 
        exit(EXIT_FAILURE); 
	}	//configure socket and address
 	address.sin_family = AF_INET; 
    address.sin_addr.s_addr = INADDR_ANY; 
    address.sin_port = htons(PORT); 
    printf("Binding to %d\n", PORT);

    //bind/attach to a PORT in system
    if(bind(server_fd, (struct sockaddr *)&address,sizeof(address))<0) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    }	
    if(listen(server_fd, 3) < 0) 
    { 
        perror("listen faileds"); 
        exit(EXIT_FAILURE); 
    }
    printf("proxy up and listening...\n");
    int rc;
    //now we are ready for incoming TCP connections
    while(1)
    {
	    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) 
	    { 
	        perror("error accepting"); 
	        exit(EXIT_FAILURE); 
	    } 
	    rc = fork();
	    if(rc < 0) // fork failed
	    {
	    	close(new_socket);
	    	continue;
	    } 
	    else if(rc > 0)	//parent
	    {
	    	close(new_socket);

	    	continue;
	    }
	    else if (rc == 0) //child
	    {
	    	int len;
	    	char buff[4096] = {0};
	    	while(true)	//read how until empty line is sent by client, i.e, consecutive enters
	    	{
	    		valread = read(new_socket , buffer, 1024);
				len = strlen(buff); 
				strcat(buff, buffer);
				bzero(buffer, 1024);
				if(strstr(buff, "\r\n\r\n"))
					break;
				if(valread==0)
					break;
		    }
		    strcat(buff, "\r\n\r\n"); //safety reasons of parsing
		    if(strstr(buff, "ApacheBench")!=NULL)	//checks ping tyoe only
		    	exit(0);

		    struct ParsedRequest *req = ParsedRequest_create();
			if(ParsedRequest_parse(req, buff, strlen(buff)) < 0) 
			{
			    printf("parse failed\n");
			    break;
			}

			// be the client for this request,
			// establish connection, get data, pass-on to  my client 
			int fwd_sock;
			struct sockaddr_in serv_addr;

			if((fwd_sock = socket(AF_INET, SOCK_STREAM, 0)) ==-1)
			{
				printf("socket creation failed");
				exit(EXIT_FAILURE);
			}

			//get ip from hostname, snipped from geeksforgeeks.com
			struct hostent *host_entry;
			host_entry = gethostbyname(req->host);
			char ip_host[32] = {0};
			strcpy(ip_host, inet_ntoa(*((struct in_addr*)host_entry->h_addr_list[0])));

			//forward-destination ip, port 
			serv_addr.sin_family = AF_INET;
			serv_addr.sin_addr.s_addr = inet_addr(ip_host);
			serv_addr.sin_port = htons(80);

		 //    printf("dest Host: %s\n", req->host);
			// printf("dest IP  : %s\n", ip_host);

			//connect to desired host
			if (connect(fwd_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
		    { 
		        printf("\nForward Connection Failed \n"); 
		        close(new_socket);
		        return -1; 
		    } 
		    // else  	printf("Established connection to server\n");
		    
			//prepare the HTTP formatted request
			char request[4096] = {0};
			char recv[4096] = {0};

			strcat(request, "GET ");
			strcat(request, req->path);
			strcat(request, " ");
			strcat(request, req->version);
			strcat(request, "\r\nHost:\t");
			strcat(request, req->host);
			strcat(request, "\r\nConnection:\tclose\r\n\r\n");

			//pass on the data receive from server to the client
			write(fwd_sock, request, strlen(request));
			while(read(fwd_sock, recv, 4096)>0)
			{
				write(new_socket, recv, strlen(recv));
				bzero(recv, 4096);
			}
			close(new_socket);
		    exit(0);
		    // job done
	    }	  
  	}
  	return 0;
}
