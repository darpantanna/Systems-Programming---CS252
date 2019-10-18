
/*
 *
 * CS-252 Fall 2017
 * shell.l: lexical analyzer for shell
 * You have to extend it.
 *
 */

%{
#include "command.hh"
#include <cstring>
#include "y.tab.hh"
#include <sys/wait.h>
//IMPLEMENT 2>
static  void yyunput (int c,char *buf_ptr  );
int SIZE = 1024;
void myunputc(int c) {
  unput(c);
}

	Command * c = new Command();
%}

%%

\n {
  return NEWLINE;
}

[ \t] {
  /* Discard spaces and tabs */
}

">" {
  return GREAT;
}

">>" {
		
	return GREATGREAT;
}

"<" {
	return LESS;
}

"|" {
	return PIPE;
}

"&" {
	return AMPERSAND;
}

"2>" {
	return TGREAT;
}

">&" {
	return GREATAMPERSAND;
}

">>&" {
	return GREATGREATAMPERSAND;
}

["][^\n\"]*["] {
	/* Quotes handler*/
	//printf("Quote\n");	
	//yytext[strlen(yytext) + 1] = '\0';
	
	yylval.string_val = strdup(yytext+1);
	if(yylval.string_val[strlen(yylval.string_val) - 1] == '"') {
		//printf("HERE");
		yylval.string_val[strlen(yylval.string_val) - 1] = 0;
	}

	return WORD;
}

[^ \t\n|><&]*\\[^ \t\n]* {
	/*Escape Character handler*/
	char * check = (char*) malloc(SIZE);
	int i;
	int count = 0;
	char * copy = yytext;		
	
    //printf("%s\n", yytext);

    //printf("HERE\n");
	/*for(i = 0; *copy != '\0'; i++) {
		if(*copy != '\\') {
			check[count] == *copy;
			//copy++;
		}
		else if( *copy == '\\' && *(copy+1) == '\\') {
				printf("HERE\n");			
				copy = copy+2;
				check[count] = '\\';	
		}
		else if( *copy == '\\' && *(copy+1) != '\\') {  
				//printf("HERE NOW\n");
				copy = copy+1;
				check[count] = *(copy);
		}
		copy++;
		count++;
	}
	*/

	i = 0;
	//printf("COPY %s\n", copy);
	while(*copy != '\0') {
		if(*copy == '\\') {
			check[count] = *(copy+1);
			copy++;
		}
		else {
			check[count] = *copy;
		}
		copy++;
		count++;
	}
	//printf("%s\n", copy);
	check[count] = '\0';
	//printf("%s\n", check);
    yylval.string_val = strdup(check);
	return WORD;
}

\`[^\n\`]*\` {
	
	//int i;
	char * check = (char*) calloc(sizeof(char), SIZE);
	
	char read_pipe; 
	//USe current input and output
	int in = dup(0);
	int out = dup(1);
	//int err = dup(2);
	
	//Two PIPES for re-direction, input to take it in subshell and out to output the sub-shell to shell
	int in_pipe[2];
	//int out_pipe[2];
	pipe(in_pipe);
	//pipe(out_pipe);

	//Eliminate back ticks
	char * sshell = strdup(yytext+1);
	sshell[strlen(sshell) - 1] = '\0';
	int length = strlen(sshell);
	/*
	//Write the pipe with command
	write(in_pipe[1], sshell, length);
	write(in_pipe[1], "\n", 1);
	write(in_pipe[1], "exit", 4);
	write(in_pipe[1], "\n", 1);
	close(in_pipe[1]);	
	*/
	//dup2(in_pipe[1],1);
	//close(in_pipe[1]);	
	//dup2(out_pipe[1],1);
	//close(out_pipe[1]);	
	int ret = fork(); // Fork to see what process is going on
	//dup2( in_pipe[1], 1);	
	//close( in_pipe[1]);
	if (ret < 0) {
		perror("fork");
		exit(1); // Forking error, exit
	}
	else if(ret == 0) {
		c->_currentSimpleCommand = new SimpleCommand();
		
		dup2(in_pipe[1],1);
		close(in_pipe[1]);	
		char * param = strtok(sshell, " ");
		while(param) {
			c->_currentSimpleCommand->insertArgument(strdup(param));
			param = strtok(NULL, " ");
		}
		c->_currentCommand.insertSimpleCommand(c->_currentSimpleCommand);
		c->_currentCommand.execute();		
		_exit(1);
	}
		waitpid(ret, NULL, 0);
		/*for(int i = 0; i < SIZE; i++) {
			check[i] = 0;
		}*/
		read(in_pipe[0], check, SIZE);	
		int count = 0;
		/*while(read(in_pipe[0], &read_pipe, 1)) { //Read pipe
			if(read_pipe != '\n') {
				check[count] = read_pipe;
			}
			else {
				check[count] = ' ';
			}	
			count++;
		}*/
		
		//check[count] = '\0';
		
		//int len = 0;
		/*while(check[len] != '\0') {
			len++;
		}*/
		int len = strlen(check);	
		len--;
		while(len>=0) {
			if(check[len] != '\n') {
				myunputc(check[len]);
			}
			else {
				myunputc(' ');
			}
			len--;
		}	
		
		//check[len] = '\0';
		//Restore and close standard input, output and error
		//close(out_pipe[0]);
		//dup2(in, 0);
		//dup2(out, 1);
		//	dup2(err, 2);
		//close(in);
		//close(out);	
		//close(err);
		
		close(in_pipe[0]);
		yylval.string_val = strdup(sshell);
		return WORD;
}

[^ \t\n<>|&][^ \t\n<>|&]*  {
  /* Assume that file names have only alpha chars */
  yylval.string_val = strdup(yytext);
  return WORD;
}
