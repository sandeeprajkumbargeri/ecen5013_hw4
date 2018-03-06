/*  Author: Sandeep Raj Kumbargeri <sandeep.kumbargeri@colorado.edu>
    Date: 5-March-2018
    Description: A program to demonstrate the implementation of UNIX sockets in Linux.
                 The parent ptocess creates a child process and communicates a structure using UNIX based sockets.
                 The child receives the data, modifies it and sends the data back to the parent process.

    Written for ECEN 5013 at University of Colorado Boulder in Spring 2018.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <ctype.h>
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
  int parent_sock = 0, child_sock = 0;
  payload_t data;
  char *parent_sockaddr_path = "/tmp/parent_sockaddr";
  char *child_sockaddr_path = "/tmp/child_sockaddr";
  struct sockaddr_un parent_sockaddr, child_sockaddr;
  socklen_t parent_sockaddr_length, child_sockaddr_length;

  bzero(&data, sizeof(payload_t));
  remove(parent_sockaddr_path);
  remove(child_sockaddr_path);

  printf("## PARENT ## Forking child process.\n");

  switch (Child_Pid = fork())
  {
    case -1: /* fork() failed */
      errExit("fork");
      break;

    case 0: /* Child of successful fork() comes here */
      printf("## CHILD ## Forked.\n");

      child_sock = socket(AF_UNIX, SOCK_DGRAM, 0);     //create a datagram socket

      if(child_sock == -1)
        errExit("creation child_sock");

      parent_sockaddr_length = sizeof(parent_sockaddr);
      child_sockaddr_length = sizeof(child_sockaddr);
      bzero(&parent_sockaddr, parent_sockaddr_length);
      bzero(&child_sockaddr, child_sockaddr_length);

      child_sockaddr.sun_family = AF_UNIX;
      strncpy(child_sockaddr.sun_path, child_sockaddr_path, sizeof(child_sockaddr.sun_path) -1);

      if(bind(child_sock, (struct sockaddr *) &child_sockaddr, sizeof(struct sockaddr_un)) == -1)
        errExit("bind child");

      printf("## CHILD ## Created socket and bind successful at \"%s\".\n", child_sockaddr_path);

      if(recvfrom(child_sock, (void *) &data, sizeof(payload_t), 0, (struct sockaddr *) &parent_sockaddr, &parent_sockaddr_length) == -1)
        errExit("receive parent to child");

      printf("## CHILD ## Received from \"%s\".\n", parent_sockaddr.sun_path);

      printf("## CHILD ## Received string: \"%s\". Received LED State: %s.\n", data.string, data.led_state ? "true" : "false");

      strcat(data.string, " World");
      data.led_state = !data.led_state;
      printf("## CHILD ## Sending modified string: \"%s\". Modified Received LED State: %s.\n", data.string, data.led_state ? "true" : "false");

      if(sendto(child_sock, (const void *) &data, sizeof(payload_t), 0, (const struct sockaddr *) &parent_sockaddr, parent_sockaddr_length) == -1)
        errExit("sending from child to parent");

        if(close(child_sock == -1))        //close child socket
          errExit("close child_sock");

        remove(child_sockaddr_path);
        printf("## CHILD ## Communication successful. Closed child_sock.\n");

      break;

    default: /* Parent comes here after successful fork() */
      parent_sock = socket(AF_UNIX, SOCK_DGRAM, 0);     //create a datagram socket

      if(parent_sock == -1)
        errExit("creation parent_sock");

      parent_sockaddr_length = sizeof(parent_sockaddr);
      child_sockaddr_length = sizeof(child_sockaddr);
      bzero(&parent_sockaddr, parent_sockaddr_length);
      bzero(&child_sockaddr, child_sockaddr_length);

      parent_sockaddr.sun_family = AF_UNIX;
      strncpy(parent_sockaddr.sun_path, parent_sockaddr_path, sizeof(parent_sockaddr.sun_path) -1);

      child_sockaddr.sun_family = AF_UNIX;
      strncpy(child_sockaddr.sun_path, child_sockaddr_path, sizeof(child_sockaddr.sun_path) -1);

      if(bind(parent_sock, (struct sockaddr *) &parent_sockaddr, sizeof(struct sockaddr_un)) == -1)
        errExit("bind parent");

      printf("## PARENT ## Created socket and bind successful at \"%s\".\n", parent_sockaddr_path);

      strcpy(data.string, "Hello");
      data.led_state = false;

      printf("## PARENT ## Sleeping for 1 second to make sure child has successfully set up its socket.\n");
      sleep(1);

      printf("## PARENT ## Sending string: \"%s\". LED State: %s.\n", data.string, data.led_state ? "true" : "false");

      if(sendto(parent_sock, (const void *) &data, sizeof(payload_t), 0, (const struct sockaddr *) &child_sockaddr, child_sockaddr_length) == -1)
        errExit("send parent to child");

      bzero(&data, sizeof(payload_t));

      if(recvfrom(parent_sock, (void *) &data, sizeof(payload_t), 0, (struct sockaddr *) &child_sockaddr, &child_sockaddr_length) == -1)
        errExit("receive from child to parent");

      printf("## PARENT ## Received string: \"%s\". Received LED State: %s.\n", data.string, data.led_state ? "true" : "false");

      if(close(parent_sock == -1))        //close parent socket
        errExit("close parent_sock");

      remove(parent_sockaddr_path);
      printf("## PARENT ## Communication successful. Closed parent_sock.\n");

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
