#include <unistd.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <iostream>
using namespace std;

/*
	Assignment 2: Predicting your Future
	COSC 3360 - Fundamentals of Operating Systems
	University of Houston
	Yikchun Ng
	1558087
*/

/* 
	<Commands>
	compile: g++ -o client client.cpp
	run: ./client
*/
void error(string msg)
{
    cerr << msg << endl;
    exit(0);
}

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
	char host[256];	
    char buffer[256];
    char terminator[] = "";

    cout << "Enter server host name: ";
	cin >> host;
    server = gethostbyname(host);
    if (server == NULL) {
    	error("ERROR, no such host");
    }
    
    cout << "Enter server port number: " ;
    cin >> portno;
    if (portno < 1024 || portno > 65535) {
	   error("ERROR, port range: 1024 - 65535\n");
    }
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    
    memset((char *) &serv_addr, 0, sizeof(serv_addr));
   	serv_addr.sin_family = AF_INET;
    memcpy((char *)server->h_addr, 
        (char *)&serv_addr.sin_addr.s_addr,
        server->h_length);
    serv_addr.sin_port = htons(portno);
   	int len;
	connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr));
		
	cout << "Enter a college major: ";
	memset(buffer,0,256);
	cin.ignore();
	cin.getline(buffer, 256);
	if(strcmp(buffer, terminator) == 0){
		exit(0);
	}
	send(sockfd,buffer,sizeof(buffer),0);
	recv(sockfd,buffer,sizeof(buffer), 0);
	cout << buffer << endl;
	close(sockfd);
    
	do{	
	    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    	if (sockfd < 0) 
        	error("ERROR opening socket");
		connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr));
		
		cout << "Enter college major: ";
		memset(buffer,0,256);
		cin.getline(buffer, 256);
		if(strcmp(buffer, terminator) == 0){
			exit(0);
		}
		send(sockfd,buffer,sizeof(buffer),0);
		memset(buffer,0,256);
		n = recv(sockfd,buffer,sizeof(buffer), 0);
		cout << buffer << endl;
		close(sockfd);
	} while(1);
	
    return 0;
}
