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
	//	printf("Keys and Values must not contain newlines or null terminators\n");
	//	return;
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

//A key-value storage service
int main(int argc, char** argv){
    if(argc != 2){
        return EXIT_FAILURE;
    }
    int port = atoi(argv[1]);
    if(port == 0){
        return EXIT_FAILURE;
    }

    node root;
    root.key = malloc(sizeof(char)*100);
    root.value = malloc(sizeof(char)*100);
    root.next = malloc(sizeof(node));
    root.key = "day";
    root.value = "Monday";
    root.next->key = "month";
    root.next->value = "February";
    hashtable[hashCode("day")] = &root;
    insert("season", "Summer");
 
    node* traverse = &root;
    while(traverse!=NULL){
            printf("%s\n", traverse->key);
	    
            traverse = traverse->next;
    }
    node* temp = search("day");
    printf("Value of day key is: %s\n", temp->value);
    node* tem = search("season");
    if(tem == NULL){
	printf("Insert not working correctly\n");
    }
    else{
   	 printf("Value of season key is: %s\n", tem->value);
    }
    //open and bind a listening socket on specified port and wait for incoming connection requests
    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int opt = 1;
    char buffer[1024] = {0};
    char* hello = "Hello from server";
    //Create socket
    if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0){
		perror("socket failed");
		return(EXIT_FAILURE);
    }
    //Open socket to port
    if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))){
		perror("socket failed");
		return(EXIT_FAILURE);
   }
   address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( port );
       
    // bind socket to specified port
    if (bind(server_fd, (struct sockaddr *)&address, 
                                 sizeof(address))<0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, 
                       (socklen_t*)&addrlen))<0)
    {
        perror("accept");
        exit(EXIT_FAILURE);
    }
    valread = read( new_socket , buffer, 1024);
    printf("%s\n",buffer );
    send(new_socket , hello , strlen(hello) , 0 );
   
    printf("Port number to listen on: %d\n", port);
    printf("Hi from server (:\n");
	return 0;
}
