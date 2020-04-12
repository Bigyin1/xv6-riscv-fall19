#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
  int p1[2];
  int p2[2];
  char byte = 'a';

  pipe(p1);
  pipe(p2);

  if (fork() == 0) {
    int pid = getpid();
    close(p1[1]);
    close(p2[0]);
    while (1) {
      if (!read(p1[0], &byte, 1)) {
        close(p2[1]);
        break;
      }
      printf("%d: received ping\n", pid);
      write(p2[1], &byte, 1);
    }
    exit();
  }

  close(p1[0]);
  close(p2[1]);
  while (1) {
    int pid = getpid();
    write(p1[1], &byte, 1);
    if (!read(p2[0], &byte, 1)) {
      close(p1[1]);
      break;
    }
    printf("%d: received pong\n", pid);
  }
  exit();
}