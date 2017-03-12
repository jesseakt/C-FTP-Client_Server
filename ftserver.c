/*****
 * ftserver.c
 * Author:Jesse Thoren
 * Description: This is the server part of a 2-connection client-server
 *    network application designed for a programming assignment for
 *    CS372 at Oregon State University.
 * References:
 *    www.tutorialspoint.com/unix_sockets/socket_server_example.htm
 *    www.linushowtos.org/C_C++/socket.htm
 *    www.thegeekstuff.com/2012/03/catch-signals-sample-c-code
 *    beej.us/guide/bgnet/output/print/bgnet_USLetter_2.pdf
 */

#include <dirent.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#define BUFFER_LEN 1024

int startup(int argc, char *argv[]);
void signal_handler(int signo);
void error(const char *);

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portNo, dataPortNo, n;
    socklen_t clilen;
    struct sockaddr_in cli_addr, serv_addr; //Control socket
    struct sockaddr_in serv_data_addr;
    char buffer[BUFFER_LEN];
    char command[BUFFER_LEN];
    char filename[BUFFER_LEN];
    struct hostent *host;
    char hostname[BUFFER_LEN];

    //Set up signal handler (Referenced thegeekstuff)
    if(signal(SIGINT, signal_handler) == SIG_ERR){
        printf("\nError initializing signal handler\n");
        printf("Shutting down server \n");
        exit(0);
    }

    //Call startup to verify command line entry and get user portNumber
    portNo = startup(argc, argv);

    printf("Server is accpeting new connections on port: %d\n",portNo);
    printf("Send a keyboard interrupt to shut down the server\n");
    
    //Loop until sigint
    while(1){
        
        //Set up socket (tutorialspoint)
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0){
            error("Error opening socket");
        }
        //If socket is open, set socket structure
        bzero((char *) &serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        serv_addr.sin_port = htons(portNo);

        //stackoverflow.com/questions/21515946
        int value = 1;
        if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value)) == -1)
        {
            error("setsockopt");
        }

        if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &value, sizeof(value)) == -1)
        {
            error("setsockopt");
        }

        //Bind host address
        if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0){
            error("Error binding");
        }

        if(listen(sockfd,5)<0){
            printf("\nError listening\n");
            close(sockfd);
            exit(1);
        }
        clilen = sizeof(cli_addr);

        printf("Ready to accept new connection\n");

        //Set up a control connection (referenced tutorialpoint)
        sockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        
        if(sockfd < 0){
            close(sockfd);
            error("Error accepting new connection\n");
        }
        
        //Connection established, accept communication
        bzero(buffer, BUFFER_LEN);
        n = read(sockfd, buffer, BUFFER_LEN-1);
        //Catch errors accepting new messages
        if(n<0)
        {
            close(sockfd);
            error("Error accepting new message\n");
        }

        //First communication is a port number for data communication
        //Cast from string to int
        dataPortNo = atoi(buffer);
        printf("Data port received from client: %d\n", dataPortNo);
        //Respond to client
        if(write(sockfd,"Data port received.", 20)<0)
        {
            close(sockfd);
            error("Error acknowledging client.\n");
            exit(0);
        }

        //Get command and filename (when appropriate) from client
        bzero(buffer, BUFFER_LEN);
        n = read(sockfd, buffer, BUFFER_LEN-1);
        if(n<0)
        {
            close(sockfd);
            error("Error accepting command message\n");
        }
        bzero(command,BUFFER_LEN);
        strcpy(command, buffer);
        //Respond that command was received
        if(write(sockfd,"Command message received.", 26)<0)
        {
            close(sockfd);
            error("Error acknowledging client request.\n");
            exit(0);
        }

        //Handle file request
        if(strcmp(buffer, "-g") == 0){
            //Get filename
            bzero(buffer, BUFFER_LEN);
            n = read(sockfd, buffer, BUFFER_LEN-1);
            if(n < 0)
            {
                close(sockfd);
                error("Error accepting filename\n");
            }
            bzero(filename,BUFFER_LEN);
            strcpy(filename, buffer);
            //Respond that filename was received
            if(write(sockfd, "Filename received.", 19)<0)
            {
                close(sockfd);
                error("Error acknowledging filename. \n");
                exit(0);
            }
        }
        
        //Open data communication socket for info transfer
        newsockfd = socket(AF_INET, SOCK_STREAM,0);
        if(newsockfd < 0)
        {
            close(sockfd);
            error("Error opening data socket\n");
        }
        //Get hostname from client (stackoverflow.com/questions/504810
        gethostname(hostname,1024);
        host = gethostbyname(hostname);
        if(host == NULL){
            error("No host exists\n");
        }


        bzero((char *) &serv_data_addr, sizeof(serv_data_addr));
        serv_data_addr.sin_family = AF_INET;
        bcopy((char *)host->h_addr, (char *)&serv_data_addr.sin_addr.s_addr, host->h_length);
        serv_data_addr.sin_port = htons(dataPortNo);
        if(connect(newsockfd,(struct sockaddr *) &serv_data_addr, sizeof(serv_data_addr))<0){
            close(newsockfd);
            close(sockfd);
            error("Error establishing data connection\n");
        }
        printf("Data connection established\n");

        //Send directory(stackoverflow.com/questions/612097 referenced)
        if(strcmp(command, "-l")==0)
        {
            DIR *dir;
            struct dirent *ent;

            printf("Now sending directory over data connection\n");

            bzero(buffer,BUFFER_LEN);

            //Get working directory
            char cwd[BUFFER_LEN-2];
            getcwd(cwd,sizeof(cwd));

            if((dir = opendir(cwd))!=NULL){
                while((ent = readdir(dir)) !=NULL) 
                {
                    strcat(buffer, ent->d_name); // Dir contents in payload
                    strcat(buffer, "\n"); //Append line feed
                }
                closedir(dir);
                if(write(newsockfd, buffer, strlen(buffer))<0)
                {
                    close(newsockfd);
                    close(sockfd);
                    error("Error writing directory to socket");
                }
            }
        }

        //Send file
        if(strcmp(command, "-g")==0)
        {
            //Open file
            FILE *transfer;
            transfer = fopen(filename,"r");
            
            //Verify file exists
            if(transfer == 0)
            {
                //Inform file doesn't exist
                if(write(newsockfd, "DNE", 4)<0)
                {
                    fclose(transfer);
                    close(newsockfd);
                    close(sockfd);
                    error("Error writing file confirmation to socket");
                }
            }
            else
            {
                if(write(newsockfd, "OK", 3)<0)
                {
                    fclose(transfer);
                    close(newsockfd);
                    close(sockfd);
                    error("Error writing file confirmation to socket");
                }

                //Wait for confirmation from client to go.
                if(read(newsockfd,buffer,BUFFER_LEN) == 0)
                {
                    printf("File transfer cancelled\n");
                    fclose(transfer);
                    close(newsockfd);
                    close(sockfd);
                    continue;
                }

                printf("Sending file to client over data connection \n");
                
                //Loop until file is sent
                while(1)
                {
                    bzero(buffer,BUFFER_LEN);
                    int bytesRead = fread(buffer,1,BUFFER_LEN,transfer);
                    //If we got some bytes, send them.
                    if(bytesRead > 0){
                        if(write(newsockfd,buffer,bytesRead)<0){
                            close(newsockfd);
                            close(sockfd);
                            fclose(transfer);
                            error("Error writing file to socket\n");
                        }
                    }
                    //If we read less than BUFFER_LEN bytes, terminate
                    if(bytesRead < BUFFER_LEN)
                    {
                        if(feof(transfer))
                        {
                            fclose(transfer);
                            break;
                        }
                        if(ferror(transfer))
                        {
                            fclose(transfer);
                            close(newsockfd);
                            close(sockfd);
                            error("Error reading from file to transfer\n");
                        }
                    }
                }
            }
        }

        //Close data connection
        close(newsockfd);
        close(sockfd);
    }
    return 0;
}

/***
 * Function: startup
 * Description: Validates command line entry
 * Returns: Port number as int
 ***/
int startup(int argc, char *argv[])
{

    if(argc < 2)
    {
        fprintf(stderr, "Usage: %s <port> \n", argv[0]);
        exit(0);
    }

    int portno = atoi(argv[1]);
    if(portno < 0 || portno > 65535){
        fprintf(stderr, "Specified port number must be between 0 and 65535\n");
        exit(0);
    }

    return portno;
}

/***
 * Function: signal_handler
 * Description: Catches SIGINT and gracefully shuts down the server
 ***/
void signal_handler(int signo){
    if(SIGINT == signo){
        printf("\nCaptured SIGINT. Gracefully shutting down server\n");
        exit(1);
    }
}

/*** 
 * Function: error
 * Description: Prints error message and exits server with code 0
 ***/
void error(const char *msg){
    perror(msg);
    exit(0);
}
