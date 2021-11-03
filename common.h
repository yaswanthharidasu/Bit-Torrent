#ifndef __COMMON_H_
#define __COMMON_H_

#include<iostream>
#include<sstream>
#include<fstream>

#include<vector>
#include<unordered_map>
#include<algorithm>
#include<string>
#include<thread>
#include<string.h>
#include<pthread.h>

#include<sys/socket.h>
#include<arpa/inet.h>  // struct sockaddr_in

#include"utils.h"
#include"color.h"
#include"log.h"

using namespace std;

// Macros for the commands
#define CREATE_USER "create_user"
#define CREATE_GROUP "create_group"
#define LOGIN "login"
#define LOGOUT "logout"
#define LIST_USERS "list_users"
#define LIST_GROUPS "list_groups"
#define LIST_REQUESTS "list_requests"
#define JOIN_GROUP "join_group"
#define LEAVE_GROUP "leave_group"
#define ACCEPT_REQUEST "accept_request"

#define USER_REGISTER_SUCCESS KGRN "Registration completed successfully" RESET
#define GROUP_REGISTER_SUCCESS KGRN "Created group successfully" RESET
#define GROUP_JOIN_SUCCESS KGRN "Sent the join request, wait for the admin's approval" RESET
#define LOGIN_SUCCESS KGRN "Logged in successfully" RESET

#endif