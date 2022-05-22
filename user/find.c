#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "user/user.h"

char*
fmtname(char *path)
{
  static char buf[DIRSIZ+1];
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  // Return name, ended with a null character
  if(strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));
  memset(buf+strlen(p), '\0', DIRSIZ-strlen(p));
  return buf;
}


void find(char* base, char* name) {
  int fd_base;

  fd_base = open(base, 0);
  if (fd_base < 0) {
    fprintf(2, "find: cannot open %s\n", base);
    return;
  }

  struct stat st_base;
  if(fstat(fd_base, &st_base) < 0) {
    fprintf(2, "find: cannot stat %s\n", st_base);
    close(fd_base);
    return;
  }

  char buf[512];
  strcpy(buf, base);
  if (st_base.type == T_FILE && strcmp(fmtname(buf), name) == 0) {
    printf("%s\n", buf);
  } else if (st_base.type == T_DIR) {
    if (strlen(base) + 1 + DIRSIZ + 1 > sizeof buf) {
        fprintf(2, "find: path too long\n");
        close(fd_base);
        return;
    }
    
    char* p = buf + strlen(buf);
    *p++ = '/';
    struct dirent de;
    struct stat st;
    
    while (read(fd_base, &de, sizeof(de)) == sizeof(de)) {
      if (de.inum == 0) continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      if (stat(buf, &st) < 0) {
        fprintf(2, "find: cannot stat %s\n", buf);
        continue;
      }
      if (strcmp(fmtname(buf), ".") != 0 && strcmp(fmtname(buf), "..") != 0) {
        find(buf, name);
      }
    }
  }  
  close(fd_base);
}

int
main(int argc, char *argv[]) {
  int i;

  if(argc < 2) {
    fprintf(2, "find takes at least one arg(find [base_dir] <file_name>)");
    exit(0);
  }
  if (argc == 2) {
    find(".", argv[1]);
  } else {
    for (i = 2; i < argc; i++) {
      find(argv[1], argv[i]);
    }
  }
  exit(0);
}
