Refer to fork.c code. Navigate to the comment starting with "Question 1" in the code.

Question 1: Which process prints this line?

  None 
  Only the parent process 
  Only the child process 
  Both the parent and the child process 
 
Question 2
Not yet graded / 4 pts
 Refer to fork.c code. Navigate to the comment starting with "Question 1" in the code.

Question 1: What is printed?

Your Answer:
The parent process will print the actual process id of the child.

The child process will print "After fork, Process id = 0", indicating that it is a child process.

 
Question 3
4 / 4 pts
Refer to fork.c code. Navigate to the comment starting with "Question 2" in the code.

Question 2: WITH RESPECT TO THE CHILD PROCESS, what will be printed if this line is commented?

  Final statement from Process 
  Final statement from Process ... print after execlp 
  print after execlp , Final statement from Process ... 
  print after execlp 
 
Question 4
4 / 4 pts
Refer to fork.c code. Navigate to the comment starting with "Question 3" in the code.

Question 3: When is this line reached/printed?

  When execlp is successful 
  When execlp is removed 
  When execlp fails 
 
Question 5
4 / 4 pts
Refer to fork.c code. Navigate to the comment starting with "Question 4" in the code.

Question 4: What happens if the parent process is killed first? Uncomment the next two lines.

//sleep(4);

//printf("In Child: %d, Parent: %d\n", getpid(), getppid());

  The child process does not become a zombie process 
  The child process gets adopted by the init (or systemd) process 
  If the child process is inherited by the init process, the getppid() should ideally return 1 
  The child process becomes a zombie process 
 
Question 6
10 / 10 pts
Refer to mfork.c code. In this program, how many processes are created including the parent process?
  7 
  4 
  3 
  8 
 
Question 7
Not yet graded / 15 pts
Refer to pipe-sync.c code.

Question: Update this program to synchronize execution of the parent and child processes using pipes, so that we are guaranteed the following sequence of print messages:

Child line 1

Parent line 1

Child line 2

Parent line 2

Submit your updated pipe-sync.c file here.

 pipe-sync.c
 
Question 8
3 / 3 pts
Refer to the fifo_producer.c and fifo_consumer.c code.

Question a: What happens if you only launch a producer (but no consumer)?

  Producer keeps on waiting 
  Producer exits 
  Producer starts producing values 
  The consumer needs to be launched to answer this question 
 
Question 9
3 / 3 pts
Refer to the fifo_producer.c and fifo_consumer.c code.

Question b: What happens if you only launch a consumer (but no producer)?

 

  Consumer starts consuming 
  Consumer keeps on waiting 
  The producer needs to be launched to answer this question 
  Consumer exits 
 
IncorrectQuestion 10
0 / 3 pts
Refer to the fifo_producer.c and fifo_consumer.c code.

Question c: If one producer and multiple consumers, then who gets the message sent?

  Last Consumer 
  Random consumer 
  The first consumer 
  No consumer 
 
Question 11
3 / 3 pts
The producer continues writing messages into the fifo, if all of the consumers have already exited.
  True 
  False 
 
IncorrectQuestion 12
0 / 3 pts
The consumers also exit, if all the producers are terminated.
  True 
  False 
 
Question 13
Not yet graded / 10 pts
Refer to the shared_memory3.c code.

Question: Explain the output

Your Answer:
shared_buf before fork: First String
unshared_buf before fork: First String
shared_buf in child: First String
unshared_buf in child: First String
shared_buf after fork: Second String
unshared_buf after fork: First String

The fork creates a copy of the parent's memory space for the child, meaning that when the child modified the unshared_buf to STR2, the parents unshared memory remained as STR1.
However, the child modified the shared_buf as well, meaning that shared memory also changed for the parent process. Since the parent process waits for the child process to finish executing, the parent process shows the shared_buf was changed to STR2.

 
Question 14
Not yet graded / 10 pts
Refer to the thread-1.c code. Observe the execution of this code and answer the following:

Question: Are changes made to the local or global variables by the child PROCESS reflected in the parent process? EXPLAIN in the context of memory spaces.

Your Answer:
No, the changes made by the child process are not reflected in the parent process.
The parent's entire memory space was copied for the child when forked, meaning that the child only changes its own local and global variables.

 
Question 15
Not yet graded / 10 pts
Refer to the thread-1.c code. Observe the execution of this code and answer the following:

Question: Are changes made to the local or global variables by the child THREAD reflected in the parent process? Separately EXPLAIN for the local and global variables.

Your Answer:
Yes, the changes made by the child thread are reflected in the parent.
Unlike creating a new child process, child threads directly share the memory space with the parent.

The global variable can be changed both threads exist in the same process.
The local variable exists in the stack of the main thread, but the child thread can change it by using the pointer leading to it (in the parent thread's stack).