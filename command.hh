#ifndef command_h
#define command_h

#include "simpleCommand.hh"
#include <signal.h>
// Command Data Structure

struct Command {
  int _numOfAvailableSimpleCommands;
  int _numOfSimpleCommands;
  SimpleCommand ** _simpleCommands;
  char * _outFile;
  char * _inFile;
  char * _errFile;
  
  int _background;
  int _append; 
  
  int _in;
  int _out;
  
  void prompt();
  void print();
  void execute();
  void clear();

  Command();
  void insertSimpleCommand( SimpleCommand * simpleCommand );

  static Command _currentCommand;
  static SimpleCommand *_currentSimpleCommand;
};

#endif
