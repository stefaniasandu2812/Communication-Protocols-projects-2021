#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"
#include "parson.h"

int main(int argc, char *argv[])
{
    /* buffers for messages and responses */
    char *message;
    char *response;

    /* socket for connection */
    int sockfd;

    /* keep track of cookies */
    char *cookie = NULL;

    /* keep track of jwt */
    char *jwt = NULL;


    /* var used for getting commands from input */
    char *command = (char *) malloc(LEN * sizeof(char));
    DIE(command < 0, "error memory for command");

    while(1) {
        /* read command from stdin */
        memset(command, 0, LEN);
        fgets(command, LEN, stdin);
        strtok(command, "\n");

        /* exit */
        if (strncmp(command, "exit", 4) == 0) {
            break;
        }

        /* register */ 
        else if (strcmp(command, "register") == 0) {
            
            /* provide prompt for register */
            printf("username:");
            char *reg_user = (char *) calloc(LEN, sizeof(char));
            DIE(reg_user < 0, "error memory for reg_user");
            fgets(reg_user, LEN, stdin);

            printf("password:");
            char *reg_pass = (char *)calloc(LEN, sizeof(char));
            DIE(reg_pass < 0, "error memory for reg_pass");
            fgets(reg_pass, LEN, stdin);

            /* get register info without string terminator */
            strtok(reg_user, "\n");
            strtok(reg_pass, "\n");

            /* newline for delimitation */
            printf("\n");

            /* parsing the message to be sent */
            JSON_Value *root_value = json_value_init_object();
            JSON_Object *root_object = json_value_get_object(root_value);
            char *serialized_string = NULL;
            json_object_set_string(root_object, "username", reg_user);
            json_object_set_string(root_object, "password", reg_pass);
            serialized_string = json_serialize_to_string_pretty(root_value);

            message = compute_post_request("34.118.48.238", "/api/v1/tema/auth/register",
                        NULL, "application/json", serialized_string, NULL);

            /* connecting to server, send message and receive response */
            sockfd = open_connection("34.118.48.238", 8080, AF_INET, SOCK_STREAM, 0);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            puts(response);
            printf("\n");

            /* end connection */
            close_connection(sockfd);

            /* free the used buffers */
            json_free_serialized_string(serialized_string);
            json_value_free(root_value);
            free(reg_user);
            free(reg_pass);
        }

        /* login */
        else if (strcmp(command, "login") == 0) {
            /* if the client tries to login again */
            if (cookie != NULL) {
                printf("Already logged in!\n");
            } else {
            
                /* provide prompt for login */
                printf("username:");
                char *log_user = (char *) calloc(LEN, sizeof(char));
                DIE(log_user < 0, "error memory for login info");
                fgets(log_user, LEN, stdin);

                printf("password:");
                char *log_pass = (char *)calloc(LEN, sizeof(char));
                DIE(log_pass < 0, "error memory for login info");
                fgets(log_pass, LEN, stdin);

                /* get login info without string terminator */
                strtok(log_user, "\n");
                strtok(log_pass, "\n");

                /* newline for delimitation */
                printf("\n");

                /* parsing the message to be sent */
                JSON_Value *root_value = json_value_init_object();
                JSON_Object *root_object = json_value_get_object(root_value);
                char *serialized_string = NULL;
                json_object_set_string(root_object, "username", log_user);
                json_object_set_string(root_object, "password", log_pass);
                serialized_string = json_serialize_to_string_pretty(root_value);

                message = compute_post_request("34.118.48.238", "/api/v1/tema/auth/login",
                            NULL, "application/json", serialized_string, NULL);

                /* connecting to server, send message and receive response */
                sockfd = open_connection("34.118.48.238", 8080, AF_INET, SOCK_STREAM, 0);
                send_to_server(sockfd, message);
                response = receive_from_server(sockfd);
                puts(response);
                printf("\n");

                char *copy_response = response;
                /* if login is successful */
                if (strstr(copy_response, "error") == NULL) {
                    /* compute cookies */
                    cookie = get_cookies(response);
                }

                /* end connection */
                close_connection(sockfd);

                /* free the used buffers */
                json_free_serialized_string(serialized_string);
                json_value_free(root_value);
                free(log_user);
                free(log_pass);
            }
        }

        /* request access to enter library */
        else if (strcmp(command, "enter_library") == 0) {
            sockfd = open_connection("34.118.48.238", 8080, AF_INET, SOCK_STREAM, 0);

            /* sending GET req with cookie to prove that the client is
                logged in */
            message = compute_get_request("34.118.48.238", "/api/v1/tema/library/access",
                             cookie, NULL);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            puts(response);
            printf("\n");

            /* get the JWT returned by the server */
            char *token;
            token = strtok(response, "\n");
                
            while (token != NULL) {
                /* if response not an error */
                if (strstr(token, "token") != NULL) {
                    jwt = strtok(token, ":");
                    jwt = strtok(NULL, "\"");
                }
                token = strtok(NULL, "\n");
            }

            /* end connection */
            close_connection(sockfd);
        }

        /* get info about books */
        else if (strcmp(command, "get_books") == 0) {
            /* newline for delimitation */
            printf("\n");

            sockfd = open_connection("34.118.48.238", 8080, AF_INET, SOCK_STREAM, 0);

            /* sending GET req with JWT to prove that the client has
                access to the library */
            message = compute_get_request("34.118.48.238", "/api/v1/tema/library/books",
                            cookie, jwt);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);

            /* returns an empty list if there are no added books */
            puts(response);
            printf("\n");

            /* end connection */
            close_connection(sockfd);
        }

        /* get info about a book with the given id */
        else if (strcmp(command, "get_book") == 0) {
            printf("id=");
            char *id = (char *) malloc(LEN * sizeof(char));
            DIE(id < 0, "error memory for id");
            fgets(id, LEN, stdin);
            strtok(id, "\n");

            /* newline for delimitation */
            printf("\n");

            /* computing url with the given id */
            char *url = malloc(URLLEN * sizeof(char));
            memset(url, 0, URLLEN);
            strcpy(url, "/api/v1/tema/library/books/");
            strcat(url, id);

            sockfd = open_connection("34.118.48.238", 8080, AF_INET, SOCK_STREAM, 0);

            /* sending GET req with JWT to prove that the client has
                access to the library */
            message = compute_get_request("34.118.48.238", url, NULL, jwt);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            puts(response);
            printf("\n");
            
            /* end connection */
            close_connection(sockfd);

            /* free the used buffers */
            free(id);
            free(url);
        }

        /* add book to the server */
        else if (strcmp(command, "add_book") == 0) {
            /* provide prompts for a book's fields */
            struct book *book = malloc(sizeof(struct book));
            DIE(book < 0, "error memory for book");
            printf("title:");
            fgets(book->title, BOOKLEN, stdin);
            strtok(book->title, "\n");

            printf("author:");
            fgets(book->author, BOOKLEN, stdin);
            strtok(book->author, "\n");

            printf("genre:");
            fgets(book->genre, BOOKLEN, stdin);
            strtok(book->genre, "\n");

            printf("publisher:");
            fgets(book->publisher, BOOKLEN, stdin);
            strtok(book->publisher, "\n");

            printf("page_count:");
            fgets(book->page_count, BOOKLEN, stdin);
            strtok(book->page_count, "\n");

            /* newline for delimitation */
            printf("\n");

            /* parsing the message to be sent */
            JSON_Value *root_value = json_value_init_object();
            JSON_Object *root_object = json_value_get_object(root_value);
            char *serialized_string = NULL;
            json_object_set_string(root_object, "title", book->title);
            json_object_set_string(root_object, "author", book->author);
            json_object_set_string(root_object, "genre", book->genre);
            json_object_set_string(root_object, "publisher", book->publisher);
            json_object_set_number(root_object, "page_count", atoi(book->page_count));
            serialized_string = json_serialize_to_string_pretty(root_value);

            /* sending POST req with JWT to prove that the client has
                access to the library */
            message = compute_post_request("34.118.48.238", "/api/v1/tema/library/books",
                            jwt, "application/json", serialized_string, NULL);

            /* connecting to server, send message and receive response */
            sockfd = open_connection("34.118.48.238", 8080, AF_INET, SOCK_STREAM, 0);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            puts(response);

            /* end connection */
            close_connection(sockfd);

            /* free the used buffers */
            json_free_serialized_string(serialized_string);
            json_value_free(root_value);
            free(book);
        }

        /* delete book from server */
        else if (strcmp(command, "del_book") == 0) {
            printf("id=");
            char *id = (char *) malloc(LEN * sizeof(char));
            DIE(id < 0, "error memory for id");
            fgets(id, LEN, stdin);
            strtok(id, "\n");

            /* newline for delimitation */
            printf("\n");

            /* computing url with the given id */
            char *url = malloc(URLLEN * sizeof(char));
            memset(url, 0, URLLEN);
            strcpy(url, "/api/v1/tema/library/books/");
            strcat(url, id);

            sockfd = open_connection("34.118.48.238", 8080, AF_INET, SOCK_STREAM, 0);

            /* sending DELETE req with JWT to prove that the client has
                access to the library */
            message = compute_del_request("34.118.48.238", url, NULL, jwt);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            puts(response);

            /* end connection */
            close_connection(sockfd);

            /* free the used buffers */
            free(id);
            free(url);
        }

        /* logout */
        else if (strcmp(command, "logout") == 0) {
            sockfd = open_connection("34.118.48.238", 8080, AF_INET, SOCK_STREAM, 0);
            printf("Logging out.....\n\n");

            /* sending GET req with cookie to prove that the client is
                logged in */
            message = compute_get_request("34.118.48.238", "/api/v1/tema/auth/logout",
                            cookie, NULL);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            puts(response);

            /* end connection */
            close_connection(sockfd);

            cookie = NULL;
            jwt = NULL;
        } 
        
        /* the input is an unknown command */
        else {
            printf("Bad command, try again!\n");
        }
    }

    free(command);
    return 0;
}
