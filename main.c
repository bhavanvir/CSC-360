#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>


void fetch_info(char *buffer){
	buffer[0] = '\0';

	char *login = getlogin();

	char hostname[1024];
       	gethostname(hostname, sizeof(hostname));
		
	char curr_dir[1024];
	getcwd(curr_dir, sizeof(curr_dir));

	strcat(buffer, login);
	strcat(buffer, "@");
	strcat(buffer, hostname);
	strcat(buffer, ": ");
	strcat(buffer, curr_dir);
	strcat(buffer, " > ");	
}

void exec_cmd(char **cmd){
	pid_t pid = fork();

	if(pid == -1)
		perror("pid");
	else if(pid > 0)
		wait(NULL);
	else{
		if(execvp(cmd[0], cmd) == -1)
			perror("execvp");
	}

}

int main(){
	char buffer[1024];
	fetch_info(buffer);
	char *cmd = readline(buffer);

	return 0;
}