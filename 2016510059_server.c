#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include "json.h"
#include "json.c"

#define MAX_GROUP 10   // MAXIMUM 10 GROUPS
#define MAX_USER 10    // MAXIMUM 10 USERS IN A GROUP
#define MAX_BUFFER 100 // BUFFER SIZE FOR MESSAGES

// USER STRUCT
struct UserStruct
{
    int socketNo;
    char userName[MAX_BUFFER];
    char groupName[MAX_BUFFER];
    int groupNo;
    int queueNo;
};
typedef struct UserStruct UserStruct;
// GROUP STRUCT
struct GroupStruct
{
    UserStruct users[MAX_USER];
    char groupName[MAX_BUFFER];
    char password[MAX_BUFFER];
    int private;
    int num_users;
};
typedef struct GroupStruct GroupStruct;
// GLOBAL VARIABLES
GroupStruct groups[MAX_GROUP];
int num_groups = 0;
// FUNCTION FOR RESETTING THE STATE OF A GROUP
void reset_groups(int i)
{
    groups[i].num_users = 0;
    groups[i].private = -1;
    strcpy(groups[i].groupName, "");
    strcpy(groups[i].password, "");
    for (int j = 0; j < MAX_GROUP; j++)
    {
        strcpy(groups[i].users[j].userName, "");
        strcpy(groups[i].users[j].groupName, "");
        groups[i].users[j].socketNo = -1;
        groups[i].users[j].queueNo = -1;
    }
}

