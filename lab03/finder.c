#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <strings.h>
#include <errno.h>
#include <sys/wait.h>

#define BSIZE 256

#define BASH_EXEC  "/bin/bash"
#define FIND_EXEC  "/bin/find"
#define XARGS_EXEC "/usr/bin/xargs"
#define GREP_EXEC  "/bin/grep"
#define SORT_EXEC  "/bin/sort"
#define HEAD_EXEC  "/usr/bin/head"

int main(int argc, char *argv[])
{
  int status;
  pid_t pid_1, pid_2, pid_3, pid_4;

  int p1[2], p2[2], p3[2];

  //Checking if input parameters DIR, STR and NUM_FILES are correct
  if (argc != 4) {
    printf("usage: finder DIR STR NUM_FILES\n");
    exit(0);
  }

  //STEP 1
	//Initialize pipes p1, p2, and p3
  if (pipe(p1) == -1 || pipe(p2) == -1 || pipe(p3) == -1) {
    printf("Couldn't initialize pipes\n");
    exit(0);
  }

  pid_1 = fork();
  if (pid_1 == 0) {
    /* First Child */

    //STEP 2
		//In this first child process, we want to send everything that is printed on the standard output, to the next child process through pipe p1
		//So, redirect standard output of this child process to p1's write end - written data will be automatically available at pipe p1's read end
		//And, close all other pipe ends except the ones used to redirect the above OUTPUT (very important)
    close(p1[0]);
    dup2(p1[1], STDOUT_FILENO);
    close(p1[1]);
    close(p2[0]);
    close(p2[1]);
    close(p3[0]);
    close(p3[1]);

    //STEP 3
    //Prepare a command string representing the find command (follow example from the slide)
    //Invoke execl for bash and find (use BASH_EXEC and FIND_EXEC as paths)
    execl(BASH_EXEC, BASH_EXEC, "-c", FIND_EXEC, argv[1], NULL);
    exit(0);
  }

  pid_2 = fork();
  if (pid_2 == 0) {
    /* Second Child */

    //STEP 4
    //In this second child process, we want to receive everything that is available at pipe p1's read end, and use the received information as standard input for this child process
		//In this second child process, we want to send everything that is printed on the standard output, to the next child process through pipe p2
		//So, redirect standard output of this child process to p2's write end - written data will be automatically available at pipe p2's read end
		//And, close all other pipe ends except the ones used to redirect the above two INPUT/OUTPUT (very important)
    dup2(p1[0], STDIN_FILENO);
    close(p1[0]);
    close(p1[1]);
    close(p2[0]);
    dup2(p2[1], STDOUT_FILENO);
    close(p2[1]);
    close(p3[0]);
    close(p3[1]);

    //STEP 5
    //Invoke execl for xargs and grep (use XARGS_EXEC and GREP_EXEC as paths)
    execl(XARGS_EXEC, XARGS_EXEC, GREP_EXEC, argv[2], NULL);
    exit(0);
  }

  pid_3 = fork();
  if (pid_3 == 0) {
    /* Third Child */

    //STEP 6
		//In this third child process, we want to receive everything that is available at pipe p2's read end, and use the received information as standard input for this child process
		//In this third child process, we want to send everything that is printed on the standard output, to the next child process through pipe p3
		//So, redirect standard output of this child process to p3's write end - written data will be automatically available at pipe p3's read end
		//And, close all other pipe ends except the ones used to redirect the above two INPUT/OUTPUT (very important)
    close(p1[0]);
    close(p1[1]);
    dup2(p2[0], STDIN_FILENO);
    close(p2[0]);
    close(p2[1]);
    close(p3[0]);
    dup2(p3[1], STDOUT_FILENO);
    close(p3[1]);

    //STEP 7
    //Invoke execl for sort (use SORT_EXEC as path)
    execl(SORT_EXEC, SORT_EXEC, NULL);
    exit(0);
  }

  pid_4 = fork();
  if (pid_4 == 0) {
    /* Fourth Child */

    //STEP 8
		//In this fourth child process, we want to receive everything that is available at pipe p3's read end, and use the received information as standard input for this child process
		//Output of this child process should directly be to the standard output and NOT to any pipe
		//And, close all other pipe ends except the ones used to redirect the above INPUT (very important)
    close(p1[0]);
    close(p1[1]);
    close(p2[0]);
    close(p2[1]);
    dup2(p3[0], STDIN_FILENO);
    close(p3[0]);
    close(p3[1]);

    //STEP 8
    //Invoke execl for head (use HEAD_EXEC as path)
    execl(HEAD_EXEC, HEAD_EXEC, "-n", argv[3], NULL);
    exit(0);
  }

  close(p1[0]);
  close(p1[1]);
  close(p2[0]);
  close(p2[1]);
  close(p3[0]);
  close(p3[1]);

  if ((waitpid(pid_1, &status, 0)) == -1) {
    fprintf(stderr, "Process 1 encountered an error. ERROR%d", errno);
    return EXIT_FAILURE;
  }
  if ((waitpid(pid_2, &status, 0)) == -1) {
    fprintf(stderr, "Process 2 encountered an error. ERROR%d", errno);
    return EXIT_FAILURE;
  }
  if ((waitpid(pid_3, &status, 0)) == -1) {
    fprintf(stderr, "Process 3 encountered an error. ERROR%d", errno);
    return EXIT_FAILURE;
  }
  if ((waitpid(pid_4, &status, 0)) == -1) {
    fprintf(stderr, "Process 4 encountered an error. ERROR%d", errno);
    return EXIT_FAILURE;
  }

  return 0;
}
