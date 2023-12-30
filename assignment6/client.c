/*
 * client.c
 */
// Varun Jhaveri
#include <stdio.h>
#include <stdlib.h>
#include "csapp.h"

int main(int argc, char **argv) 
{
    int clientfd;
    char *num1, *num2;
    char *host, *port;
    char *ptr;

    if (argc != 3) {
        fprintf(stderr, "usage: %s <num1> <num2>\n", argv[0]);
        exit(0);
    }

    num1 = argv[1];
    num2 = argv[2];

    host = "localhost";
    port = "8080";

    clientfd = Open_clientfd(host, port);
    rio_t rio; // I/O stream for reading from the server
    char requestBuffer[MAXLINE], headerBuffer[MAXLINE]; // Buffers for request and header
    // Initialize buffers
    memset(requestBuffer, 0, MAXLINE);
    memset(headerBuffer, 0, MAXLINE);
    
    snprintf(requestBuffer, MAXLINE, "<?xml version=\"1.0\"?>\r\n");
    strncat(requestBuffer, "<methodCall>\r\n", MAXLINE - strlen(requestBuffer));
    strncat(requestBuffer, "<methodName>sample.addmultiply</methodName>\r\n", MAXLINE - strlen(requestBuffer));
    strncat(requestBuffer, "<params>\r\n", MAXLINE - strlen(requestBuffer));
    snprintf(requestBuffer + strlen(requestBuffer), MAXLINE - strlen(requestBuffer), "<param><value><double>%s</double></value></param>\r\n", num1);
    snprintf(requestBuffer + strlen(requestBuffer), MAXLINE - strlen(requestBuffer), "<param><value><double>%s</double></value></param>\r\n", num2);
    strncat(requestBuffer, "</params>\r\n", MAXLINE - strlen(requestBuffer));
    strncat(requestBuffer, "</methodCall>\r\n", MAXLINE - strlen(requestBuffer));
    snprintf(headerBuffer, MAXLINE, "POST /RPC2 HTTP/1.1\r\n");
    strncat(headerBuffer, "Content-Type: text/xml\r\n", MAXLINE - strlen(headerBuffer));
    strncat(headerBuffer, "User-Agent: XML-RPC.NET\r\n", MAXLINE - strlen(headerBuffer));
    snprintf(headerBuffer + strlen(headerBuffer), MAXLINE - strlen(headerBuffer), "Content-Length: %ld\r\n", strlen(requestBuffer));
    strncat(headerBuffer, "Expect: 100-continue\r\n", MAXLINE - strlen(headerBuffer));
    strncat(headerBuffer, "Connection: Keep-Alive\r\n", MAXLINE - strlen(headerBuffer));
    strncat(headerBuffer, "Host: localhost:8080\r\n\r\n", MAXLINE - strlen(headerBuffer));


    // Append body to header
    strncat(headerBuffer, requestBuffer, MAXLINE - strlen(headerBuffer));
    // Write the request
    Rio_writen(clientfd, headerBuffer, strlen(headerBuffer));
    // Initialize read buffer
    Rio_readinitb(&rio, clientfd);

    for (; Rio_readlineb(&rio, requestBuffer, MAXLINE) > 0; ) {
        ptr = ExtractDoubleValue(requestBuffer);
        if (ptr != NULL) {
            printf("%s    ", ptr);
        }
    }
    printf("\n");
    Close(clientfd);
    exit(0);
}
