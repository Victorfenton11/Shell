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

/*
 * CS-252
 * shell.y: parser for shell
 *
 * This parser compiles the following grammar:
 *
 *	cmd [arg]* [> filename]
 *
 * you must extend it to understand the complete shell grammar
 *
 */

%code requires
{
#include <cstdio>
#include <cassert>
#include <stdio.h>
#include "shell.hh"
#include <regex.h>
#include <string>
#include <string.h>
#include <strings.h>
#include <dirent.h>
#include <sys/types.h>
#include <vector>
#include <cstdio>
#include <iostream>
#include <ctype.h>

#if __cplusplus > 199711L
#define register      // Deprecated in C++11 so remove the keyword
#endif
}

%union
{
  char        *string_val;
  // Example of using a c++ type in yacc
  std::string *cpp_string;
}

%token <cpp_string> WORD
%token NOTOKEN AMPERSAND GREAT NEWLINE PIPE LESS TWOGREAT GREATAMPERSAND GREATGREAT GREATGREATAMPERSAND

%{
#define yylex yylex
#include <cstdio>
#include <cassert>
#include <stdio.h>
#include "shell.hh"
#include <regex.h>
#include <string>
#include <string.h>
#include <strings.h>
#include <dirent.h>
#include <sys/types.h>
#include <vector>
#include <cstdio>
#include <algorithm>
#include <iostream>
#include <ctype.h>

#define MAXFILENAME 1024

static std::vector<char *> array = std::vector<char *>();
bool myCompare(char *, char *);
void expandWildcard(char *, char *);

void yyerror(const char * s);
int yylex();

%}

%%

goal:
  command_list
  ;

command_list:
  command_line |
  command_list command_line
  ;

argument:
  WORD {
    //printf("   Yacc: insert argument \"%s\"\n", $1->c_str());
    //expandWildcardsIfNecessary($1);
    if (strchr($1->c_str(), '?') == NULL && strchr($1->c_str(), '*') == NULL) {
      Command::_currentSimpleCommand->insertArgument($1);
    } else {
      char *prefix = (char *) "";
      expandWildcard(prefix, (char *) $1->c_str());
      //sort global array
      std::sort(array.begin(), array.end(), myCompare);
      //add arguments from array
      for (int i = 0; i < array.size(); i++) {
        std::string *p = new std::string(array[i]);
        Command::_currentSimpleCommand->insertArgument(p);
      }
      array.clear();
    }
  }
  ;

arg_list:
  arg_list argument
  | /* can be empty */
  ;

command_word:
  WORD {
    //printf("   Yacc: insert command \"%s\"\n", $1->c_str());
    Command::_currentSimpleCommand = new SimpleCommand();
    Command::_currentSimpleCommand->insertArgument($1);
  }
  ;

cmd_and_args:
  command_word arg_list {
    Shell::_currentCommand.
    insertSimpleCommand( Command::_currentSimpleCommand );
  }
  ;

pipe_list:
  cmd_and_args
  | pipe_list PIPE cmd_and_args
  ;

io_modifier:
  GREATGREAT WORD {
    if (Shell::_currentCommand._outFile) {
      printf("Ambiguous output redirect.\n");
    } else {
      //printf("   Yacc: append output \"%s\"\n", $2->c_str()); 
      Shell::_currentCommand._outFile = $2;
      Shell::_currentCommand._append = true;
    }
  }
  | GREAT WORD {
    if (Shell::_currentCommand._outFile) {
      printf("Ambiguous output redirect.\n");
    } else {
      //printf("   Yacc: insert output \"%s\"\n", $2->c_str());
      Shell::_currentCommand._outFile = $2;
    }
  }
  | GREATGREATAMPERSAND WORD {
    //printf("   Yacc: append output and error \"%s\"\n", $2->c_str());
    if (Shell::_currentCommand._outFile) {
      printf("Ambiguous output redirect.\n");
    } else {
      Shell::_currentCommand._outFile = $2;
    }
    if (Shell::_currentCommand._errFile) {
      printf("Ambiguous error redirect.\n");
    } else {
      Shell::_currentCommand._errFile = $2;
    }
    Shell::_currentCommand._append = true;
  }
  | GREATAMPERSAND WORD {
    //printf("   Yacc: insert output and error \"%s\"\n", $2->c_str());
    if (Shell::_currentCommand._outFile) {
      printf("Ambiguous output redirect.\n");
    } else {
      Shell::_currentCommand._outFile = $2;
    }
    if (Shell::_currentCommand._errFile) {
      printf("Ambiguous error redirect.\n");
    } else {
      Shell::_currentCommand._errFile = $2;
    }
  }
  | TWOGREAT WORD {
    //printf("   Yacc: insert error \"%s\"\n", $2->c_str());
    if (Shell::_currentCommand._errFile) {
      printf("Ambiguous error redirect.\n");
    } else {
      Shell::_currentCommand._errFile = $2;
    }
  }
  | LESS WORD {
    //printf("   Yacc: insert input \"%s\"\n", $2->c_str());
    if (Shell::_currentCommand._inFile) {
      printf("Ambiguous input redirect.\n");
    } else {
      Shell::_currentCommand._inFile = $2;
    }
  }
  ;

