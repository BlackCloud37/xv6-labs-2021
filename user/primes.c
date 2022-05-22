#include "kernel/types.h"
#include "user/user.h"

void prime(int in_fd) {
  int base;
  if (read(in_fd, &base, sizeof(base)) == 0) {
    exit(0);
  }
  printf("prime %d\n", base);

  int to_right_pipe[2];
  pipe(to_right_pipe);

  if (fork() == 0) {
    close(to_right_pipe[1]);
    prime(to_right_pipe[0]);
  } else {
    close(to_right_pipe[0]);
    int n;
    while (read(in_fd, &n, sizeof(n))) {
      if (n % base != 0) {
        write(to_right_pipe[1], &n, sizeof(n));
      }
    }
    close(to_right_pipe[1]);
  }
  wait(0);
  exit(0);
}

int
main(int argc, char *argv[]) {
  int num_generator_pipe[2];
  pipe(num_generator_pipe);

  if (fork() == 0) {
    close(num_generator_pipe[1]);
    prime(num_generator_pipe[0]);
  } else {
    for (int i = 2; i <= 35; i++) {
      write(num_generator_pipe[1], &i, sizeof(i));
    }
    close(num_generator_pipe[0]);
    close(num_generator_pipe[1]);
  }
  wait(0);
  exit(0);
}
