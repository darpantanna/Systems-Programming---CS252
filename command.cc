/*
 * CS252: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "command.hh"

void ctrl(int);
void ctrlZ(int);
void zombieElimination(int);

Command::Command()
{
	// Create available space for one simple command
	_numOfAvailableSimpleCommands = 1;
	_simpleCommands = (SimpleCommand **)
		malloc( _numOfSimpleCommands * sizeof( SimpleCommand * ) );

	_numOfSimpleCommands = 0;
	_outFile = 0;
	_inFile = 0;
	_errFile = 0;
	_background = 0;

	_append = 0; // Variable to append
	//Ambiguous redirect check
	_in = 0; 
	_out = 0;
}

void Command::insertSimpleCommand( SimpleCommand * simpleCommand ) {
	if ( _numOfAvailableSimpleCommands == _numOfSimpleCommands ) {
		_numOfAvailableSimpleCommands *= 2;
		_simpleCommands = (SimpleCommand **) realloc( _simpleCommands,
			 _numOfAvailableSimpleCommands * sizeof( SimpleCommand * ) );
	}
	
	_simpleCommands[ _numOfSimpleCommands ] = simpleCommand;
	_numOfSimpleCommands++;
}

void Command:: clear() {
	for ( int i = 0; i < _numOfSimpleCommands; i++ ) {
		for ( int j = 0; j < _simpleCommands[ i ]->_numOfArguments; j ++ ) {
			free ( _simpleCommands[ i ]->_arguments[ j ] );
		}
		
		free ( _simpleCommands[ i ]->_arguments );
		free ( _simpleCommands[ i ] );
	}

	if(_outFile == _errFile) {
		if ( _outFile ) {
			free( _outFile );
			_errFile = NULL;
		}
	}

	else {
		if( _outFile ) {
			free( _outFile );
		}
		if( _errFile ) {
			free ( _errFile );
		}
	}

	if ( _inFile ) {
		free( _inFile );
	}

	/*if ( _errFile ) {
		free( _errFile );
	}*/

	_numOfSimpleCommands = 0;
	_outFile = 0;
	_inFile = 0;
	_errFile = 0;
	_append = 0;
	_background = 0;
	_in = 0;
	_out = 0;
}

void Command::print() {
	printf("\n\n");
	printf("              COMMAND TABLE                \n");
	printf("\n");
	printf("  #   Simple Commands\n");
	printf("  --- ----------------------------------------------------------\n");
	
	for ( int i = 0; i < _numOfSimpleCommands; i++ ) {
		printf("  %-3d ", i );
		for ( int j = 0; j < _simpleCommands[i]->_numOfArguments; j++ ) {
			printf("\"%s\" \t", _simpleCommands[i]->_arguments[ j ] );
		}
	}

	printf( "\n\n" );
	printf( "  Output       Input        Error        Background\n" );
	printf( "  ------------ ------------ ------------ ------------\n" );
	printf( "  %-12s %-12s %-12s %-12s\n", _outFile?_outFile:"default",
		_inFile?_inFile:"default", _errFile?_errFile:"default",
		_background?"YES":"NO");
	printf( "\n\n" );
	
}

