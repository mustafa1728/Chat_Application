#include "constants.h"
#include "errors.hpp"
#include "messages.hpp"
#include "register.hpp"

#include <iostream>
#include <ostream>
#include <vector>
#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>
#include <regex>


using namespace std;

string username;


void *send_msg_handler(void* client_send_socket) {

    int socket_id = *(int*)client_send_socket;
    char buffer[BUFFER_SZ];

    while (1) {
        char usr_input[200] = {};
        char buffer[BUFFER_SZ];
        cout<<'\r'<<username<<"=> ";
		fflush(stdout);
        fgets(usr_input, 200, stdin);
        string line(usr_input);
        if( line == "" || line == " " || line == "\n")
            continue;
        regex msg_format("@.+\\s.+\n*");
        if (!regex_match(line, msg_format)){
			cout<<"Invalid line. Please write the message in format: @[recepient] [msg]\n\n";
			continue;
        }
        bool found_space = false;
        string recepient = "", message = "";
        for (int i=1;i<line.length();i++){
			if (line[i] == '\n'){
				continue;
			}
			if (!found_space && line[i] == ' '){
				found_space = true;
				continue;
			}
			if (!found_space){
				recepient += line[i];
			}
			else{
				message += line[i];
			}
        }
		Message::req_send(recepient, message, socket_id);
        bzero(buffer,BUFFER_SZ);
        recv( socket_id , buffer , BUFFER_SZ , 0 );
        string rv(buffer);
		if (Error::is_fatal_error(rv)){
			cout<<"Fatal error: ";
			cout<<rv;
			cout<<"Closing the socket. Please reconnect!\n\n";
			close(socket_id);
			exit(0);
		}
		else if (Error::is_error(rv)){
			cout<<rv;
		}
        else{
			cout<<"Message delivered successfully\n\n";
		}
    }

	close(socket_id);
    pthread_exit(client_send_socket);

}

void *recv_msg_handler(void* client_receive_socket ){

	int socket_id = *(int*)client_receive_socket;
	char buffer[BUFFER_SZ] ;
    while (1) {
		bzero(buffer, BUFFER_SZ);
		int receive = recv(socket_id, buffer , BUFFER_SZ , 0);
		string rv(buffer);
		// cout<<rv;

		Message message(rv);
		if (!message.c_length_present){
            Error::send_msg(103, socket_id);
            break;
        }
        if (!message.valid){
            Error::send_msg(103, socket_id);
            break;
        }

		// string separator = "\n==========================================================================\n";
		string separator = "\n";
		cout<<separator;
		cout<<"New message from "<<message.recepient<<" | "<<message.content;
		cout<<separator<<'\n';
		
		cout<<username<<"=> ";
		fflush(stdout);

		Message::ack_forward(message.recepient, socket_id);
    }

	close(socket_id);
    pthread_exit(client_receive_socket);

}


int main(){

	int port = PORT;
	int client_receive_socket , client_send_socket;

	struct sockaddr_in server_address ;
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = INADDR_ANY;
	server_address.sin_port = htons(port);

	char buffer[BUFFER_SZ];

	while(1){

			cout<<"Please Enter your username (alphanumeric only)\n\n";
			char usr_input[200] = {};
			fgets(usr_input, 200, stdin);
			string raw_username(usr_input);
			username = "";
			for (int i=0;i<raw_username.length();i++){
				if (raw_username[i] != '\n')
					username += raw_username[i];
			}
			if( username == "" || username == " " || username == "\n")
				continue;



			client_receive_socket = socket(AF_INET,SOCK_STREAM,0);
			int receive_socket_connection_status = connect( client_receive_socket , (struct sockaddr *)&server_address, sizeof(server_address));
			if ( receive_socket_connection_status    == -1) {
				cout<<"ERROR: receive socket not created properly!\n";
				return EXIT_FAILURE;
			}
			Register::req_rcv(username, client_receive_socket);
			bzero(buffer, BUFFER_SZ);
			recv( client_receive_socket , buffer , BUFFER_SZ, 0 );
			string rv(buffer);
			cout<<rv;
			if(!Register::is_ack_rcv(rv, username))
				continue;



			client_send_socket = socket(AF_INET,SOCK_STREAM,0);
			int send_socket_connection_status = connect( client_send_socket , (struct sockaddr *)&server_address, sizeof(server_address));	
			if ( send_socket_connection_status    == -1) {
				printf("ERROR: send socket not created properly!\n");
				return EXIT_FAILURE;
			}
			Register::req_send(username, client_send_socket);
			bzero(buffer, BUFFER_SZ);
			recv( client_send_socket , buffer , BUFFER_SZ, 0 );
			string rv1(buffer);
			cout<<rv1;
			if(!Register::is_ack_send(rv1, username))
				continue;

			break;
	}

	pthread_t send_msg_thread;
	if(pthread_create(&send_msg_thread, NULL,    &send_msg_handler, &client_send_socket ) != 0){
		cout<<"ERROR: pthread\n";
		return EXIT_FAILURE;
	}

	pthread_t recv_msg_thread;
	if(pthread_create(&recv_msg_thread, NULL,    &recv_msg_handler, &client_receive_socket ) != 0){
		cout<<"ERROR: pthread\n";
		return EXIT_FAILURE;
	}

	pthread_join(send_msg_thread, NULL);
	pthread_join(recv_msg_thread, NULL);

    return 0;

}