io_modifier_list:
  io_modifier_list io_modifier
  | /* empty */
  ;

background_opt:
  AMPERSAND {
    //printf("   Yacc: Run in background\n");
    Shell::_currentCommand._background = true;
  }
  | /* empty */
  ;

command_line:
  simple_command
  ;

simple_command:
  pipe_list io_modifier_list background_opt NEWLINE {
    //printf("   Yacc: Execute command\n");
    Shell::_currentCommand.execute();
  }
  | NEWLINE {
    /* accept empty cmd line */
    Shell::_currentCommand.execute();
  }
  | error NEWLINE { yyerrok; }
  ;

%%

void
yyerror(const char * s)
{
  fprintf(stderr,"%s", s);
}

bool myCompare(char *a, char *b) {
  return strcmp(a, b) < 0;
}

void expandWildcard(char *prefix, char *suffix) {
  if (suffix[0] == 0) {
    //fprintf(stderr, "%s\n", prefix);
    array.push_back(strdup(prefix));
    return;
  }

  char parsedPrefix[MAXFILENAME];
  if (prefix[0] == 0) {
    if (suffix[0] == '/') {
      sprintf(parsedPrefix, "%s/", prefix);
      suffix += 1;
    } else {
      strcpy(parsedPrefix, prefix);
    }
  } else {
    sprintf(parsedPrefix, "%s/", prefix);
  }

  char *s = strchr(suffix, '/');
  char component[MAXFILENAME];
  if (s != NULL) {
    strncpy(component, suffix, s-suffix);
    component[s-suffix] = 0;
    suffix = s + 1;
  } else {
    strcpy(component, suffix);
    suffix = suffix + strlen(suffix);
  }

  char newPrefix[MAXFILENAME];
  if (strchr(component, '?') == NULL && strchr(component, '*') == NULL) {
    //component does not have wildcards
    if (parsedPrefix[0] == 0) {
      strcpy(newPrefix, component);
    } else {
      sprintf(newPrefix, "%s/%s", prefix, component);
    }
    expandWildcard(newPrefix, suffix);
    return;
  }

  //component has wildcards

  char *reg = (char *) malloc(2*strlen(component)+10);
  char *r = reg;
  *r = '^';
  r++;
  int i = 0;
  while (component[i]) {
    if (component[i] == '*') {
      *r = '.';
      r++;
      *r = '*';
      r++;
    } else if (component[i] == '?') {
      *r = '.';
      r++;
    } else if (component[i] == '.') {
      *r = '\\';
      r++;
      *r = '.';
      r++;
    } else {
      *r = component[i];
      r++;
    }
    i++;
  }
  *r = '$';
  r++;
  *r = 0;

  regex_t re;
  int expbuf = regcomp(&re, reg, REG_EXTENDED|REG_NOSUB);

  char *dir;
  if (parsedPrefix[0] == 0) {
    dir = (char *) ".";
  } else {
    dir = parsedPrefix;
  }
  DIR *d = opendir(dir);
  if (d == NULL) return;

  struct dirent *ent;
  int flag = 0;
  while ((ent = readdir(d)) != NULL) {
    if (regexec(&re, ent->d_name, 1, NULL, 0) == 0) {
      flag = 1;
      if (parsedPrefix[0] == 0) strcpy(newPrefix, ent->d_name);
      else sprintf(newPrefix, "%s/%s", prefix, ent->d_name);

      if (reg[1] == '.') {
        if (ent->d_name[0] != '.') expandWildcard(newPrefix, suffix);
      } else {
        expandWildcard(newPrefix, suffix);
      }
    }
  }

  if (flag == 0) {
    if (parsedPrefix[0] == 0) {
      strcpy(newPrefix, component);
    } else {
      sprintf(newPrefix, "%s/%s", prefix, component);
    }
    expandWildcard(newPrefix, suffix);
  }

  closedir(d);
  regfree(&re);
  free(reg);
}

#if 0
main()
{
  yyparse();
}
#endif
