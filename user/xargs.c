#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

char whitespace[] = " \t\r\n\v";

void read_argv(int s_idx, char* argv[MAXARG], char* buf) {
  int len = 0;
  char* arg;

  while (len < 99 && read(0, buf + len, 1) == 1) {
    ++len;
  }
  buf[len] = 0;
  while (*buf && strchr(whitespace, *buf)) buf++;

  arg = buf;
  while (*buf) {
    if (strchr(whitespace, *buf)) {
      *buf++ = 0;
      if (s_idx >= MAXARG) {
        printf("xargs: too many args\n");
        exit();
      }
      while (*buf && strchr(whitespace, *buf)) buf++;
      argv[s_idx++] = arg;
      arg = buf;
      continue;
    }
    ++buf;
  }
  if (*buf) {
    if (s_idx >= MAXARG) {
      printf("xargs: too many args\n");
      exit();
    }
    argv[s_idx++] = arg;
  }
  argv[s_idx] = 0;
}

int main(int argc, char* argv[]) {
  char* xargv[MAXARG + 1];
  char buf[100];

  if (argc < 2) {
    printf("xargs: need at least one orgsn\n");
    exit();
  }

  for (int i = 1; i < argc; ++i) {
    xargv[i - 1] = argv[i];
  }
  read_argv(argc - 1, xargv, buf);
  if (fork() == 0) {
    exec(xargv[0], xargv);
  }
  wait();
  exit();
}