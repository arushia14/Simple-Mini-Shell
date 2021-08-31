// Modify this file for your assignment
#include <stdio.h>
#include <stdlib.h>
#include <signal.h> // this is for signal
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
 
#define MAX_BUFFER_SIZE 80
#define MAX_DEPTH 30


// This is a signal handler to help stop the program.
void sigint_handler(int sig){
        // the write here has 22 as last parameter due to string size
        write(1,"\nmini shell terminated\n", 23);
	// since this would be used to terminate a program when there
	// is a problem, exit(1) is used
        exit(1);
}

//----------- Building a DLL for the history------------------

typedef struct node{
	char* data;
	struct node* next;
}node_t;

typedef struct dll{
	int count; // for size of doubly linked list
	unsigned int capacity; // the maximum size of the list
	node_t* head; // the head node
	node_t* tail; // the tail node
}dll_t;


dll_t* create_dll(){
	dll_t* myDll = malloc(sizeof(dll_t));
	if (myDll == NULL){
		return NULL;
	}
	myDll->count = 0;
	myDll->capacity = MAX_DEPTH; // All the lists will have capacity of MAX_DEPTH
	myDll->head = NULL;
	myDll->tail= NULL;
	return myDll;
}

int dll_empty(dll_t* s){
	if (s == NULL){
		return -1;
	}
	if (s->count == 0){
		return 1;
	} else {
		return 0;
	}
}


int dll_full(dll_t* s){
        if (s == NULL){
                return -1;
        }
        if (s->count == s->capacity){
                return 1;
        } else {
                return 0;
        }
}

void dll_enqueue(dll_t* s, char* _data){
	if (s == NULL){
		return;
	}
	// create new node
	node_t* newnode = malloc(sizeof(node_t*));
	if (newnode == NULL){
		return;
	}
	newnode->data = (char*)malloc(strlen(_data)*sizeof(char)+1);
	if (newnode->data == NULL){
		return;
	}
	strcpy(newnode->data, _data);
	newnode->next = NULL;
	// check if empty and return if so
	if (dll_empty(s) == 1){
		s->head = newnode;
		s->tail = newnode;
		s->count = s->count + 1;
		return;
	}
	// delete the oldest item if at capacity
        if (dll_full(s) == 1) {
                node_t* newhead = s->head->next;
                free(s->head->data);
                free(s->head);
                s->head = newhead;
                s->count = s->count - 1;
        }
	// add the new node as the tail to existing list
	s->tail->next = newnode;
	s->tail = newnode;
	s->count = s->count +1;
	return;
}

void free_dll(dll_t* d){
	if(d != NULL){
		node_t* currentnode = d->head;
		node_t* iterator = currentnode;
		while (iterator != NULL){
			iterator = currentnode->next;
			free(currentnode->data);
			free(currentnode);
			currentnode = iterator;
		}
		// free list at end
		free(d);
	}
	return;
}

//----------------------------------------------------------

// This function will allow us to exit the shell.
// The history list will also be cleared when properly exited
void exit_function(char *charinput, dll_t* d){
	free_dll(d);
        printf("mini shell exited\n");
	// exit(0) is for program termination
        exit(0);
}

// This function provides descriptions of the built in functions.
void help_function(char *charinput, dll_t* d){
        printf("\nexit:\n");
	printf("Serves to successfully exit the program.\n");
	printf("No parameters are needed for input alongside this command.\n\n\n");
	printf("help:\n");
	printf("Provides descriptions of the built in commands.\n");
	printf("No parameters are needed for input alongside this command.\n\n\n");
	printf("cd:\n");
	printf("Changes the directory to the specified path.\n");
	printf("The path name will need to be entered after cd.\n");
	printf("If trying to go up a directory, enter '..'.\n\n\n");
	printf("history:\n");
	printf("Prints out at maximum the last 30 user inputted commands.\n");
	printf("No parameters are needed for input alongside this command.\n\n\n");
        return;
}
 

// This function serves to change the directories
// It will print an error stating that no such directory exists if fails
void cd_function(char *path, dll_t* d){
	// change directory using chdir
	// check that the correct change was made
	if (chdir(path) != 0){
        	printf("Error: cd %s: No such directory.\n", path);
	}
        return;
}

