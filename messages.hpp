#pragma once

#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <regex>
#include <ctype.h>

using namespace std;

class Message{

public:
    string recepient, content;
    int content_length;
    bool valid;
    bool c_length_present;

    Message(){}

    Message(string msg){
        valid = true;
        c_length_present = true;
        string send_title = "SEND";
        string for_title = "FORWARD";
        if (strstr(msg.c_str(), send_title.c_str())){
            parse_msg(msg, 0);
        }
        else if (strstr(msg.c_str(), for_title.c_str())){
            parse_msg(msg, 1);
        }
        else{
            valid = false;
        }
    }

    void parse_msg(string msg, int matched_type){
        string regex_format;
        int init_idx;
        if (matched_type == 0){
            regex_format = "SEND\\s[0-9a-zA-Z]+\nContent-length:\\s\\d+\n\n.*\n*";
            init_idx = 5;
        }
        else{
            regex_format = "FORWARD\\s[0-9a-zA-Z]+\nContent-length:\\s\\d+\n\n.*\n*";
            init_idx = 8;
        }
        regex match_format(regex_format);

        if (!regex_match(msg, match_format)){
            valid = false;
            return;
        }
        bool found_space = false;
        string message_len = "";
        recepient = "";
        content = "";
        int no_new_lines = 0;
        for (int i=init_idx;i<msg.length();i++){
            if (msg[i] == '\n'){
                no_new_lines += 1;
                continue;
            }
			if (no_new_lines == 0)
                recepient += msg[i];
            else if (no_new_lines == 1){
                if (isdigit(msg[i]))
                    message_len += msg[i];
            }
            else{
                content += msg[i];
            }
        }
        if (message_len == "")
            c_length_present = false;
        else
            content_length = stoi(message_len);

        if (content.length() != content_length){
            valid = false;
        }
        
    }

    static bool is_ack_rcv(string msg, string username){
        return (msg == "RECEIVED "+username+"\n\n");
    }

    static int req_send(string username, string message, int sockfd){
        string msg = "SEND " + username + "\nContent-length: " + to_string(message.length()) + "\n\n" + message+"\n";
        char* char_msg = strcpy(new char[ msg.length() + 1], msg.c_str()); 
        return send(sockfd, char_msg, strlen(char_msg), 0);
    }

    static void ack_send(string username, int sockfd){
        string msg = "SEND " + username + "\n\n";
        char* char_msg = strcpy(new char[ msg.length() + 1], msg.c_str()); 
        send(sockfd, char_msg, strlen(char_msg), 0);
    }

    static int req_forward(string username, string message, int sockfd){
        string msg = "FORWARD " + username + "\nContent-length: " + to_string(message.length()) + "\n\n" + message+"\n";
        char* char_msg = strcpy(new char[ msg.length() + 1], msg.c_str()); 
        return send(sockfd, char_msg, strlen(char_msg), 0);
    }

    static void ack_forward(string username, int sockfd){
        string msg = "RECEIVED " + username + "\n\n";
        char* char_msg = strcpy(new char[ msg.length() + 1], msg.c_str()); 
        send(sockfd, char_msg, strlen(char_msg), 0);
    }
};