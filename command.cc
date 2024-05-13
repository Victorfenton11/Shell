/*
 * CS252: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 * DO NOT PUT THIS PROJECT IN A PUBLIC REPOSITORY LIKE GIT. IF YOU WANT 
 * TO MAKE IT PUBLICALLY AVAILABLE YOU NEED TO REMOVE ANY SKELETON CODE 
 * AND REWRITE YOUR PROJECT SO IT IMPLEMENTS FUNCTIONALITY DIFFERENT THAN
 * WHAT IS SPECIFIED IN THE HANDOUT. WE OFTEN REUSE PART OF THE PROJECTS FROM  
 * SEMESTER TO SEMESTER AND PUTTING YOUR CODE IN A PUBLIC REPOSITORY
 * MAY FACILITATE ACADEMIC DISHONESTY.
 */

#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <string>
#include <string.h>

#include <iostream>

#include "command.hh"
#include "shell.hh"
extern char ** environ;


Command::Command() {
    // Initialize a new vector of Simple Commands
    _simpleCommands = std::vector<SimpleCommand *>();

    _outFile = NULL;
    _inFile = NULL;
    _errFile = NULL;
    _background = false;
    _append = false;
}

void Command::insertSimpleCommand( SimpleCommand * simpleCommand ) {
    // add the simple command to the vector
    _simpleCommands.push_back(simpleCommand);
}

void Command::clear() {
    // deallocate all the simple commands in the command vector
    for (auto simpleCommand : _simpleCommands) {
        delete simpleCommand;
    }

    // remove all references to the simple commands we've deallocated
    // (basically just sets the size to 0)
    _simpleCommands.clear();

    if ( _outFile && _outFile != _errFile) {
        delete _outFile;
    }
    _outFile = NULL;

    if ( _inFile ) {
        delete _inFile;
    }
    _inFile = NULL;

    if ( _errFile ) {
        delete _errFile;
    }
    _errFile = NULL;

    _background = false;
    _append = false;
}

void Command::print() {
    printf("\n\n");
    printf("              COMMAND TABLE                \n");
    printf("\n");
    printf("  #   Simple Commands\n");
    printf("  --- ----------------------------------------------------------\n");

    int i = 0;
    // iterate over the simple commands and print them nicely
    for ( auto & simpleCommand : _simpleCommands ) {
        printf("  %-3d ", i++ );
        simpleCommand->print();
    }

    printf( "\n\n" );
    printf( "  Output       Input        Error        Background\n" );
    printf( "  ------------ ------------ ------------ ------------\n" );
    printf( "  %-12s %-12s %-12s %-12s\n",
            _outFile?_outFile->c_str():"default",
            _inFile?_inFile->c_str():"default",
            _errFile?_errFile->c_str():"default",
            _background?"YES":"NO");
    printf( "\n\n" );
}

