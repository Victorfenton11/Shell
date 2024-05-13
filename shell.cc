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
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#include "shell.hh"

int yyparse(void);
void yyrestart(FILE *);

void Shell::prompt() {
  if (isatty(0)) {
    char *prompt = getenv("PROMPT");
    if (prompt != NULL) printf("%s", prompt);
    else printf("myshell>");
  }
  fflush(stdout);
}

void sigIntHandler(int sig) {
  (void) sig;
  printf("\n");
  Shell::prompt();
}

void sigHandlerZombieCleanup(int sig) {
  (void) sig;
  int pid = waitpid(-1, NULL, WNOHANG);
  while (pid > 0) {
    //printf("%d exited.\n", pid);
    pid = waitpid(-1, NULL, WNOHANG);
  }
}

int main(int argc, char **argv) {
  (void) argc;
  struct sigaction signalAction;
  signalAction.sa_handler = sigIntHandler;
  sigemptyset(&signalAction.sa_mask);
  signalAction.sa_flags = SA_RESTART;
  int error = sigaction(SIGINT, &signalAction, NULL);
  if ( error ) {
    perror("sigaction");
    exit(-1);
  }

  struct sigaction signalAction2;
  signalAction2.sa_handler = sigHandlerZombieCleanup;
  sigemptyset(&signalAction2.sa_mask);
  signalAction2.sa_flags = SA_RESTART;
  //TODO sigaction for SIGCHLD zombie processe
  error = sigaction(SIGCHLD, &signalAction2, NULL);

  // Environment Variable Expansion: "$" and "SHELL"
  std::string shellPID = std::to_string(getpid());
  setenv("$", shellPID.c_str(), 1);

  char shellPath[1000];
  realpath(argv[0], shellPath);
  setenv("SHELL", shellPath, 1);

  FILE *fp = fopen(".shellrc", "r");
  if (fp != NULL) {
    yyrestart(fp);
    yyparse();
    yyrestart(stdin);
    fclose(fp);
  } else {
    Shell::prompt();
  }

  yyparse();
}

Command Shell::_currentCommand;
