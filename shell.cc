#include "command.hh"
int yyparse(void);
extern void ctrl(int);
extern void ctrlZ(int);
extern void zombieElimination(int);
int main() {
	signal(SIGINT, ctrl);
	signal(SIGTSTP, ctrlZ);
	signal(SIGCHLD, zombieElimination);
	Command::_currentCommand.prompt();
	yyparse();
}
