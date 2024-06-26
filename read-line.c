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
 * CS252: Systems Programming
 * Purdue University
 * Example that shows how to read one line with simple editing
 * using raw terminal.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>

#define MAX_BUFFER_LINE 2048
#define HISTORY_SIZE 16

extern void tty_raw_mode(void);
extern void tty_reset(void);

// Buffer where line is stored
int line_length;
int remaining_length;
char line_buffer[MAX_BUFFER_LINE];
char remaining_buffer[MAX_BUFFER_LINE];

// Simple history array
// This history does not change. 
// Yours have to be updated.
int history_index = 0;
int reverse_index = 0;
char *history[HISTORY_SIZE];
int history_length = HISTORY_SIZE;
int maxCapacity = 0;

void read_line_print_usage()
{
  char * usage = "\n"
    " ctrl-?       Print usage\n"
    " ctrl-A       The cursor moves to the beginning of the line\n"
    " ctrl-D       Remove character at the cursor\n"
    " ctrl-E       The cursor moves to the end of the line\n"
    " ctrl-H       Removes the character at the position before the cursor\n"
    " L/R arrows   Move the cursor to the Left/Right\n"
    " Backspace    Deletes last character\n"
    " up arrow     See last command in the history\n";

  write(1, usage, strlen(usage));
}

/* 
 * Input a line with some basic editing.
 */
char * read_line() {

  // Set terminal in raw mode
  tty_raw_mode();

  line_length = 0;
  remaining_length = 0;

  // Read one line until enter is typed
  while (1) {

    // Read one character in raw mode.
    char ch;
    read(0, &ch, 1);

    if (ch>=32 && ch != 127) {
      // It is a printable character. 

      // Do echo
      write(1,&ch,1);

      // If max number of character reached return.
      if (line_length==MAX_BUFFER_LINE-2) break;

      // add char to buffer.
      line_buffer[line_length]=ch;
      line_length++;

      if (remaining_length) {
        for (int i = remaining_length - 1; i >= 0; i--) {
          char chr = remaining_buffer[i];
          write(1,&chr,1);
        }
      }

      char cl = 8;
      for (int j = 0; j < remaining_length; j++) {
        write(1,&cl,1);
      }
    } else if (ch==10) {
      // <Enter> was typed. Return line
      if (remaining_length > 0) {
        for (int i = remaining_length - 1; i >= 0; i--) {
          char chr = remaining_buffer[i];

          line_buffer[line_length++] = chr;
        }
      }

      if (line_length > 0) {
        if (history[history_index] == NULL) {
          history[history_index] = (char *) malloc(MAX_BUFFER_LINE);
        }

        strcpy(history[history_index], line_buffer);
        reverse_index = history_index;
        history_index++;
        if (history_index >= history_length) {
          history_index = 0;
          maxCapacity = 1;
        }
      }

      remaining_length = 0;

      // Print newline
      write(1,&ch,1);
      break;
    } else if (ch == 31) {
      // ctrl-?
      read_line_print_usage();
      line_buffer[0]=0;
      break;
    } else if (ch == 8 || ch == 127) {
      // <backspace> was typed. Remove previous character read.
      if (line_length == 0) continue;

      // Go back one character
      ch = 8;
      write(1,&ch,1);

      for (int i = remaining_length - 1; i >= 0; i--) {
        char chr = remaining_buffer[i];
        write(1,&chr,1);
      }

      // Write a space to erase the last character read
      ch = ' ';
      write(1,&ch,1);

      char cl = 8;
      for (int j = 0; j < remaining_length+1; j++) {
        write(1,&cl,1);
      }

      // Remove one character from buffer
      line_length--;
    }
    else if (ch==27) {
      // Escape sequence. Read two chars more
      //
      // HINT: Use the program "keyboard-example" to
      // see the ascii code for the different chars typed.
      //
      char ch1;
      char ch2;
      read(0, &ch1, 1);
      read(0, &ch2, 1);
      if (ch1==91 && ch2==65) {
        // Up arrow. Print next line in history.
 
        // Erase old line
        // Print backspaces
        int i = 0;
        ch = 8;
        for (i = 0; i < line_length; i++) {
          write(1,&ch,1);
        }

        // Print spaces on top
        ch = ' ';
        for (i = 0; i < line_length + remaining_length; i++) {
          write(1,&ch,1);
        }

        // Print backspaces
        ch = 8;
        for (i =0; i < line_length + remaining_length; i++) {
          write(1,&ch,1);
        }

        // Copy line from history
        strcpy(line_buffer, history[reverse_index]);
        line_length = strlen(line_buffer);
        int indx = maxCapacity ? history_length : history_index;
        int add = (ch2 == 65) ? -1 : 1;
        reverse_index=(reverse_index+add)%indx;
        if (reverse_index == -1) reverse_index = indx - 1;

        // echo line
        write(1, line_buffer, line_length);

        remaining_length = 0;
      } else if (ch1 == 91 && ch2 == 68) {
        // Left arrow key
        if (line_length == 0) continue;
        ch = 8;
        write(1,&ch,1);
        remaining_buffer[remaining_length++] = line_buffer[line_length-1];
        line_length--;
      } else if (ch1 == 91 && ch2 == 67) {
        // Right arrow key
        if (remaining_length == 0) continue;
        write(1,"\033[1C", 5);
        line_buffer[line_length++] = remaining_buffer[remaining_length - 1];
        remaining_length--;
      }
    } else if (ch == 1) {
      // ctrl-A
      int max = line_length;
      char cl = 8;
      for (int i = 0; i < max; i++) {
        write(1,&cl,1);
        remaining_buffer[remaining_length++] = line_buffer[line_length - 1];
        line_length--;
      }
    } else if (ch == 4) {
      // ctrl-D
      if (line_length == 0 || remaining_length == 0) continue;

      for (int i = remaining_length - 2; i >= 0; i--) {
        char chr = remaining_buffer[i];
        write(1,&chr,1);
      }

      ch = ' ';
      write(1,&ch,1);

      char cl = 8;
      for (int j = 0; j < remaining_length; j++) {
        write(1,&cl,1);
      }

      remaining_length--;
    } else if (ch == 5) {
      // ctrl-E
      for (int i = remaining_length - 1; i >= 0; i--) {
        write(1,"\033[1C",5);
        line_buffer[line_length++] = remaining_buffer[remaining_length - 1];
        remaining_length--;
      }
    }
  }

  // Add eol and null char at the end of string
  line_buffer[line_length]=10;
  line_length++;
  line_buffer[line_length]=0;

  tty_reset();

  return line_buffer;
}

