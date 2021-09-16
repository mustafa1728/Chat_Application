#pragma once

#include <iostream>
#include <cstring>
#include <sys/socket.h>

using namespace std;

class Register{
public:
    static bool is_to_rcv(string msg){
        string to_recieve = "REGISTER TORECV ";
        return strstr(msg.c_str(), to_recieve.c_str());
    }

    static bool is_ack_rcv(string msg, string username){
        return (msg == "REGISTERED TORECV "+username+" \n \n");
    }

    static bool is_to_send(string msg){
        string to_send = "REGISTER TOSEND ";
        return strstr(msg.c_str(), to_send.c_str());
    }

    static bool is_ack_send(string msg, string username){
        return (msg == "REGISTERED TOSEND "+username+" \n \n");
    }

    static void req_send(string username, int sockfd){
        string msg = "REGISTER TOSEND " + username + " \n \n";
        char* char_msg = strcpy(new char[ msg.length() + 1], msg.c_str()); 
        send(sockfd, char_msg, strlen(char_msg), 0);
    }

    static void ack_send(string username, int sockfd){
        string msg = "REGISTERED TOSEND " + username + " \n \n";
        char* char_msg = strcpy(new char[ msg.length() + 1], msg.c_str()); 
        send(sockfd, char_msg, strlen(char_msg), 0);
    }

    static void req_rcv(string username, int sockfd){
        string msg = "REGISTER TORECV " + username + " \n \n";
        char* char_msg = strcpy(new char[ msg.length() + 1], msg.c_str()); 
        send(sockfd, char_msg, strlen(char_msg), 0);
    }

    static void ack_rcv(string username, int sockfd){
        string msg = "REGISTERED TORECV " + username + " \n \n";
        char* char_msg = strcpy(new char[ msg.length() + 1], msg.c_str()); 
        send(sockfd, char_msg, strlen(char_msg), 0);
    }
};
