#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>

void fetch_info(char *buffer){
	char hostname[64];
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

void exec_cmd(char **tokenized){
	pid_t pid = fork();

	if(pid == -1)
		perror("pid");
	else if(pid > 0)
		wait(NULL);
	else
		if(execvp(tokenized[0], tokenized) == -1) perror("execvp");	
}

char **tokenize_str(char *cmd, int *num_cmd){
	size_t init_size = 1;
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

int main(){
	while(1){
		char buffer[512];
		buffer[0] = '\0';

		fetch_info(buffer);

		char *cmd = readline(buffer);	

		int num_cmd = 0;
		char **tokenized = tokenize_str(cmd, &num_cmd);

		if(num_cmd == 0) continue;

		if(strcmp(tokenized[0], "cd") == 0)
			change_dir(tokenized, num_cmd);	
		else if(strcmp(tokenized[0], "bg") == 0)
			//TODO
			printf("bg\n");
		else if(strcmp(tokenized[0], "bglist") == 0)
			//TODO
			printf("bglist\n");
		else
			exec_cmd(tokenized);
	}
	return 0;
}
