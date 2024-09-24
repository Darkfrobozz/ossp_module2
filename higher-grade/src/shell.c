#include "parser.h"    // cmd_t, position_t, parse_commands()

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <fcntl.h>     //fcntl(), F_GETFL
#include <sys/wait.h> // wait()

#define READ  0
#define WRITE 1

/**
 * For simplicitiy we use a global array to store data of each command in a
 * command pipeline .
 */
cmd_t commands[MAX_COMMANDS];

/**
 *  Debug printout of the commands array.
 */
void print_commands(int n) {
  for (int i = 0; i < n; i++) {
    printf("==> commands[%d]\n", i);
    printf("  pos = %s\n", position_to_string(commands[i].pos));
    printf("  in  = %d\n", commands[i].in);
    printf("  out = %d\n", commands[i].out);

    print_argv(commands[i].argv);
  }

}

/**
 * Returns true if file descriptor fd is open. Otherwise returns false.
 */
int is_open(int fd) {
  return fcntl(fd, F_GETFL) != -1 || errno != EBADF;
}

void fork_error() {
  perror("fork() failed)");
  exit(EXIT_FAILURE);
}

/**
 *  Fork a proccess for command with index i in the command pipeline. If needed,
 *  create a new pipe and update the in and out members for the command..
 */
void fork_cmd(int i, int left[2], int right[2]) {
  pid_t pid;

  switch (pid = fork()) {
    case -1:
      fork_error();
    case 0:
      // Close unnecessary pipes
      close(left[WRITE]);
      close(right[READ]);
      // Child process after a successful fork().
      printf("%d", getpid());
      printf("%d", commands[i].pos);
      switch (commands[i].pos)
      {
        case single:
          close(left[READ]);
          close(right[WRITE]);
          /* code */
          break;
        case first:
          close(left[READ]);
          dup2(right[WRITE], STDOUT_FILENO);
          break;
        case middle:
          dup2(left[READ], STDIN_FILENO);
          dup2(right[WRITE], STDOUT_FILENO);
          break;
        case last:
          close(right[WRITE]);
          dup2(left[READ], STDIN_FILENO);
          break;
        
        default:
          break;
      }

      // Execute the command in the contex of the child process.
      execvp(commands[i].argv[0], commands[i].argv);

      // If execvp() succeeds, this code should never be reached.
      fprintf(stderr, "shell: command not found: %s\n", commands[i].argv[0]);
      exit(EXIT_FAILURE);

    default:
      // Parent process after a successful fork().

      break;
  }
}

/**
 *  Fork one child process for each command in the command pipeline.
 */
void fork_commands(int n) {
  // The pipe created in previous loop
  int fd_l[] = {-1, -1};
  // The pipe created in current loop
  int fd_n[] = {-1, -1};
  for (int i = 0; i < n; i++) {
    if (i != n - 1) {
      if(pipe(fd_n) == -1) {
        perror("Failed to create pipe");
        exit(EXIT_FAILURE);
      }
    }

    // make pipe and send it based on 
    printf("%d", i);
    fork_cmd(i, fd_l, fd_n);

    //Close pipe that is leaving
    close(fd_l[0]);
    close(fd_l[1]);
    // move next pipe to last
    fd_l[0] = fd_n[0];
    fd_l[1] = fd_n[1];
  }
}

/**
 *  Reads a command line from the user and stores the string in the provided
 *  buffer.
 */
void get_line(char* buffer, size_t size) {
  getline(&buffer, &size, stdin);
  buffer[strlen(buffer)-1] = '\0';
}

/**
 * Make the parents wait for all the child processes.
 */
void wait_for_all_cmds(int n) {
  int i;
  for (i=0; i<n; i++){
    wait(NULL);
  }
  // Not implemented yet!
}

int main() {
  int n;               // Number of commands in a command pipeline.
  size_t size = 128;   // Max size of a command line string.
  char line[size];     // Buffer for a command line string.


  while(true) {
    printf(" >>> ");

    get_line(line, size);

    n = parse_commands(line, commands);
    print_commands(n);

    fork_commands(n);

    wait_for_all_cmds(n);
  }

  exit(EXIT_SUCCESS);
}
