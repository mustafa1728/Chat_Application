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
#include <map>
#include <regex>

using namespace std;


map<string, pair<int, int> > clients; 
bool no_user_registered(){
    bool any_registered = false;
    map<string,pair<int,int> > ::iterator it;
    for( it = clients.begin() ; it !=clients.end() ;it++){
        pair<int,int> temp =  it->second;
        if (temp.first != 0 || temp.second != 0)
            any_registered = true;
    }
    return !any_registered;
}

// Message senfing API. supports unicast as well as broadcast (when to_user == all)
int send_message(string message, string from_user, string to_user ){
    
    char buffer[BUFFER_SZ];

    if( to_user == "ALL"){

        map<string,pair<int,int> > ::iterator it;
        bool unable = false;

        for( it = clients.begin() ; it !=clients.end() ;it++){
            if( it->first == from_user )
                continue;
            pair<int,int> temp =  it->second;
            if (temp.first == 0 && temp.second == 0)
                continue;

            int send_status = Message::req_forward(from_user, message, temp.second);

            if( send_status < 0)
                return 0;

            bzero(buffer, BUFFER_SZ);
            recv(temp.second, buffer , BUFFER_SZ , 0 );
            string m(buffer);
            cout<<m<<'\n';

            if(!Message::is_ack_rcv(m, from_user))
                return 0;
        }
    }
    else{
        bool client_exists = false;
        pair<int,int> temp;
        for (auto it = clients.cbegin(); it != clients.cend(); ++it) {
            if ((*it).first == to_user){
                client_exists = true;
                temp = (*it).second;
            }
        }
        if (!client_exists)
            return 0;

        int send_status = Message::req_forward(from_user, message, temp.second);
        if( send_status < 0)
            return 0;

        bzero(buffer, BUFFER_SZ);
        recv(temp.second, buffer , BUFFER_SZ , 0 );
        string m(buffer);
        if (Error::is_fatal_error(m)){
			cout<<"Fatal error: ";
			cout<<m;
			cout<<"Closing the client socket. Please reconnect!\n\n";
			close(temp.second);
			return 0;
		}
        else
            cout<<m<<'\n';

        if(!Message::is_ack_rcv(m, from_user))
            return 0;
    }

    return 1;
}

// waits for any message from the user. Then on the basis of the type of message, handles it
void* manage_client(void* socket){

    char buff_out[BUFFER_SZ];
    int socket_id = *(int*)socket;
    string username;
    while (1){
        
        bzero(buff_out, BUFFER_SZ);
        int receive = recv(socket_id, buff_out, BUFFER_SZ, 0);
        string recieved_msg(buff_out);
        regex alphanum("[0-9a-zA-Z]+");

        // If this thread got a TORECV message

        if(Register::is_to_rcv(recieved_msg)){
            cout<<recieved_msg;
            int end = recieved_msg.length();
            username = recieved_msg.substr(16,end-20);

            if (!regex_match(username, alphanum)){
                Error::send_msg(100, socket_id);
                close(socket_id);
                break;
            }

            if( clients.find(username) == clients.end() ){
                pair<int,int> p(-1,socket_id);
                clients[username] = p;
            }else{
                pair<int,int> p = clients[username];
                p.second = socket_id;
                clients[username] = p;
            }

            Register::ack_rcv(username, socket_id);
            break;

        }

        // If this thread got a TOSEND message

        else if(Register::is_to_send(recieved_msg)){
            cout<<recieved_msg;
            int end = recieved_msg.length();
            username = recieved_msg.substr(16,end-20);

            if (!regex_match(username, alphanum)){
                Error::send_msg(100, socket_id);
                close(socket_id);
                break;
            }

            if( clients.find(username) == clients.end() ){
                pair<int,int> p(socket_id , -1);
                clients[username] = p;
            }else{
                pair<int,int> p = clients[username];
                p.first = socket_id;
                clients[username] = p;
            }

            Register::ack_send(username, socket_id);
        }

        // If no users registered, but some other message received

        else if (no_user_registered()){
            Error::send_msg(101, socket_id);
            continue;
        }

        // If a normal message received

        else{
            cout<<recieved_msg<<'\n';
            Message message(recieved_msg);

            if( clients[message.recepient].second == -1 ){
                Error::send_msg(101, socket_id);
                continue;
            }
            if (!message.c_length_present){
                Error::send_msg(103, socket_id);
                close(socket_id);
                close(clients[username].second);
                clients.erase(username);
                break;
            }
            if (!message.valid){
                Error::send_msg(103, socket_id);
                close(socket_id);
                close(clients[username].second);
                clients.erase(username);
                break;
            }

            int send_status = send_message(message.content, username, message.recepient);
            if(send_status){
                Message::ack_send(message.recepient, socket_id);
            }
            else{
                Error::send_msg(102, socket_id);
                continue;
            }
        }
    }

    pthread_exit(socket);
}




int main(){

    // create a socket 
    int server_socket = socket(AF_INET,SOCK_STREAM,0);
    int port = PORT;

    // pecify an address for the socket 
    struct sockaddr_in server_address , client_address, client_snd_address;
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(port);

    int socket_id = bind( server_socket , (struct sockaddr *)&server_address, sizeof(server_address));
    if( socket_id == -1 ){
        cout<<"There was an error making connection to the server socket\n\n";
        return EXIT_FAILURE;
    }
    if( listen(server_socket , 10 ) < 0 ){
        cout<<"ERROR, in server socket listen\n \n";
        return EXIT_FAILURE;
    }

    cout<<"\nServer running successfully. Clients may connect!\n";
    cout<<"\nMessages received by server: \n\n";


    pthread_t t_id;

    while(1){

        socklen_t cli_len = sizeof(client_address);
        int *client_socket_id = (int *)malloc(sizeof(int));
		*client_socket_id = accept(server_socket , (struct sockaddr*)&client_address, &cli_len);
        pthread_create(&t_id, NULL, &manage_client, (void*)client_socket_id);
    }
    return 0;
}