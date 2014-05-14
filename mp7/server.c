/** @file server.c */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <queue.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "queue.h"
#include "libhttp.h"
#include "libdictionary.h"

const char *HTTP_404_CONTENT = "<html><head><title>404 Not Found</title></head><body><h1>404 Not Found</h1>The requested resource could not be found but may be available again in the future.<div style=\"color: #eeeeee; font-size: 8pt;\">Actually, it probably won't ever be available unless this is showing up because of a bug in your program. :(</div></html>";
const char *HTTP_501_CONTENT = "<html><head><title>501 Not Implemented</title></head><body><h1>501 Not Implemented</h1>The server either does not recognise the request method, or it lacks the ability to fulfill the request.</body></html>";

const char *HTTP_200_STRING = "OK";
const char *HTTP_404_STRING = "Not Found";
const char *HTTP_501_STRING = "Not Implemented";

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
queue_t fds;
int* csock;
pthread_t workers[1024];

/**
 * Processes the request line of the &HTTP header.
 * 
 * @param request The request line of the &HTTP header.  This should be
 *                the first line of an &HTTP request header and must
 *                NOT include the &HTTP line terminator ("\r\n").
 *
 * @return The filename of the requested document or NULL if the
 *         request is not supported by the server.  If a filename
 *         is returned, the string must be free'd by a call to free().
 */
char* process_http_header_request(const char *request)
{
	// Ensure our request type is correct...
	if (strncmp(request, "GET ", 4) != 0)
		return NULL;

	// Ensure the function was called properly...
	assert( strstr(request, "\r") == NULL );
	assert( strstr(request, "\n") == NULL );

	// Find the length, minus "GET "(4) and " &HTTP/1.1"(9)...
	int len = strlen(request) - 4 - 9;

	// Copy the filename portion to our new string...
	char *filename = malloc(len + 1);
	strncpy(filename, request + 4, len);
	filename[len] = '\0';

	// Prevent a directory attack...
	//  (You don't want someone to go to &http://server:1234/../server.c to view your source code.)
	if (strstr(filename, ".."))
	{
		free(filename);
		return NULL;
	}

	return filename;
}

