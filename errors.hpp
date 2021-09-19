#pragma once

#include <iostream>
#include <cstring>
#include <sys/socket.h>

using namespace std;

// Unified class to manage different errors
// Only need to specify the error code, the function sends corresponding error message over the socket.
class Error{
public:
    static char* get(int status){
        string msg;
        switch(status){
            case 100:
                msg = "ERROR 100 Malformed username\n\n";
                break;
            case 101:
                msg = "ERROR 101 No user registered\n\n";
                break;
            case 102:
                msg = "ERROR 102 Unable to send\n\n";
                break;
            case 103:
                msg = "ERROR 103 Header incomplete\n\n";
                break;
            default:
                msg = "ERROR: An inknown error ocurred\n\n";
                break;
        }
        char* char_msg = strcpy(new char[ msg.length() + 1], msg.c_str());
        return char_msg;
    }
    static void display(int status){
        cout<<Error::get(status);
    }

    static void send_msg(int status, int sockfd){
        char* msg = Error::get(status); 
        send(sockfd, msg, strlen(msg), 0);
    }

    static bool is_fatal_error(string msg){
        // for parsing received error message
        string error_103 = "ERROR 103";
        if (strstr(msg.c_str(), error_103.c_str())){
            return true;
        }
        return false;
    }
    static bool is_error(string msg){
        // for parsing received error message
        string error = "ERROR";
        if (strstr(msg.c_str(), error.c_str())){
            return true;
        }
        return false;
    }
    
};