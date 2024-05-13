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
 *
 * CS-252 
 * shell.l: lexical analyzer for shell
 * You have to extend it.
 *
 */

%{

#include <stdio.h>
#include <cstring>
#include <sys/types.h>
#include <unistd.h>
#include "y.tab.hh"
#include <pwd.h>
#include <string.h>

extern "C" char * read_line();

int mygetc(FILE * f) {
  static char *p;
  char ch;

  if (!isatty(0)) {
    return getc(f);
  }

  if (p==NULL || *p == 0) {
    char *s = read_line();
    p = s;
  }

  ch = *p;
  p++;

  return ch;
}

#undef getc
#define getc(f) mygetc(f)

static  void yyunput (int c,char *buf_ptr  );

void myunputc(int c) {
  unput(c);
}

%}

%option noyywrap

%x src

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

">&" {
  return GREATAMPERSAND;
}

">>" {
  return GREATGREAT;
}

">>&" {
  return GREATGREATAMPERSAND;
}

"2>" {
  return TWOGREAT;
}

"&" {
  return AMPERSAND;
}

"<" {
  return LESS;
}

"|" {
  return PIPE;
}

source {
  BEGIN(src);
}

<src>[ \t]*
<src>[^ \t\n<>|&]+ {
  // Source
  FILE *fp = fopen(yytext, "r+");
  if (fp == NULL) {
    perror("source");
    BEGIN(INITIAL);
  } else {
    fputc('\n', fp);
    yypush_buffer_state(yy_create_buffer(fp, YY_BUF_SIZE));
    BEGIN(INITIAL);
    yyparse();
    yypop_buffer_state();
    fclose(fp);
  }
}

\$\{[^ \n\}]*\}([^ \t\n<>\|&]*|\$\{[^ \n\}]*\})* {
  std::string str = yytext;
  std::string result = "";
  while (str.length() > 0) {
    int index = str.find("${");
    if (index < 0) {
      result = result + str;
      break;
    } else if (index > 0) {
      result = result + str.substr(0, index);
      str = str.substr(index);
    } else {
      int close = str.find("}");
      std::string parsed = str.substr(2, close - 2);
      char *envname = (char *) parsed.c_str();
      char *envval = getenv(envname);

      if (envval != NULL) {
        result = result + std::string(envval);
      }
      str = str.substr(close + 1);
    }
  }

  yylval.cpp_string = new std::string(result);
  return WORD;
}

~[^ \t\n<>\|&]* {
  // Tilde Expansion
  std::string str = yytext;
  std::string thisHome = getenv("HOME");
  if (str.length() == 1) {
    yylval.cpp_string = new std::string(thisHome);
  } else {
    int pathStart = str.find("/");
    std::string usr;
    if (pathStart > 0) {
      usr = str.substr(1, pathStart - 1);
      std::string usrPath = std::string(getpwnam(usr.c_str())->pw_dir) + str.substr(pathStart);
      yylval.cpp_string = new std::string(usrPath);
    } else {
      usr = str.substr(1);
      yylval.cpp_string = new std::string(getpwnam(usr.c_str())->pw_dir);
    }
  }

  return WORD;
}

\$\([^\n]*\) {
  // Subshells
  std::string str = yytext;
  std::string parsed = str.substr(2, str.length() - 3) + "\n";
  char *subcommand = (char *) parsed.c_str();

  int defaultin = dup(0);
  int defaultout = dup(1);

  int inpipe[2];
  pipe(inpipe);

  int outpipe[2];
  pipe(outpipe);

  write(inpipe[1], subcommand, strlen(subcommand));
  close(inpipe[1]);
  dup2(inpipe[0], 0);
  close(inpipe[0]);

  dup2(outpipe[1], 1);
  close(outpipe[1]);

  int pid = fork();

  if (pid == 0) {
    execvp("/proc/self/exe", NULL);
    perror("execvp");
    exit(1);
  } else if (pid < 0) {
    perror("fork");
    exit(1);
  }

  dup2(defaultin, 0);
  dup2(defaultout, 1);
  close(defaultin);
  close(defaultout);

  char *buffer = (char *) malloc(sizeof(char) * 10000);
  char current;
  int n = 0;

  while (read(outpipe[0], &current, 1)) {
    if (current != '\n') {
      buffer[n++] = current;
    } else {
      buffer[n++] = ' ';
    }
  }
  buffer[n] = '\0';
  close(outpipe[0]);

  for (int m = n - 1; m >= 0; m--) {
    myunputc(buffer[m]);
  }
  free(buffer);
  buffer = NULL;
}

\".*\" {
  std::string str = std::string(yytext);
  str = str.substr(1, str.length()-2);
  yylval.cpp_string = new std::string(str);
  return WORD;
}

[^ \t\n<>\|&]*\\[^ \t\n]* {
  std::string str = std::string(yytext);
  int length = str.length();
  char *escaped = (char *) malloc(sizeof(char) * (length - 1));
  int j = 0;
  for (int i = 0; i < length - 1; i++) {
    if (str[j] == '\\') {
      if (str[j+1] == '\\') {
        escaped[i] = '\\';
        j++;
      } else {
        i--;
      }
      j++;
      continue;
    }
    escaped[i] = str[j];
    j++;
  }
  escaped[length - 1] = '\0';
  yylval.cpp_string = new std::string(escaped);
  free(escaped);
  escaped = NULL;
  return WORD;
}


[^ \t\n<>\|&][^ \t\n<>\|&]*  {
  /* Assume that file names have only alpha chars */
  yylval.cpp_string = new std::string(yytext);
  return WORD;
}

. {
  return NOTOKEN;
}

%%