void Command::execute() {
    // Don't do anything if there are no simple commands
    if ( _simpleCommands.size() == 0 ) {
        Shell::prompt();
        return;
    }

    // Print contents of Command data structure
    //print();

    if (strcasecmp(_simpleCommands[0]->_arguments[0]->c_str(), "exit") == 0) {
      printf("Good bye!!\n");
      exit(0);
    }

    // Add execution here
    int defaultin = dup(0);
    int defaultout = dup(1);
    int defaulterr = dup(2);

    // Set the initial input
    int cmdinput;
    if (_inFile) {
      cmdinput = open(_inFile->c_str(), O_RDONLY);
      if (cmdinput < 0) {
        perror("open");
        exit(1);
      }
    } else {
      cmdinput = dup(defaultin);
    }

    unsigned int numCommands = _simpleCommands.size();
    int cmdoutput;
    int cmderr;
    int pid;
    // For every simple command fork a new process except the last one
    for (unsigned int i = 0; i < numCommands; i++) {
      // setenv
      if (strcmp(_simpleCommands[i]->_arguments[0]->c_str(), "setenv") == 0) {
        int err = setenv(_simpleCommands[i]->_arguments[1]->c_str(), _simpleCommands[i]->_arguments[2]->c_str(), 1);
        if (err != 0) {
          perror("Error: setenv");
        }
        clear();
        Shell::prompt();
        return;
      }

      // unsetenv
      if (strcmp(_simpleCommands[i]->_arguments[0]->c_str(), "unsetenv") == 0) {
        for (unsigned int i = 1; i < _simpleCommands[i]->_arguments.size(); i++) {
          int err = unsetenv(_simpleCommands[i]->_arguments[i]->c_str());
          if (err != 0) {
            perror("Error: unsetenv");
          }
        }
        clear();
        Shell::prompt();
        return;
      }

      // cd
      if (strcmp(_simpleCommands[i]->_arguments[0]->c_str(), "cd") == 0) {
        if (_simpleCommands[i]->_arguments.size() > 2) {
          perror("cd: too many arguments");
          Shell::prompt();
          return;
        }
        int err;
        if (!_simpleCommands[i]->_arguments[1] || strcmp(_simpleCommands[i]->_arguments[1]->c_str(), "${HOME}") == 0) {
          err = chdir(getenv("HOME"));
        } else {
          err = chdir(_simpleCommands[i]->_arguments[1]->c_str());
        }

        if (err != 0) {
          fprintf(stderr, "cd: can't cd to %s\n", _simpleCommands[i]->_arguments[1]->c_str());
        }

        clear();
        Shell::prompt();
        return;
      }

      // redirect input
      dup2(cmdinput, 0);
      close(cmdinput);

      // setup output and error
      if (i == numCommands-1) {
        // last simple command
        if (!_outFile) {
          cmdoutput = dup( defaultout );
        } else {
          if (_append) {
            cmdoutput = open(_outFile->c_str(), O_CREAT|O_WRONLY|O_APPEND, 0664);
          } else {
            cmdoutput = open(_outFile->c_str(), O_CREAT|O_WRONLY|O_TRUNC, 0664);
          }

          if (cmdoutput < 0) {
            perror("open");
            exit(1);
          }
        }

        if (!_errFile) {
          cmderr = dup( defaulterr );
        } else {
          if (_append) {
            cmderr = open(_errFile->c_str(), O_CREAT|O_WRONLY|O_APPEND, 0664);
          } else {
            cmderr = open(_errFile->c_str(), O_CREAT|O_WRONLY|O_TRUNC, 0664);
          }

          if (cmderr < 0) {
            perror("open");
            exit(1);
          }
        }

        dup2(cmderr, 2);
        close(cmderr);

        // Environment Variable Expansion: "_"
        char *lastArg = strdup(_simpleCommands[i]->_arguments[_simpleCommands[i]->_arguments.size() - 1]->c_str());
        setenv("_", lastArg, 1);
        free(lastArg);
        lastArg = NULL;

      } else { // not last simple command
        int fdpipe[2]; // Create new pipe
        pipe(fdpipe);
        if ( pipe(fdpipe) == -1 ) {
          perror("pipe");
          exit(2);
        }
        cmdinput = fdpipe[0];
        cmdoutput = fdpipe[1];
      }

      // Redirect output and error
      dup2(cmdoutput, 1);
      close(cmdoutput);

      pid = fork();
      char **args = NULL;
      if (pid == 0) {
        // printenv
        if (strcmp(_simpleCommands[i]->_arguments[0]->c_str(), "printenv") == 0) {
          int e = 0;
          while (environ[e] != NULL) {
            printf("%s\n", environ[e++]);
          }
          clear();
          Shell::prompt();
          return;
        }

        // Child process
        unsigned int numArguments = _simpleCommands[i]->_arguments.size();
        args = (char **) malloc((numArguments + 1) * sizeof(char *));
        for (unsigned int j = 0; j < numArguments; j++) {
          args[j] = (char *) _simpleCommands[i]->_arguments[j]->c_str();
        }
        args[numArguments] = NULL;
        // Execute command
        execvp(_simpleCommands[i]->_arguments[0]->c_str(), args);
        // Execvp failed
        perror("execvp");
        exit(1);
      } else if (pid < 0) {
        // There was an error in fork
        perror("fork");
        return;
      }
      if (args != NULL) {
        free(args);
        args = NULL;
      }
    }

    // restore in/out/err defaults
    dup2(defaultin, 0);
    dup2(defaultout, 1);
    dup2(defaulterr, 2);
    close(defaultin);
    close(defaultout);
    close(defaulterr);

    if (_background == false) {
      int exitStatus;
      waitpid(pid, &exitStatus, 0);
      std::string lastExec = std::to_string(WEXITSTATUS(exitStatus));
      setenv("?", lastExec.c_str(), 1);
    } else {
      std::string lastPID = std::to_string(pid);
      setenv("!", lastPID.c_str(), 1);
    }

    // Clear to prepare for next command
    clear();

    // Print new prompt
    Shell::prompt();
}

SimpleCommand * Command::_currentSimpleCommand;
