#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int parent_send_pipe[2];
  int child_send_pipe[2];
  if (pipe(parent_send_pipe) == 1 || pipe(child_send_pipe)) {
    fprintf(2, "error creating pipes");
    exit(0);
  }
  
  int is_child = fork() == 0;
  if (is_child) {
    int child_write_fd = child_send_pipe[1];
    int child_read_fd = parent_send_pipe[0];
    char ball[1];
    read(child_read_fd, ball, 1);
    printf("%d: received ping\n", getpid());
    write(child_write_fd, ball, 1);
    exit(0);
  } else {
    int parent_write_fd = parent_send_pipe[1];
    int parent_read_fd = child_send_pipe[0];
    char ball[1];
    write(parent_write_fd, ball, 1);
    close(parent_write_fd);    
    read(parent_read_fd, ball, 1);
    printf("%d: received pong\n", getpid());
    exit(0);
  }
}
