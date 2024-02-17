
/*
 * CS354: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <ctime>
#include <errno.h>

#include "command.h"

SimpleCommand::SimpleCommand()
{
	// Creat available space for 5 arguments
	_numberOfAvailableArguments = 5;
	_numberOfArguments = 0;
	_arguments = (char **) malloc( _numberOfAvailableArguments * sizeof( char * ) );
}

void
SimpleCommand::insertArgument( char * argument )
{
	if ( _numberOfAvailableArguments == _numberOfArguments  + 1 ) {
		// Double the available space
		_numberOfAvailableArguments *= 2;
		_arguments = (char **) realloc( _arguments,
				  _numberOfAvailableArguments * sizeof( char * ) );
	}
	
	_arguments[ _numberOfArguments ] = argument;

	// Add NULL argument at the end
	_arguments[ _numberOfArguments + 1] = NULL;
	
	_numberOfArguments++;
}

Command::Command()
{
	// Create available space for one simple command
	_numberOfAvailableSimpleCommands = 1;
	_simpleCommands = (SimpleCommand **)
		malloc( _numberOfSimpleCommands * sizeof( SimpleCommand * ) );

	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_inputFile = 0;
	_errFile = 0;
	_background = 0;
}

void
Command::insertSimpleCommand( SimpleCommand * simpleCommand )
{
	if ( _numberOfAvailableSimpleCommands == _numberOfSimpleCommands ) {
		_numberOfAvailableSimpleCommands *= 2;
		_simpleCommands = (SimpleCommand **) realloc( _simpleCommands,
			 _numberOfAvailableSimpleCommands * sizeof( SimpleCommand * ) );
	}
	
	_simpleCommands[ _numberOfSimpleCommands ] = simpleCommand;
	_numberOfSimpleCommands++;
}

void
Command:: clear()
{
	for ( int i = 0; i < _numberOfSimpleCommands; i++ ) {
		for ( int j = 0; j < _simpleCommands[ i ]->_numberOfArguments; j ++ ) {
			free ( _simpleCommands[ i ]->_arguments[ j ] );
		}
		
		free ( _simpleCommands[ i ]->_arguments );
		free ( _simpleCommands[ i ] );
	}

	if ( _outFile ) {
		free( _outFile );
	}

	if ( _inputFile ) {
		free( _inputFile );
	}

	if ( _errFile ) {
		free( _errFile );
	}

	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_inputFile = 0;
	_errFile = 0;
	_background = 0;
	_append = 0;
}

void
Command::print()
{
	printf("\n\n");
	printf("              COMMAND TABLE                \n");
	printf("\n");
	printf("  #   Simple Commands\n");
	printf("  --- ----------------------------------------------------------\n");
	
	for ( int i = 0; i < _numberOfSimpleCommands; i++ ) {
		printf("  %-3d ", i );
		for ( int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++ ) {
			printf("\"%s\" \t", _simpleCommands[i]->_arguments[ j ] );
		}
	}

	printf( "\n\n" );
	printf( "  Output       Input        Error        Background\n" );
	printf( "  ------------ ------------ ------------ ------------\n" );
	printf( "  %-12s %-12s %-12s %-12s\n", _outFile?_outFile:"default",
		_inputFile?_inputFile:"default", _errFile?_errFile:"default",
		_background?"YES":"NO");
	printf( "\n\n" );
	
}

void
Command::execute()
{
	// Don't do anything if there are no simple commands
	if ( _numberOfSimpleCommands == 0 ) {
		prompt();
		return;
	}

	// Print contents of Command data structure
	print();
	
	// Add execution here
	// For every simple command fork a new process
	// Setup i/o redirection
	// and call exec
	
	// duplicate the default input, output and error stream
	int defaultin = dup( 0 );
	int defaultout = dup( 1 );
	int defaulterr = dup( 2 );
	
	int pid;
	
	int outfd, infd, errfd; 
	
	int fdpipe[_numberOfSimpleCommands][2];
	// ls -al | grep command 
	for (int i = 0; i<_numberOfSimpleCommands; i++) { //grep command > o.txt
                if(strcmp(_simpleCommands[i]->_arguments[0], "cd") == 0){
                	int cd = chdir((const char*)_simpleCommands[i]->_arguments[1]);
                	if(cd == -1){
                		printf("cd: %s: No such file or directory\nreturn to home\n",_simpleCommands[i]->_arguments[1]);
                		chdir(getenv("HOME"));
                	}
                	continue;
                }
                if ( pipe(fdpipe[i]) == -1) {
			perror( "pipe");
			exit( 2 );
		}
		// redirect input
		if(i != 0){
			if(_inputFile){
				infd = open(_inputFile, O_RDONLY, 0666);
				if ( infd < 0 ) {
					char* str = strcat(_currentSimpleCommand->_arguments[0]," : create infile");
					perror( str );
					exit( 2 );
				}
				dup2( infd, 0 );
				close(infd);
			}
			else {
				dup2( fdpipe[i - 1][0], 0);
				close(fdpipe[i - 1][0]);
			}
		}
		else {
			if(_inputFile){
				infd = open(_inputFile, O_RDONLY, 0666);
				if ( infd < 0 ) {
					char* str = strcat(_currentSimpleCommand->_arguments[0]," : create infile");
					perror( str );
					exit( 2 );
				}
				dup2( infd, 0 );
				close(infd);
			}
			else{
				dup2( defaultin, 0 );
			}
		}
		
		// redirect output 
		if(i != _numberOfSimpleCommands-1){  
			dup2( fdpipe[i][ 1 ], 1 );
			close(fdpipe[i][1]);
		}
		else if(_outFile) {
			if(_append == 1){
				outfd = open(_outFile, O_WRONLY | O_APPEND | O_CREAT, 0666);	
			}
			else {
				outfd = creat(_outFile, 0666 );
			}
			if ( outfd < 0 ) {
				char* str = strcat(_currentSimpleCommand->_arguments[0]," : create outfile");
				perror( str );
				exit( 2 );
			}
			dup2( outfd, 1 );
			close(outfd);
		}
		else{
			dup2( defaultout, 1);
		}
		
		if(_errFile){
			errfd = creat(_errFile, 0666 );
			if ( errfd < 0 ) {
				char* str = strcat(_currentSimpleCommand->_arguments[0]," : create errfile");
				perror( str );
				exit( 2 );
			}
			dup2( errfd, 2 );
			close(errfd);
		}
		else {
			dup2( defaulterr, 2 );
		}
	
		// fork 
		pid = fork();
		if ( pid == -1 ) {
			char* str = strcat(_currentSimpleCommand->_arguments[0],": fork\n");
			perror( str);
			exit( 2 );
		}
		else if (pid == 0) {
			//Child
		
			// close file descriptors that are not needed
			close( defaultout );
			close( defaultin );
			close( defaulterr );
			// You can use execvp() instead if the arguments are stored in an array
			execvp(_simpleCommands[i]->_arguments[0], &_simpleCommands[i]->_arguments[0]);
			// exec() is not suppose to return, something went wrong
			char* str = strcat(_currentSimpleCommand->_arguments[0],": exec ");
			str = strcat(str, _currentSimpleCommand->_arguments[0]);
			perror( str );
			exit( 2 );
		}
		else {
			signal(SIGCHLD, SIGCHLDhandeler);
			dup2( defaultin, 0 );
			dup2( defaultout, 1 );
			dup2( defaulterr, 2 );
			
			// Wait for last process in the pipe line
			if (_background == 0) {
				waitpid( pid, 0, 0 );
			}
			// Restore input, output, and error
	
		}
			
	}
	
	// Clear to prepare for next command
	clear();
	
	// Print new prompt
	prompt();
}

// Shell implementation

void 
removeNewline(char *str, int size) {
	for (int i = 0; i < size; i++){
		if (str[i] == '\n'){
			str[i] = '\0';
			return;
		}
	}
}

void
SIGINThandeler(int sig)
{
	signal(SIGINT, SIGINThandeler); 
	printf("\33[2K\r");
	Command::_currentCommand.clear();
	Command::_currentCommand.prompt();
}

FILE* openLogFile(){
	FILE *logfile;
	logfile = fopen("log.txt","a+");
	if(logfile == NULL){
		perror("fopen");
		return NULL;
	}
	return logfile;
}


void 
SIGCHLDhandeler(int sig){
	FILE* logfile;
	logfile = openLogFile();
	int status;
	wait(&status);
	time_t TIMER = time(NULL);
	tm *ptm = localtime((&TIMER));
	char currentTime[32];
	strcpy(currentTime, asctime(ptm));
	removeNewline(currentTime, 32);
	fprintf(logfile, "%s: Child Process %d Terminated\n", currentTime, getpid());
	fclose(logfile);
	signal(SIGCHLD, SIGCHLDhandeler);
}


void
Command::prompt()
{
	char cwd[1024];
	getcwd(cwd, 1024);
	signal(SIGINT, SIGINThandeler);
	printf("myshell>%s>",cwd);
	fflush(stdout);
}

Command Command::_currentCommand;
SimpleCommand * Command::_currentSimpleCommand;

int yyparse(void);

int 
main()
{
	Command::_currentCommand.prompt();
	yyparse();
	return 0;
}

