#include "systemcalls.h"
#include<syslog.h>
#include<stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include<sys/wait.h>
#include<stdio.h>
#include <fcntl.h>
/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{

/*
 * TODO  add your code here
 *  Call the system() function with the command set in the cmd
 *   and return a boolean true if the system() call completed with success
 *   or false() if it returned a failure
*/
	openlog("syslog",LOG_PID | LOG_CONS ,LOG_USER);
	
	if(cmd == NULL){
		syslog(LOG_ERR,"command is NULL\n");
		return false;
		}
		
	int ret = system(cmd);
	
	if(ret == -1){
		syslog(LOG_ERR,"child process not created\n");
		return false;
		}
		
	if(WIFEXITED(ret)){
		syslog(LOG_DEBUG,"Normal termination with exit status=%d\n",WEXITSTATUS(ret));
		return true;
		}
	else
		return false;

	closelog();
    	return true;
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
	va_list args;
	va_start(args, count);
	char * command[count+1];
	int i;
	for(i=0; i<count; i++)
	    {
		command[i] = va_arg(args, char *);
	    }
	command[count] = NULL;
	    // this line is to avoid a compile warning before your implementation is complete
	    // and may be removed
	//command[count] = command[count];


	openlog("syslog",LOG_PID | LOG_CONS ,LOG_USER);
		
 	pid_t chld_pid = fork();
 	
	if (chld_pid == -1){
	    	perror("fork");
	    	syslog(LOG_ERR,"fork failed");
	    	va_end(args);
	    	return false;
	    	}
	    	
	else if(chld_pid == 0){
	    	if(execv(command[0],command)==-1){
			perror("execv");
			syslog(LOG_ERR,"execv failed");
			exit(1);
			
			}
		}
		
	else {
		int status;
		if(waitpid(chld_pid,&status,0)==-1){
			perror("waitpid");
			syslog(LOG_ERR,"wait error\n");
			va_end(args);
			return false;
			}
	
					
		if(WIFEXITED(status)){
			int exit_status = WEXITSTATUS(status);
			if(exit_status == 0){
				va_end(args);
				return true;
				}
			}
		}
			
			
	 va_end(args);
	 closelog();
	 return false;
}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
	va_list args;
	va_start(args, count);
        char * command[count+1];
        int i;
        for(i=0; i<count; i++)
        {
        	command[i] = va_arg(args, char *);
         }
    	command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];

	openlog("syslog",LOG_PID | LOG_CONS ,LOG_USER);
	//open file to redirect
	
	int fd = open(outputfile,O_RDWR |O_TRUNC | O_CREAT,0666);
	
	if (fd == -1){
		perror("open");
		syslog(LOG_ERR,"can't open file\n");
		va_end(args);
		return false;
		}
		
	pid_t chld_pid = fork();
	
	if(chld_pid == -1){
		perror("fork");
	    	syslog(LOG_ERR,"fork failed\n");
	    	close(fd);
	    	va_end(args);
	    	return false;
	    	}
	
	else if(chld_pid == 0){
	     	if(dup2(fd,STDOUT_FILENO)==-1){
			perror("dup2");
			syslog(LOG_ERR,"dup2 failed,failed to redirect\n");
			close(fd);
			va_end(args);
			return false;
			}
		close(fd);
		
		if(execv(command[0],command)==-1){
			perror("execv");
			syslog(LOG_ERR,"execv failed\n");
			exit(1);
			}
			
		}
	else{
		int status;
		if(waitpid(chld_pid,&status,0)==-1){
			perror("waitpid");
			syslog(LOG_ERR,"wait error\n");
			close(fd);
			va_end(args);
			return false;
			}
		close(fd);
					
		if(WIFEXITED(status)){
			int exit_status = WEXITSTATUS(status);
			if(exit_status == 0){
				va_end(args);
				return true;
				}
			}
		}
				
	     	
		
		
	
	

    	va_end(args);
    	closelog();

    	return false;
}