void* process_connection(void* dat)
{
	long size = 0;
	char* loc = NULL;
	char* filename = NULL;
	char* response_string = NULL;
	char* content_type = NULL;
	char* type = NULL;
	char* response = NULL;
	const char* status;
	int response_code;
	const char* response_code_string;
	FILE* req_file;
	char* curr_char;
	const char* connect_type;
	int header_size;
	int chars_sent;
	int i;
	int* curr;

	/*	TASK 3a  */
    int fd = *((int*)dat);
	pthread_mutex_unlock(&lock);
	http_t http;

	int loop = 1; // keep reading while connect_type is Keep-Alive
	while(loop){
		if(http_read(&http, fd) >= 0) 
		{

			/*	TASK 3b  */
			status = http_get_status(&http);
			printf("Status: %s\n", status);
			filename = process_http_header_request(status);

			/*  TASK 3c  */
			response_code = 0;
			response_code_string = NULL;
			response_string = NULL;
			if(!filename)
			{
				response_code = 501;
				response_code_string = HTTP_501_STRING;
				response_string = (char*)HTTP_501_CONTENT;
				size = strlen(HTTP_501_CONTENT);
			} else {
				if(strcmp(filename, "/") == 0)
				{
					filename = (char*)realloc((void*)filename, 12);
					strcpy(filename, "/index.html");
				}

				loc = malloc(sizeof(char)*(4+strlen(filename)));
				strcpy(loc, "web");
				strcat(loc, filename);
				printf("Opening %s\n", loc);
				req_file = fopen(loc, "rb");
				if(!req_file)
				{
					response_code = 404;
					response_code_string = HTTP_404_STRING;
					response_string = (char*)HTTP_404_CONTENT;
					size = strlen(HTTP_404_CONTENT);
				} else {
					response_code = 200;
					response_code_string = HTTP_200_STRING;
					fseek(req_file , 0L , SEEK_END);
					size = ftell(req_file);
					rewind(req_file);

					response_string = malloc(size+1);
					if(!response_string)
					{
						fclose(req_file);
						close(fd);
						perror("Error allocating memory for buffer\n");
						exit(1);
					}
					//printf("Reading %ld bytes\n", size);
					if(fread((void*)response_string, size, 1, req_file)!=1)
					{
						fclose(req_file);
						close(fd);
						free((char*)response_string);
						perror("Error reading file\n");
						exit(1);
					}
					fclose(req_file);
					//printf("Data from file: %s\n", response_string);
				}
			}

			/*  TASK 3d  */
			printf("Getting content type\n");
			content_type = malloc(128);
			if(response_code == 501 || response_code == 404)
			{
				strcpy(content_type, "text/html");
			} else {
				curr_char = strtok(filename, ".");
				type = malloc(128);
				while((curr_char = strtok(NULL, ".")))
				{
					//printf ("Token: %s\n", curr_char);
					strncpy(type, curr_char, 5);
				}

				if(strcmp(type, "html") == 0)
					strncpy(content_type, "text/html", 11);
				else if(strcmp(type, "css") == 0)
					strncpy(content_type, "text/css", 11);
				else if(strcmp(type, "jpg") == 0)
					strncpy(content_type, "image/jpeg", 11);
				else if(strcmp(type, "png") == 0)
					strncpy(content_type, "image/png", 11);
				else
					strncpy(content_type, "text/plain", 11);
			}

			/*  TASK 3e  */
			printf("Preparing response\n");
			response = malloc(size+512);
			sprintf(response, "HTTP/1.1 %d %s\r\n", response_code, response_code_string);
			sprintf(response + strlen(response), "Content-Type: %s\r\n", content_type);
			sprintf(response + strlen(response), "Content-Length: %ld\r\n", size);

			connect_type = http_get_header(&http, "Connection");
			if(strcmp(connect_type, "Keep-Alive") == 0 || strcmp(connect_type, "keep-alive") == 0)
				sprintf(response + strlen(response), "Connection: %s\r\n", connect_type);
			else
			{
				sprintf(response + strlen(response), "Connection: close\r\n");
				loop = 0;
			}
			sprintf(response + strlen(response), "\r\n");
			header_size = strlen(response);

			memcpy(response + header_size, response_string, size);

			//printf("Response string: \n%s\n", response);

			printf("Sending response\n");
			chars_sent = send(fd, (const void*)response, header_size+size, 0);
			if(chars_sent <= 0)
				perror("No chars sent\n");
			printf("Sent response of %d bytes\n\n", chars_sent);
			free(loc);
			loc=NULL;
			free(filename);
			filename=NULL;
			if(response_code == 200)
				free(response_string);
			response_string = NULL;
			free(content_type);
			content_type = NULL;
			free(type);
			type = NULL;
			free(response);
			response = NULL;
			http_free(&http);
		} else {
			printf("http_read failed\n"); 
			break;
		}
	}

	/*  TASK 3f  */
	printf("Exiting thread\n");
	for (i = 0; i < (int)queue_size(&fds); ++i)
	{
		curr = (int*)queue_at(&fds, i);
		if(*curr == fd)
		{
			curr = (int*)queue_remove_at(&fds, i);
			free(curr);
		}
	}
	close(fd);
	return NULL;
}

void sighandler(int sig_num)
{
	printf("Shutting down server\n");
	int i;
	int* curr;
	while(queue_size(&fds) > 0)
	{
		curr = (int*)queue_dequeue(&fds);
		shutdown(*curr, 0);
		free(curr);
	}
	for (i = 0; i < 1024; ++i)
	{
		if(workers[i])
			pthread_join(workers[i], NULL);
	}
	queue_destroy(&fds);
	free(csock);

	if(sig_num == SIGINT){
		exit(1);
	}
}


int main(int argc, char **argv)
{
	/*	TASK 4  */
	signal(SIGINT, sighandler);
	signal(SIGPIPE, SIG_IGN);

	/*	TASK 1  */
	int port;
	int ssock;
	queue_init(&fds);
	struct sockaddr_in params;
	int count = 0;
	
	if(argc < 2){
		fprintf(stderr, "Usage: ./server <port_num>\n");
	}

	port = strtol(argv[1], NULL, 10);

	ssock = socket(AF_INET, SOCK_STREAM, 0);
	params.sin_family = AF_INET;
	params.sin_port = htons(port);
	params.sin_addr.s_addr = htonl(INADDR_ANY);
	
	if(bind(ssock, (struct sockaddr *)&params, sizeof(struct sockaddr_in)) < 0) {
		perror("Unable to bind\n");
		return 1;
	}
	
	if(listen(ssock, 10) < 0) {
		perror("Unable to listen\n");
		return 1;
	}

	/*	TASK 2  */
	while(1) {
		pthread_mutex_lock(&lock);
		csock = malloc(sizeof(int));
		*csock = accept(ssock, NULL, NULL); //file descriptor
		if(*csock < 0)
		{
			perror("Unable to accept\n");
			return 1;
		}
		queue_enqueue(&fds, (void*)csock);

		printf("Launching thread\n");
		pthread_create(&workers[count], NULL, &process_connection, (void*)csock);
		count++;
		if(count == 1024)
			count = 0;
	}
}
