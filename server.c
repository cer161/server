#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>
#include <fcntl.h>
#include <dirent.h>
#include <ctype.h>
#include <limits.h>
#include <netdb.h>
#include <signal.h>

// define amount of connections to put into backlog
#define BACKLOG 5

// server is running by default
int running = 1;

pthread_mutex_t locked;

// node for use in hash table
typedef struct node{
    char* key;
    char* value;
    struct node* next;
} node;

//Store key-value pairs in a hashtable
node* hashtable[26];

//Get the index to insert the node: nodes are hashed alphabeticaly 0-a 25-z
int hashCode(char* key){
        int index;
        char first = toupper(key[0]);
        switch(first){
                case 'A':
                        index = 0;
                        break;
                case 'B':
                        index = 1;
                        break;
                case 'C':
                        index = 2;
                        break;
                case 'D':
                        index = 3;
                        break;
                case 'E':
                        index = 4;
                        break;
                case 'F':
                        index = 5;
                        break;
                case 'G':
                        index = 6;
                        break;
                case 'H':
                        index = 7;
                        break;
                case 'I':
                        index = 8;
                        break;
                case 'J':
                        index = 9;
                        break;
                case 'K':
                        index = 10;
                        break;
                case 'L':
                        index = 11;
                        break;
                case 'M':
                        index = 12;
                        break;
                case 'N':
                        index = 13;
                        break;
                case 'O':
                        index = 14;
                        break;
                case 'P':
                        index = 15;
                        break;
                case 'Q':
                        index = 16;
                        break;
                case 'R':
                        index = 17;
                        break;
                case 'S':
                        index = 18;
                        break;
                case 'T':
                        index = 19;
                        break;
                case 'U':
                        index = 20;
                        break;
                case 'V':
                        index = 21;
                        break;
                case 'W':
                        index = 22;
                        break;
                case 'X':
                        index = 23;
                        break;
                case 'Y':
                        index = 24;
                        break;
                case 'Z':
                        index = 25;
                        break;
                default:
                        index = 0;

        }
        return index;
}

//Search for a key in the hashtable -- return the node searched for upon request, NULL upon failure
node* search(char* key){
        //get the hash index
        int index = hashCode(key);
        node* head = hashtable[index];
        while(head != NULL){
                if(strcmp(head->key, key) == 0){
                        return head;
                }
                head = head->next;
        }
        //The key does not exist
        return NULL;
}

//Insert a key value pair into the hashtable
void insert(char* key, char* data){
        //Ensure keys and values do not contain "\0" or "\n"
        //if(strchr(key, '\0') || strchr(key, '\n') || strchr(data, '\0') || strchr(data, '\n')){
        //      printf("Keys and Values must not contain newlines or null terminators\n");
        //      return;
        //}
        int index = hashCode(key);
        node* new = malloc(sizeof(node));
        node* node = search(key);
        if(node != NULL){
                char* oldValue = node->value;
                node->value = data;
        }
        else{
                new->key = malloc(sizeof(char)*1000);
                new->value = malloc(sizeof(char)*1000);
                new->next = malloc(sizeof(node));
                new->next = NULL;
                new->key = key;
                new->value = data;
                //Insert node at next available spot in the linked list
                if(hashtable[index] == NULL){
                        hashtable[index] = new;
                }
                else{
                        node = hashtable[index];
                        while(node != NULL){
                                node = node->next;
                        }
                        node = new;
                }
        }
}



//Delete a key's value from the hashtable -- return the most recent value upon success , NULL upon failure -- sets associated value to NULL
char* delete(char* key){
        node* node_to_delete = search(key);
        if(node_to_delete == NULL){
                return NULL;
        }
        char* prev_val = malloc(sizeof(char)*100);
        prev_val = node_to_delete->value;
        node_to_delete->value = NULL;
        return prev_val;
}