// This function will output a history of the past commands, up to 30 max.
void history_function(char *charinput, dll_t* d){
        if (d == NULL){
		printf("Error: No history.\n");
	}
	printf("Last user inputted commands:\n");
	node_t* iterator = d->head;
	while (iterator != NULL){
		printf("%s\n", iterator->data);
		iterator = iterator->next;
	}
        return;
}

// This function serves to run commands with a pipe
void pipe_command(char *commandline){
	// check the first input line using strtok
        char* pipeargv[16];
        int j = 0;
        char *token;
        token = strtok(commandline, " ");  // strtok breaks strings into tokens ls -l | wc
        while (token != NULL){             
                pipeargv[j] = token;
                token = strtok(NULL, " ");
                j = j + 1;
        }
        pipeargv[j] = NULL;
 
	//need array of int to hold file descriptor
	int fd[2];
	// now use pipe
        pipe(fd); 
        // store the child process id
        pid_t childProcessID;
        // store the status of child execution
        int child_status;
        // execute fork and duplicate parent
        childProcessID = fork();
        // check child was successfully created
        if (childProcessID == -1){
		printf("Fork failed");
                exit(EXIT_FAILURE);
        }
        if (childProcessID == 0){
                char* newargv[3];
                newargv[0] = pipeargv[0];
                newargv[1] = pipeargv[1];
		newargv[2] = NULL;
                // we want to capture output so will do so with dup2
                // it will duplicate file descriptor fd[1] into
                // STDOUT_FILENO
                dup2(fd[1], STDOUT_FILENO);
                close(fd[1]);
                close(fd[0]);
                // now execute first part of commands
                execvp(newargv[0], newargv);
                // for any error input:
                printf("mini-shell>Command not found--Did you mean something else?\n");
                exit(1);
                // now done with child
	} else {
                // now in parent again
                // wait for child to finish
                waitpid(childProcessID, &child_status, 0);
		// make another arg array for commands
                char* lastargv[3];
                lastargv[0] = pipeargv[3];
                lastargv[1] = pipeargv[4];
                lastargv[2] = NULL;
                // now use output from other process as input
                dup2(fd[0], STDIN_FILENO);
                close(fd[1]);
                close(fd[0]);
                // now execute second part of commands
                execvp(lastargv[0], lastargv);
                printf("mini-shell>Command not found--Did you mean something else?\n");
		exit(1);
	}
	return;
} 

void parse_line(char *inputline, dll_t* d){
        // array of commands
        char command_array[4][9] = {"help", "exit", "cd", "history"};
        // array of function pointers
        void (*fun_ptr_arr[])(char *, dll_t*) = {help_function, exit_function,
                cd_function, history_function};
	// check to see if it has a pipe and pass to pipe function if so
	if (strstr(inputline, "|") != NULL){
		if (fork() == 0){
			pipe_command(inputline);
		} else {
			wait(NULL);
			return;
		}
	}
        // check the first input line using strtok
        char* myargv[16];
        int j = 0;
        char *token;
        token = strtok(inputline, " ");
        while (token != NULL){
                myargv[j] = token;
                token = strtok(NULL, " ");
                j = j + 1;
        }
	myargv[j] = NULL;
        int i;
        for (i = 0; i < 4; i++){
                if (strcmp(myargv[0], command_array[i]) == 0){
                        (*fun_ptr_arr[i])(myargv[1], d);
                        return;
                }
        }
	// this is for single commands
        if (fork() == 0){
                // execute command from child
               execvp(myargv[0], myargv);
                // only print this if command doesn't show up
                printf("mini-shell>Command not found--Did you mean something else?\n");
                exit(1);
        } else {
                wait(NULL);
                return;
        }
}


int main(){
  // Please leave in this line as the first statement in your program.
  // alarm(120); // This will terminate your shell after 120 seconds,
              // and is useful in the case that you accidently create a 'fork bomb'
  
  char line[MAX_BUFFER_SIZE];
	dll_t* history = create_dll();
        // now to put signal handler in the code
        signal(SIGINT, sigint_handler);
        
        char* username = getenv("USER");

 
        while(1){
                // start by printing the line
                printf("%s-mini-shell>", username);
                // get the input from the keyboard
                fgets(line, MAX_BUFFER_SIZE, stdin);
                // check that the user actually inputted something
                if (strlen(line) > 1){
			// this is to ignore the end of line character
                        line[strlen(line)-1] = '\0';
			// add to history of commands
			dll_enqueue(history, line);
                        // now call function to parse line
                        parse_line(line, history);
                }
        }
 
        return 0;
}