// CONNECTION HANDLER
void *handle_connection(void *socket_desc)
{
    int sock = *((int *)socket_desc);
    char temp[MAX_BUFFER];
    char temp2[MAX_BUFFER];
    char temp3[MAX_BUFFER];
    char usernameCheck[MAX_BUFFER];
    UserStruct user;
    user.socketNo = sock;
    user.groupNo = -1;
    // Retrieve user name (phone number)
    char message[200] = "Please enter your phone number: ";
    send(sock, message, strlen(message), 0);
    recv(sock, usernameCheck, MAX_BUFFER, 0);

    // Check for phone number validation
    while (strlen(usernameCheck) != 10)
    {
        char message[200] = "Please enter a valid phone number with 10 digits: ";
        send(sock, message, strlen(message), 0);
        recv(sock, usernameCheck, MAX_BUFFER, 0);
    }

    strcpy(user.userName, usernameCheck);
    strcpy(temp, user.userName);
    // Send information to client and store in a variable
    sprintf(temp2, "-username %s", temp);
    send(sock, temp2, MAX_BUFFER, 0);

    // While loop to handle transmissions
    while (sock != -1)
    {
        char msg[MAX_BUFFER] = "";
        char temp[MAX_BUFFER] = "";
        char *temp1;
        recv(sock, msg, MAX_BUFFER, 0);
        puts(msg);
        strcpy(temp, msg);
        char *token = (char *)malloc(MAX_BUFFER * sizeof(char));
        char *phoneNum = (char *)malloc(MAX_BUFFER * sizeof(char));
        char *groupName = (char *)malloc(MAX_BUFFER * sizeof(char));

        // -whoami COMMAND
        if (strcmp(msg, "-whoami") == 0)
        {
            send(sock, user.userName, strlen(user.userName), 0);
        }
        // -gcreate COMMAND (e.g. 5532487153+oda)
        token = strtok(temp, " ");
        if ((strcmp(token, "-gcreate") == 0 && strlen(msg) > 20))
        {
            if (user.groupNo == -1)
            {
                // If the user does not already have a group move on
                temp1 = strstr(msg, " ");
                temp1 = temp1 + 1;
                strcpy(groupName, temp1);
                phoneNum = strtok(temp1, "+");
                if (strcmp(phoneNum, user.userName) == 0)
                {
                    // Checking for maximum amount of groups (10)
                    if (num_groups < MAX_GROUP)
                    {
                        //checking for group name uniqueness
                        int is_exist = 0;
                        for (int i = 0; i < MAX_GROUP; i++)
                        {
                            if (groups[i].num_users != 0 && strcmp(groupName, groups[i].groupName) == 0)
                            {
                                is_exist = 1;
                                break;
                            }
                        }
                        // group name is not unique inform client
                        if (is_exist == 1)
                        {
                            char *msg = "This group name already exists...\n";
                            send(user.socketNo, msg, strlen(msg), 0);
                        }
                        else
                        {
                            for (int i = 0; i < MAX_GROUP; i++)
                            {
                                //group name is unique, then find index
                                if (groups[i].num_users == 0)
                                {
                                    // variables are created
                                    groups[i].num_users++;
                                    strcpy(user.groupName, groupName);
                                    strcpy(groups[i].groupName, groupName);
                                    user.groupNo = i;
                                    user.queueNo = 0;
                                    groups[i].users[0] = user;
                                    // all groups are private so password is needed
                                    char message[MAX_BUFFER];
                                    strcpy(message, "Please enter the group's password: ");
                                    send(user.socketNo, message, strlen(message), 0);
                                    recv(user.socketNo, groups[i].password, MAX_BUFFER, 0);
                                    groups[i].private = 0;

                                    char *msg = "Group is created.\n";
                                    send(user.socketNo, msg, strlen(msg), 0);
                                    num_groups++;
                                    char temp[MAX_BUFFER];
                                    // send groupname to the client to store it in a variable
                                    sprintf(temp, "-groupname %s", groupName);
                                    send(user.socketNo, temp, MAX_BUFFER, 0);
                                    break;
                                }
                            }
                        }
                    }
                    else
                    { // exceed 10 groups
                        char *msg = "Cannot create another group since there are already 10 groups...\n";
                        send(user.socketNo, msg, strlen(msg), 0);
                    }
                }
                else
                {
                    strcpy(message, "Please enter your phone number correctly!\n");
                    send(sock, message, strlen(message), 0);
                }
            }
            else
            {
                // A user cannot create another group while owning a group already
                strcpy(message, "You cannot create another group while you are already owning one.\n");
                send(sock, message, strlen(message), 0);
            }
        } // no group name is given
        else if ((strcmp(token, "-gcreate") == 0))
        {
            strcpy(message, "Please enter a valid group name.\n");
            send(sock, message, strlen(message), 0);
        }
        // -join COMMAND
        else if (strcmp(token, "-join") == 0)
        {
            if (strlen(msg) > 5)
            {
                // join the group
                if (user.groupNo == -1)
                {
                    temp1 = strstr(msg, " ");
                    temp1 = temp1 + 1;

                    int exists = -1;
                    int entry_status = -1;
                    for (int i = 0; i < MAX_GROUP; i++)
                    {
                        // chechking if the group exists
                        if (strstr(groups[i].groupName, temp1) != NULL)
                        {
                            exists = 1;
                            //checking the found room's availability, capacity is 10
                            if (groups[i].num_users < MAX_USER)
                            {
                                //the room is found and is available for joining
                                if (groups[i].private == 0)
                                {
                                    char message[MAX_BUFFER];
                                    char temp[MAX_BUFFER];
                                    strcpy(message, "This group is password protected. Please enter the password:");
                                    send(user.socketNo, message, strlen(message), 0);
                                    recv(user.socketNo, temp, MAX_BUFFER, 0);
                                    if (strcmp(groups[i].password, temp) == 0)
                                    {
                                        entry_status = 0;
                                    }
                                    else
                                    {
                                        strcpy(message, "Password is incorrect.");
                                        send(user.socketNo, message, strlen(message), 0);
                                    }
                                }
                                if (entry_status == 0)
                                {
                                    // find index to put the user into
                                    // if socketno is -1 then the index is free
                                    for (int j = 0; j < MAX_GROUP; j++)
                                    {
                                        if (groups[i].users[j].socketNo == -1)
                                        {
                                            user.groupNo = i;
                                            user.queueNo = j;
                                            strcpy(user.groupName, groups[i].groupName);
                                            groups[i].num_users++;
                                            groups[i].users[j] = user;
                                            char message[MAX_BUFFER];
                                            strcpy(message, "You entered the group.");
                                            send(user.socketNo, message, strlen(message), 0);
                                            char temp[MAX_BUFFER];
                                            // send the groupname for the client
                                            sprintf(temp, "-groupname+%s", temp1);
                                            send(sock, temp, MAX_BUFFER, 0);
                                            break;
                                        }
                                    }
                                }
                            }
                            else
                            {
                                // the group is full
                                char *msg = "The group is full...\n";
                                send(user.socketNo, msg, strlen(msg), 0);
                                break;
                            }
                        }
                    }
                    if (exists == -1)
                    {
                        // no room with that name
                        char *msg = "There are no rooms with that name...\n";
                        send(user.socketNo, msg, strlen(msg), 0);
                    }
                }
                else
                {
                    // cannot join if you are already in a group
                    strcpy(message, "You are already in a group. Exit first.\n");
                    send(sock, message, strlen(message), 0);
                }
            }
            else
            {
                // No group name is given
                strcpy(message, "Please enter a valid group name...\n");
                send(sock, message, strlen(message), 0);
            }
        }
        // -send COMMAND
        else if (strcmp(token, "-send") == 0)
        {
            if (user.groupNo > -1)
            {
                // message is taken in the form of json
                if (strlen(msg) > 5)
                {
                    temp1 = strstr(msg, " ");
                    temp1 = temp1 + 1;
                    char string[MAX_BUFFER];
                    int gno;
                    // parse with json
                    puts(temp1);
                    JSONObject *json = parseJSON(temp1);
                    char msg[200];
                    char selfmsg[200];
                    strcpy(msg, json->pairs[0].value->stringValue);
                    strcat(msg, " : ");
                    strcat(msg, json->pairs[2].value->stringValue); // get the message body
                    for (int i = 0; i < MAX_USER; i++)
                    {
                        // find group's index
                        if (strstr(user.groupName, json->pairs[1].value->stringValue) != NULL)
                        {
                            gno = user.groupNo;
                        }
                        // send the message to that room parsed from json
                        if (groups[gno].users[i].socketNo != -1)
                        {
                            // if the client is themselves, show them You instead of their numbers.
                            if (strcmp(groups[gno].users[i].userName, json->pairs[0].value->stringValue) == 0)
                            {
                                strcpy(selfmsg, "You");
                                strcat(selfmsg, " : ");
                                strcat(selfmsg, json->pairs[2].value->stringValue);
                                send(groups[gno].users[i].socketNo, selfmsg, strlen(selfmsg), 0);
                            }
                            else
                            {
                                send(groups[gno].users[i].socketNo, msg, strlen(msg), 0);
                            }
                        }
                    }
                }
                else
                {
                    strcpy(message, "Please send a valid message.\n");
                    send(sock, message, strlen(message), 0);
                }
            }
            else
            {
                // user cannot send messages when not in a group
                strcpy(message, "Please join any group to be able to send messages.\n");
                send(sock, message, strlen(message), 0);
            }
        }
        // -exit COMMAND
        else if (strcmp(token, "-exit") == 0)
        {
            if (strlen(msg) <= 5)
            {
                // if the user is not in a room the exit function terminates the program
                if (user.groupNo < 0)
                {
                    sock = -1;
                    strcpy(message, "Program is terminated with the command -exit. Exiting...\n");
                    free(socket_desc);
                    send(user.socketNo, message, strlen(message), 0);
                                }
                else
                {
                    // if they are in a room and they use the command -exit without group name it is forbidden
                    strcpy(message, "You cannot terminate the program within a room. Exit the room first.\n");
                    send(sock, message, strlen(message), 0);
                }
            }
            // the -exit command with the group name parameter
            else if (strlen(msg) > 5)
            {
                token = strstr(msg, " ");
                token = token + 1;
                if (user.groupNo == -1)
                {
                    // if the user is not in any of the groups
                    strcpy(message, "You are not in any of the groups. Join first.\n");
                    send(sock, message, strlen(message), 0);
                }
                else if (strstr(user.groupName, token) != NULL)
                {
                    //if the command is fully correct
                    //delete the user from the group
                    UserStruct temp;
                    groups[user.groupNo].num_users--;
                    //if there are no users left in the group delete it
                    if (groups[user.groupNo].num_users == 0)
                    {
                        reset_groups(user.groupNo);
                        num_groups--;
                    }
                    else
                    {
                        groups[user.groupNo].users[user.queueNo] = temp;
                        groups[user.groupNo].users[user.queueNo].socketNo = -1;
                    }
                    user.groupNo = -1;
                    user.queueNo = -1;
                    strcpy(user.groupName, "");
                    strcpy(message, "You have left the group.\n");
                    send(sock, message, strlen(message), 0);
                    char temp4[MAX_BUFFER];
                    // the client's current room is sent to the client as none
                    sprintf(temp4, "-groupname+none");
                    send(sock, temp4, MAX_BUFFER, 0);
                }
                else
                {
                    strcpy(message, "There are no such groups with that name.\n");
                    send(sock, message, strlen(message), 0);
                }
            }
            else
            {
                strcpy(message, "Please enter a group name.\n");
                send(sock, message, strlen(message), 0);
            }
        }
    }
    return 0;
}

int main(int argc, char *argv[])
{
    for (int i = 0; i < MAX_GROUP; i++)
    {
        reset_groups(i);
    }
    int socket_desc, new_socket, c, *new_sock;
    struct sockaddr_in server, client;
    char *message;

    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1)
    {
        puts("Error while creating the socket.");
        return 1;
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(3205);

    if (bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        puts("Binding failed. Please change the port.");
        return 1;
    }
    else
    {
        puts("********* DEU Signal Messaging App *********\nWaiting for clients........");
    }

    listen(socket_desc, MAX_BUFFER);
    c = sizeof(struct sockaddr_in);
    while ((new_socket = accept(socket_desc, (struct sockaddr *)&client, (socklen_t *)&c)))
    {
        int socket_des = new_socket;
        puts("Client has connected.");
        message = "Welcome to the DEU Signal!!!!!!\n";
        write(new_socket, message, strlen(message));
        pthread_t sniffer_thread;
        new_sock = malloc(1);
        *new_sock = new_socket;
        if (pthread_create(&sniffer_thread, NULL, handle_connection,
                           (void *)new_sock) < 0)
        {
            puts("Error while creating the thread.");
            return 1;
        }
    }
    return 0;
}