// struct for use in connection threads
struct connection {
    struct sockaddr_storage addr;
    socklen_t addr_len;
    int fd;
};
// method for connection handler
void *connectionHandler(void *arg);
//A key-value storage service
int main(int argc, char** argv){
    if(argc != 2){
        return EXIT_FAILURE;
    }
    char* port = argv[1];

    //open and bind a listening socket on specified port and wait for incoming connection requests
    struct addrinfo hint, *info_list, *info;
    struct connection *con;
    int error, sfd;
    pthread_t tid;

         //Initialize mutex
         error = pthread_mutex_init(&locked, NULL);
         //Check if there was a problem creating the mutex and print to perror
         if(error != 0){
                errno = error;
                perror("pthread_mutex_init");
                exit(1);
        }

    // initialize hints
    memset(&hint, 0, sizeof(struct addrinfo));
    hint.ai_family = AF_UNSPEC;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_flags = AI_PASSIVE;
    // setting AI_PASSIVE means that we want to create a listening socket
    // get socket and address info for listening port
    // - for a listening socket, give NULL as the host name (because the socket is on
    //   the local host)
    error = getaddrinfo(NULL, port, &hint, &info_list);
    if (error != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(error));
        return -1;
    }
    // attempt to create socket
    for (info = info_list; info != NULL; info = info->ai_next) {
        sfd = socket(info->ai_family, info->ai_socktype, info->ai_protocol);

        // if we couldn't create the socket, try the next method
        if (sfd == -1) {
            continue;
        }

        // if we were able to create the socket, try to set it up for
        // incoming connections;
        //
        // note that this requires two steps:
        // - bind associates the socket with the specified port on the local host
        // - listen sets up a queue for incoming connections and allows us to use accept
        if ((bind(sfd, info->ai_addr, info->ai_addrlen) == 0) &&
            (listen(sfd, BACKLOG) == 0)) {
            break;
        }

        // unable to set it up, so try the next method
        close(sfd);
    }

    if (info == NULL) {
        // we reached the end of result without successfuly binding a socket
        fprintf(stderr, "Could not bind\n");
        return -1;
    }

    freeaddrinfo(info_list);

        struct sigaction act;
        //act.sa_handler = handler;
        act.sa_flags = 0;
        sigemptyset(&act.sa_mask);
        sigaction(SIGINT, &act, NULL);

        sigset_t mask;

        sigemptyset(&mask);
        sigaddset(&mask, SIGINT);


    // at this point sfd is bound and listening
    printf("Waiting for connection\n");
        while (running) {
        // create argument struct for child thread
                con = malloc(sizeof(struct connection));
                con->addr_len = sizeof(struct sockaddr_storage);
                // addr_len is a read/write parameter to accept
                // we set the initial value, saying how much space is available
                // after the call to accept, this field will contain the actual address length

                // wait for an incoming connection
                con->fd = accept(sfd, (struct sockaddr *) &con->addr, &con->addr_len);
                // we provide
                // sfd - the listening socket
                // &con->addr - a location to write the address of the remote host
                // &con->addr_len - a location to write the length of the address
                //
                // accept will block until a remote host tries to connect
                // it returns a new socket that can be used to communicate with the remote
                // host, and writes the address of the remote hist into the provided location

        // if we got back -1, it means something went wrong
        if (con->fd == -1) {
            perror("accept");
            continue;
        }

        // temporarily block SIGINT (child will inherit mask)
        error = pthread_sigmask(SIG_BLOCK, &mask, NULL);
        if (error != 0) {
                fprintf(stderr, "sigmask: %s\n", strerror(error));
                abort();
        }

                // spin off a worker thread to handle the remote connection
        error = pthread_create(&tid, NULL, connectionHandler, con);

                // if we couldn't spin off the thread, clean up and wait for another connection
        if (error != 0) {
            fprintf(stderr, "Unable to create thread: %d\n", error);
            close(con->fd);
            free(con);
            continue;
        }

        // otherwise, detach the thread and wait for the next connection request
        pthread_detach(tid);

        // unblock SIGINT
        error = pthread_sigmask(SIG_UNBLOCK, &mask, NULL);
        if (error != 0) {
                fprintf(stderr, "sigmask: %s\n", strerror(error));
                abort();
        }

    }

        puts("No longer listening.");
        pthread_detach(pthread_self());
        pthread_exit(NULL);
    // never reach here
    return 0;
}

//Helper method to send the message to the client
//void sendMessageToClient(char* retMsg, FILE* fout){

//        fprintf(fout, "%s\n", retMsg);
 //       fflush(fout);
//}