void Command::execute() {
	// Don't do anything if there are no simple commands
	if ( _numOfSimpleCommands == 0 ) {
		prompt();
		return;
	}

	// Print contents of Command data structure
//	print();

	// Add execution here
	// For every simple command fork a new process
	// Setup i/o redirection
	// and call exec
	
	//Check for exit command	
	if( strcmp( _simpleCommands[0] -> _arguments[0], "exit") == 0 || strcmp( _simpleCommands[0] -> _arguments[0], "logout") == 0 || strcmp( _simpleCommands[0] -> _arguments[0], "bye") == 0 || strcmp( _simpleCommands[0] -> _arguments[0], "quit") == 0  ) {
		printf("Thank you! GG\n");
		exit(1);
	}	
	int check;
	//Set environment
	if( strcmp( _simpleCommands[0] -> _arguments[0], "setenv") == 0 ) {
		check = setenv( _simpleCommands[0] -> _arguments[1], _simpleCommands[0] -> _arguments[2], 1);
		if( check ) {
			perror("setenv");
		}

		clear();
		prompt();
		return;
	}
	//Un Set environment
	if( strcmp( _simpleCommands[0] -> _arguments[0], "unsetenv") == 0 ) {
		check = unsetenv( _simpleCommands[0] -> _arguments[1]);
		if( check ) {
			perror("setenv");
		}

		clear();
		prompt();
		return;
	}
	// Source check
	/*if( strcmp ( _simpleCommands[0] -> _arguments[0], "source") == 0 ) {
		check = source( _simpleCommands[0] -> _arguments[1]);
		if ( check ) {
			perror("source");
		}

		clear();
		prompt();
		return;
	}*/
	//Change directory check
	if ( strcmp( _simpleCommands[0] -> _arguments[0], "cd") == 0) {
		if( _simpleCommands[0] -> _numOfArguments != 1) {
			check = chdir( _simpleCommands[0] -> _arguments[1]);
		}
		else {
			check = chdir(getenv("HOME"));	
		}
		
		if (check != 0) {
			perror("chdir");
		}

		clear();
		prompt();
		return;
	}	
	
	if(_in > 1|| _out > 1) {
		//Ambiguous redirect check
		printf("Ambiguous output redirect");
		clear();
		prompt();
		return;
	}


	int in = dup(0);
	int out = dup(1);
	int err = dup(2);
	int i, fin, fout, ferr, process;

	if(_inFile) {
		fin = open(_inFile, O_RDONLY); //To read the file
	}
	else {
		fin = dup(in); //Use preset input
	}

	if(_errFile) {
		if(!_append) {
			ferr = open(_errFile, O_WRONLY|O_CREAT|O_TRUNC, 0600);
		}
		else {
			ferr = open(_errFile, O_WRONLY|O_CREAT|O_APPEND, 0600);
		}
	}
	else {
		ferr = dup(err);
	}
	dup2(ferr,2);
	close(ferr);
	i = 0;

	while(i < _numOfSimpleCommands) {
		dup2(fin, 0);
		close(fin);	
		
		if( i == _numOfSimpleCommands - 1) {
			if(_outFile != 0) {
				if(!_append) {
					fout = open(_outFile, O_WRONLY|O_CREAT|O_TRUNC, 0600);
				}
				else {
					fout = open(_outFile, O_WRONLY|O_CREAT|O_APPEND, 0600);
				}
			}
			else {
				fout = dup(out);
			}
			if(_errFile != 0) {
				if(!_append) {
					ferr = open(_errFile, O_WRONLY|O_CREAT|O_TRUNC, 0600);
				}
				else {
					ferr = open(_errFile, O_WRONLY|O_CREAT|O_APPEND, 0600);

				}
			}
			else {
				ferr = dup(err);
			}
		}
		else {
			int fpipe[2];
			pipe(fpipe);
			fin = fpipe[0];
			fout = fpipe[1];
		}
		
		dup2(fout, 1);
		close(fout);
		
		process = fork();
		if(process < 0) {
			perror ("Fork");
			exit(2);
		}
		else if (process == 0){
			
			//Printenv check
		/*	if (strcmp( _simpleCommands[i] -> _arguments[0], "printenv")) {
				char ** e = environ;
				while(*e != NULL) {
					printf("%s\n", *e);
					e++;
				}
			}*/	
			signal(SIGINT, SIG_DFL); //Check if ctrl-c
			execvp(_simpleCommands[i] -> _arguments[0], _simpleCommands[i] -> _arguments);
			perror("execvp");
			_exit(1);
		} 

		i++;
	}

	dup2(in, 0);
	close(in);
	dup2(out,1);
	close(out);
	dup2(err,2);
	close(err);

	if(!_background) {
		waitpid(process, NULL, 0);
	}
	// Clear to prepare for next command
	clear();
	
	// Print new prompt
	prompt();
}

// Shell implementation

void Command::prompt() {
	if(isatty(0)) { //Only print when taking stdin 
		printf("Derp > ");
		fflush(stdout);
	}
}

void ctrl(int sig) {
	printf("\n");
	Command::_currentCommand.prompt();
}

void ctrlZ(int sig) {
	printf("\n");
	Command::_currentCommand.prompt();
}
void zombieElimination(int sig) {
	int process = wait3(0,0,NULL);
	while(waitpid(-1,NULL,WNOHANG) > 0);	
}
Command Command::_currentCommand;
SimpleCommand * Command::_currentSimpleCommand;

//int yyparse(void);

/*int main() {
	
	struct sigaction sa; //ctrl-c from ctrl-c.cc
	sa.sa_handler = disp;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART; //to retry if system is interrupted
	
	if(sigaction(SIGINT, &sa, NULL)) {
		perror("sigaction");
		exit(-1);
	}
}*/
