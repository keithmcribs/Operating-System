#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include <ctype.h>
#include <iostream>
#include <fstream>
#include <sstream>
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
	compile: g++ -o server server.cpp
	run: ./server
*/

void error(string msg)
{
    cerr << msg << endl;
    exit(1);
}

int main()
{
	 ifstream fptr;
	 char* filename = new char[20];
	 cout << "Enter the filename to be read: ";
	 cin >> filename;

     int sockfd, newsockfd, portno, clilen;
     char buffer[256];
     struct sockaddr_in serv_addr, cli_addr;
     int n;
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) 
        error("ERROR opening socket");
     memset((char *) &serv_addr, 0, sizeof(serv_addr));
     
    printf("Enter server port number: ");
    cin >> portno;
    if (portno < 1024 || portno > 65535) {
       cout << "ERROR, port range: 1024 - 65535\n";
       exit(0);
    }
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);
     if (bind(sockfd, (struct sockaddr *) &serv_addr,
            sizeof(serv_addr)) < 0)
            error("ERROR on binding");
     listen(sockfd,10);
     clilen = sizeof(cli_addr);
	 while(1)
	 {
		 newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, (socklen_t *) &clilen);
	     int pid = fork();
	     if(pid < 0){
	     	error("ERROR on fork");
	     }
	     if(pid == 0){
			try {
				fptr.open(filename);
			} catch (ifstream::failure e) {
			 	error("Cannot open file");
			}
	     	close(sockfd);
	     	memset(buffer,0,256);
  			recv(newsockfd,buffer,255,0);
			cout << "Here is the message: " << buffer << endl;
			string line;
			string major;
		    int diff = 0, i = 0, j = 0;
			while(getline(fptr, line)){
				istringstream iss(line);
				string temp;
				int earlySalary, midSalary;		
				for(int i=0; i < line.length(); i++){
					iss >> temp;
					if(isalpha(temp.at(0))||ispunct(temp.at(0))){
						major.append(temp);
						major.append(" ");
					}
					if(isdigit(temp.at(0))){ 
						earlySalary = atoi(temp.c_str());
						iss >> temp;
						midSalary = atoi(temp.c_str());
						break;
					}
				}				
				major.erase(major.size()-1);
				//cout << major << " " << major.size() << " " << earlySalary << " " << midSalary << endl;
				diff = strcmp(major.c_str(), buffer);
		    	if(diff == 0){
		    		sprintf(buffer, "The average early career pay for %s is $%d \nThe corresponding mid-career pay is $%d", major.c_str(), earlySalary, midSalary);
					send(newsockfd,buffer,sizeof(buffer),0);
					cout << buffer << endl;
		    		break;
		    	}
		    	major.clear();
		    }
				  
		    if(diff != 0){
			   	sprintf(buffer, "The major is not in the table");
			   	send(newsockfd,buffer,sizeof(buffer),0);
			   	cout << "Input not found" << endl;
			}
			break;
		} else {
			close(newsockfd);  
			fptr.close();
		}
	}
	
     return 0; 
}