void *connectionHandler(void *arg)
{
    // connection to socket
    struct connection *c = (struct connection *) arg;
    // create write file/buffer to send messages from server to client
    int sock2 = dup(c->fd);
    FILE *fout = fdopen(sock2, "w");
    FILE *fin = fdopen (c->fd, "r");
    int error, nread;
    // input[0] - command
    // input[1] - msg length
    // input[2] - key
    // input[3] - value, should be set to an empty string if command is not SET


    ssize_t valread;
    int i =0;
    char* input[4];
    input[3] = "";
    int num_inputs = 3;
    int bytes_to_read = 3;
    char* buffer= malloc(sizeof(char)*100);
    char* chars = malloc(sizeof(char)*100);
    int section = 0;
    int msgLength = 9999;
    int counter = 0;
    char temp[4] = {0};
    int newInput = 0;
    // msg returned by handler to client, indicates if operation was succesful or not
    char* retMsg;
    // error flag, 0 means no error, 1 means error, used when determining if the loop should continue
    error = 0;
    // while no errors have occured, process commands from client
    while(error == 0)
    {
        i = 0;
        while((valread = read( c->fd, buffer, bytes_to_read))>0)
        {
            buffer[valread] = '\0';
                    if(i==0){
                //TODO close the connection succsssfully  if the user enters an invalid command
                if(strcmp(buffer, "SET") != 0 && strcmp(buffer, "GET") != 0 && strcmp(buffer, "DEL") != 0){
                    errno = 1;
                    perror("Invalid command");
                     fprintf(fout, "ERR\nBAD\n");
                     fflush(fout);
                     error = 1;
                    //close the connection
                    close(c->fd);
                    free(c);
                    error = 1;
                    return NULL;
                }
                newInput = 1;
                input[0] = malloc(sizeof(char)*100);
                strcpy(temp, buffer);

                if(strcmp(buffer, "SET") == 0){
                    num_inputs = 4;
                }

                bytes_to_read = 1;
                i++;
                continue;
            }
            //read one byte at a time until we hit a newline
            if(strcmp(buffer, "\n") != 0){
                strcat(chars, buffer);
                counter++;
                //Return error if the user attempts to add MORE characters than specified, shutdown thread
                if(counter==(msgLength-1)){
                    printf("ERROR");
                   // retMsg = "ERR\nLEN";
                    error = 1;
                    fprintf(fout, "ERR\nBAD\n");
                    fflush(fout);
                    return NULL;
                }
            }
            else{
                input[section] = malloc(sizeof(char)*100);
                strcpy(input[section], chars);
                            *chars = '\0';
                if(section == 1){
                    counter=0;
                    msgLength = atoi(input[section]);
                }
                section++;
                if(section == num_inputs) break;
            }
                i++;
        }

   strcpy(input[0],temp);
   printf("command:%s\n", input[0]);
   printf("message length:%d\n", atoi(input[1]));
   printf("key:%s\n", input[2]);
   printf("value:%s\n", input[3]);

    // temp char* to store messages retrieved from hash table, returned to client to display hash table contents when GET is used
    char* tempMsg;
    // length of key and message combined  (add num_inputs-2 to consider newlines)
    int rLen = strlen(input[2]) + strlen(input[3]) + (num_inputs-2);

    // TODO also retMsg needs to be returned to client through a helper method that formats it appropriately

        //sendMessageToClient("HELLO\n");
        newInput = 0;
        // if-else tree to parse commands, should restart after every succesful set of commands/parameters
        // check to see if length is correct, if not, set an error message
        // Return error if the user attempts to add LESS characters than specified


    // lock to ensure that all behavior is deterministic when writing to/ reading from hash table / reading from client
    pthread_mutex_lock(&locked);
        if(rLen == atoi(input[1]))
        {
            // attempt to run GET command, return OKG on success
            if(strcmp(input[0], "GET") == 0)
            {
                // if key is in hash table, get the value associated, else return error
                // TODO return tempMsg to client in similar way to retMSG
                if(search(input[2]))
                {
                    tempMsg = search(input[2])->value;
                    printf("%s\n",tempMsg);
                    //sendMessageToClient(tempMsg, fout);
                    //msg indicates operation success
                    //retMsg = "OKG";
                    fprintf(fout, "OKG\n%d\n%s\n", strlen(tempMsg)+1, tempMsg);
                    fflush(fout);
                }
                // key not found, set return message to notify user, but do not cause error and end loop
                else
                {
                   fprintf(fout, "KNF\n");
                   fflush(fout);
                }
            }
            // attempt to run SET command, return OKS on success
            else if(strcmp(input[0], "SET") == 0)
            {
                //input[3] =
                insert(input[2],input[3]);
                fprintf(fout, "OKS\n");
                fflush(fout);
                printf("inserted %s at %s\n",input[3],input[2]);
            }
            // attempt to run DEL command, return OKD on success
            else if(strcmp(input[0], "DEL") == 0)
            {
                tempMsg = search(input[2])->value;
                if(delete(input[2]))
                {
                    fprintf(fout, "OKD\n%d\n%s\n", strlen(tempMsg)+1, tempMsg);
                    fflush(fout);
    
                }
                else
                {
                    // key not found, set return message to notify user, but do not cause error and end loop
                    fprintf(fout, "KNF\n");
                    fflush(fout);
                }
            }
            else
            {
                // Message is malformed
                fprintf(fout, "ERR\nBAD\n");
                fflush(fout);
                error = 1;
            }
        }
        else
        {
            // Message length is inconsistent with actual length
            fprintf(fout, "ERR\nLEN\n");
            fflush(fout);
            error = 1;
        }
      //  sendMessageToClient(retMsg,fout);
    //exiting critical section
    pthread_mutex_unlock(&locked);
  printf("before freeing\n");
        //Free dynamically allocated data
        free(buffer);
        free(chars);
        for(int k=0; k<num_inputs; k++){
                free(input[k]);
        }
    printf("next msg \n");
    }
    fclose(fout);
    fclose(fin);
    // close and free connection thread
   // close(c->fd);
    free(c);
    return NULL;
}
                                                
                                                                                                                                                                                                                                                                                                           250,1         37%
                                                                                                                                                                                                                                                                                                           165,1-8       18%

                                                         