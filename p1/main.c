#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>

typedef struct node_t {
	pid_t pid;
	char cmd[1024];
	struct node_t *next;
} node_t;

void fetch_info(char *buffer){
	char hostname[256];
       	gethostname(hostname, sizeof(hostname));	
		
	char curr_dir[256];
	getcwd(curr_dir, sizeof(curr_dir));

	strcat(buffer, getlogin());
	strcat(buffer, "@");
	strcat(buffer, hostname);
	strcat(buffer, ": ");
	strcat(buffer, curr_dir);
	strcat(buffer, " > ");
}

void execute_cmd(char **tokenized){
	pid_t pid = fork();

	if(pid < 0)
		perror("pid");
	else if(pid > 0)
		wait(NULL);
	else
		if(execvp(tokenized[0], tokenized) == -1) perror("execvp");	
}

char **tokenize_str(char *cmd, int *num_cmd){
	size_t init_size = 2;
	char *t = strtok(cmd, " ");
	char **t_cmd = malloc(sizeof(char*));

	int i = 0;
	while(t != NULL){
		if( i >= init_size){
			init_size *= 2;
			char **tmp = (char **)realloc(t_cmd, init_size * sizeof(char *));

			if(tmp == NULL) perror("realloc");

			t_cmd = tmp;
		}
		t_cmd[i++] = t;
		t = strtok(NULL, " ");

	}
	*num_cmd = i;
	t_cmd[i] = NULL;	
	
	return t_cmd;
}

void change_dir(char **tokenized, int num_cmd){
	if(tokenized[1] == NULL || strcmp(tokenized[1], "~") == 0)
		chdir(getenv("HOME"));
	else if(strcmp(tokenized[1], "../") == 0 || strcmp(tokenized[1], "..") == 0)
		chdir("..");
	else if(num_cmd > 2 && tokenized[2] != NULL)
		perror("chdir");
	else
		chdir(tokenized[1]);
}

int total_pro(node_t **head){
	node_t* curr = *head;

	int total = 0;
	while(curr != NULL){
		total++;
		curr = curr->next;
	}

	return total;
}

void background_pro(char **tokenized, node_t **head){
	pid_t pid = fork();

	if(pid < 0)
		perror("fork");
	else if(pid == 0)
		execvp(tokenized[1], tokenized + 1);
	else{
		node_t* new_node = (node_t*)malloc(sizeof(node_t));
		
		if(new_node == NULL) perror("malloc");

		new_node->cmd[0] = '\0';
		new_node->pid = pid;

		int i = 0;
		while(tokenized[i] != NULL){
			if(strcmp(tokenized[i], "bg") != 0){
				strcat(new_node->cmd, tokenized[i]);
				strcat(new_node->cmd, " ");
			}
			i++;
		}
		new_node->next = * head;
		*head = new_node;
	}
		
}

void background_list(node_t **head){
	node_t* curr = *head;

	while(curr != NULL){
		printf("%d: %s\n", curr->pid, curr->cmd);
		curr = curr->next;
	}
	printf("Total Background jobs: %d\n", total_pro(head));
}

void check_pro(node_t **head){
	node_t* curr = *head;
	
	if(total_pro(&curr) > 0){
		pid_t ter = waitpid(0, NULL, WNOHANG);

		while(ter > 0){
			if(ter > 0){
				if(curr->pid == ter){
					printf("%d: %s has terminated\n", curr->pid, curr->cmd);
					curr = curr->next;
				}else{
					while(curr->next->pid != ter)
						curr = curr->next;
					printf("%d: %s has terminated\n", curr->pid, curr->cmd);
					curr->next = curr->next->next;
				}
			}
			ter = waitpid(0, NULL, WNOHANG);
		}
	}
}

int main(){
	while(1){
		char buffer[512];
		buffer[0] = '\0';

		fetch_info(buffer);

		char *cmd = readline(buffer);	

		int num_cmd = 0;
		char **tokenized = tokenize_str(cmd, &num_cmd);
		
		node_t *head;
		check_pro(&head);

		if(num_cmd == 0) continue;

		if(strcmp(tokenized[0], "cd") == 0)
			change_dir(tokenized, num_cmd);	
		else if(strcmp(tokenized[0], "bg") == 0)
			background_pro(tokenized, &head);
		else if(strcmp(tokenized[0], "bglist") == 0)
			background_list(&head);
		else if(strcmp(tokenized[0], "exit") == 0)
			exit(1);
		else 
			execute_cmd(tokenized);
	}

	return 0;
}
