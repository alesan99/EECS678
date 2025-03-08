/*
  pipe-sync.c: Use Pipe for Process Synchronization
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

// Question: Update this program to synchronize execution of the parent and
// child processes using pipes, so that we are guaranteed the following
// sequence of print messages:
// Child line 1
// Parent line 1
// Child line 2
// Parent line 2


int main()
{
  char *s, buf[1024];
  int ret, stat;
  s  = "Use Pipe for Process Synchronization\n";

  /* create pipe */
  int p1[2], p2[2]; // use two pipes (one for child->parent, other for parent->child)
  if (pipe(p1) == -1 || pipe(p2) == -1) { // initialize the two pipes
    printf("Couldn't initialize pipes\n");
    exit(0);
  }

  ret = fork();
  if (ret == 0) {

    /* child process. */
    close(p1[0]); // close read end of pipe 1
    close(p2[1]); // close write end of pipe 2

    printf("Child line 1\n");
    write(p1[1], "done", 4); // tell parent "done"

    read(p2[0], buf, 4); // wait until parent says "done"
    printf("Child line 2\n");
    write(p1[1], "done", 4); // tell parent "done" again

    close(p1[1]);
    close(p2[0]);
  } else {

    /* parent process */
    close(p1[1]); // close write end of pipe 1
    close(p2[0]); // close read end of pipe 2

    read(p1[0], buf, 4); // wait until child says "done"
    printf("Parent line 1\n");
    write(p2[1], "done", 4); // tell child "done"

    read(p1[0], buf, 4); // wait until child says "done" again
    printf("Parent line 2\n");
    write(p2[1], "done", 4); // tell child "done" again

    close(p1[0]);
    close(p2[1]);

    wait(&stat);
  }
}
