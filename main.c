#include <stdio.h>
#include <unistd.h>
#include <string.h>

void fetch_info(){
	char buffer[1024];
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

int main(){
	fetch_info();
	return 0;
}