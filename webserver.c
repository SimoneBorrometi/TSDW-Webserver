#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <unistd.h>

#define BUFF_LEN 4096



int parse_http_request(char *buffer, int bufflen){
	//parse the request line with strtok
	char *token; 
	token = strtok (buffer, " ");		//read the method
	printf("Method: %s\n", token);
	token = strtok( NULL , " ");		//read the URI
	printf("URI: %s\n",token);
	token = strtok( NULL , "\r\n");		//read the version, here we read until the new line
	printf("Version: %s\n",token);

	//parse the headers

	token = strtok (NULL, "\r\n");
	while(token != NULL){
		printf("Header: %s\n",token);
		token = strtok (NULL, "\r\n");
	}
	
	return 1;
}


//BUFFER
//		A B C \r \n \r \n

int find_empty_line( char *buffer, int bufflen){
	for( int i=0; i < bufflen - 3; i++){
		if( buffer[i] == '\r' && buffer[i+3] == '\n' ){
//			printf("debug\n");
			return 1;					//found the end of the header
		}
	}
	return 0;
}


int main(){
	//Create TCP socket
	
	int server_socket_fd = socket(PF_INET,SOCK_STREAM,0);
	if(server_socket_fd == -1){
		perror("Error on socket creation\n");
		exit(1);
	}
	//Set socket to reuse its address
	int sock_opt_enable = 1;
	if(setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &sock_opt_enable ,sizeof(int)) == -1 ){
		perror("Error on setsockopt");
		exit(1);
	}

	//Create socket address
	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(8080);	
	server_addr.sin_addr.s_addr = INADDR_ANY; 		//bind to all available interfaces

	//Bind socket to the address
	if(bind(server_socket_fd, (struct sockaddr * ) &server_addr, sizeof(server_addr)) == -1){
		perror("Error in socket binding");
		exit(1);
	}

	//Listen for connections
	if(listen(server_socket_fd, 1) == -1 ){
		perror("Error in lisening");
		exit(1);
	}

	printf("Server ready\n");


	//accept connections
	int client_socket_fd;
	struct sockaddr_in client_addr;
	int client_addr_len = sizeof(client_addr);

	char buffer[BUFF_LEN];
	char out_buffer[BUFF_LEN];
	

	while((client_socket_fd = accept( server_socket_fd, 
					(struct sockaddr *) &client_addr,
					&client_addr_len))
				!= -1){
		printf("Client connected!\n");
		
		int header_received = 0;
		int read_value = 0;
		int total_bytes_read = 0;
//		char debug_output [BUFF_LEN];
		
		
		//read from client until there is an error or we read the HTTP request header
		while(!header_received && 
				(read_value = read(client_socket_fd, 
						buffer + total_bytes_read, 
						BUFF_LEN - total_bytes_read))
				> 0) {
			total_bytes_read += read_value;
			
//			snprintf(debug_output, total_bytes_read+1,"%s",buffer);		//copy the first total_bytes_read bytes to the debug_output string to be printed
//			printf("Read %d bytes:\n%s",total_bytes_read,debug_output);

			header_received = find_empty_line(buffer, total_bytes_read);
		}
		buffer[total_bytes_read] = '\0';
		printf("Received an http request\n");
		parse_http_request(buffer,total_bytes_read);
		
		//placeholder response
		FILE *index = fopen("/home/simoneb/index.html", "r");		//html file

		//get file size
		fseek(index,0L, SEEK_END);
		int f_size = ftell(index);
		rewind (index);

		//write the response status line and header
		int printed = sprintf(out_buffer,
				"HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n",f_size);

		//copy the index onto the buffer
		fread(out_buffer+printed,sizeof(char), BUFF_LEN,index);
		out_buffer[printed+f_size] = '\0';

		printf("To write:\n%s",out_buffer);
		int out_buff_len = strlen(out_buffer);
		int bytes_written=0, total_bytes_written=0;
		while((bytes_written = write(client_socket_fd,
						out_buffer+total_bytes_written,
						out_buff_len-total_bytes_written)) 
				>0 ){
			total_bytes_written += bytes_written;
		}


		//End of connection 
		close(client_socket_fd);		
	}

	//Error on accept
	
	perror("Error on accept");
	close(server_socket_fd);
	exit(-1);


}
