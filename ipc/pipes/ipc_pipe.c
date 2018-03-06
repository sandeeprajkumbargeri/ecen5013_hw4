/*  Author: Sandeep Raj Kumbargeri <sandeep.kumbargeri@colorado.edu>
    Date: 5-March-2018
    Description: A program to demonstrate the implementation of pipes in UNIX/Linux.
                 The parent ptocess creates a child process and communicates a structure using pipes.
                 The child receives the data, modifies it and sends the data back to the parent process.

    To Build:    gcc -o ipc_pipe ipc_pipe.c

    Written for ECEN 5013 at University of Colorado Boulder in Spring 2018.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/wait.h>
#include <stdbool.h>

//Structure of the data which is communicated between the parent and the child using pipes.
typedef struct payload
{
  char string[16];
  bool led_state;
} payload_t;

void errExit(char *);

int main(void)
{
  pid_t Child_Pid = 0;
  payload_t data;

  int parent_to_child[2];     //pipe file descriptor for sending data from parent ptocess to child process.
  int child_to_parent[2];     //pipe file descriptor for sending data from child ptocess to parent process.

/*In the above file descriptors, element [0] is meant and used the reading end of the pipe while element [1] is for writing.*/

  if(pipe(parent_to_child) == -1)
    errExit("pipe parent_to_child");

  if(pipe(child_to_parent) == -1)
    errExit("pipe child_to_parent");

  printf("## PARENT ## Forking child process.\n");

  switch (Child_Pid = fork())
  {
    case -1: /* fork() failed */
      errExit("fork");
      break;

    case 0: /* Child of successful fork() comes here */
      printf("## CHILD ## Forked.\n");
      if(close(child_to_parent[0]) == -1)      //close child_to_parent read
        errExit("close child_to_parent read");

      if(close(parent_to_child[1]) == -1)      //close parent_to_child write
        errExit("close parent_to_child write");
      printf("## CHILD ## Handled pipe descriptors.\n");

      if(read(parent_to_child[0], &data, sizeof(payload_t)) == -1)
        errExit("read parent_to_child");

      printf("## CHILD ## Received string: \"%s\". Received LED State: %s.\n", data.string, data.led_state ? "true" : "false");

      if(close(parent_to_child[0]) == -1)        //close parent_to_child read
        errExit("close parent_to_child read");
      printf("## CHILD ## Closed read end of parent_to_child.\n");

      strcat(data.string, " World");
      data.led_state = !data.led_state;

      printf("## CHILD ## Piping modified string: \"%s\". Modified Received LED State: %s.\n", data.string, data.led_state ? "true" : "false");
      if(write(child_to_parent[1], &data, sizeof(payload_t)) == -1)
        errExit("write child_to_parent");

      if(close(child_to_parent[1]) == -1)        //close child_to_parent write
        errExit("close child_to_parent write");
      printf("## CHILD ## Piping successful. Closed write end of child_to_parent.\n");

      break;

    default: /* Parent comes here after successful fork() */
      if(close(child_to_parent[1]) == -1)        //close child_to_parent write
        errExit("close child_to_parent write");

      if(close(parent_to_child[0]) == -1)        //close parent_to_child read
        errExit("close parent_to_child read");
      printf("## PARENT ## Handled pipe descriptors.\n");

      strcpy(data.string, "Hello");
      data.led_state = false;

      printf("## PARENT ## Piping string: \"%s\". LED State: %s.\n", data.string, data.led_state ? "true" : "false");

      if(write(parent_to_child[1], &data, sizeof(payload_t)) == -1)
        errExit("write parent_to_child");

      if(close(parent_to_child[1]) == -1)        //close parent_to_child write
        errExit("close parent_to_child write");
      printf("## PARENT ## Piping successful. Closed write end of parent_to_child.\n");

      if(read(child_to_parent[0], &data, sizeof(payload_t)) == -1)
        errExit("read child_to_parent");

      printf("## PARENT ## Received string: \"%s\". Received LED State: %s.\n", data.string, data.led_state ? "true" : "false");

      if(close(child_to_parent[0]) == -1)        //close child_to_parent read
        errExit("close child_to_parent read");
      printf("## PARENT ## Piping successful. Closed read end of child_to_parent.\n");

      break;
  }

  exit(EXIT_SUCCESS);

}

//Function to print the error to STDOUT and exit.
void errExit(char *strError)
{
  perror(strError);
  exit(EXIT_FAILURE);
}
