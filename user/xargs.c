#include "kernel/types.h"
#include "kernel/param.h"
#include "user/user.h"

// scan from start of input line, split into args by space and end at \0
int split_line_by_space(char* line, char** splited, int max_splited_count) {
  char* p = line;
  int count = 0;
  while (*p != '\0' && count < max_splited_count) {
    splited[count++] = p;
    while (*p != '\0' && *p != ' ') p++;
    if (*p == ' ') {
      *p++ = '\0';
    }
  }
  return count;
}

int main(int argc, char *argv[]) {
  char* args[MAXARG];
  for (int i = 0; i < argc - 1; i++) {
    args[i] = argv[i + 1];
  }
  
  char line_buf[512];
  char *buf_p = line_buf;
  char ch;
  while (read(0, &ch, 1) != 0) {
    if (ch != '\n') {
      *buf_p++ = ch;
    }
    else {
      *buf_p = '\0';
      int new_argc = argc - 1;
      new_argc += split_line_by_space(line_buf, args + new_argc, MAXARG - new_argc);
      if (fork() == 0) {
        if (exec(argv[1], args)) {
          printf("%s: exec failed in xargs\n", argv[1]);
        }
      } else {
        wait(0);
        buf_p = line_buf;
      }
    }
  }
  exit(0);
}
