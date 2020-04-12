#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void int_to_bytes(int num, char res[4]) {
  res[0] = (char)num;
  res[1] = (char)(num >> 8);
  res[2] = (char)(num >> 16);
  res[3] = (char)(num >> 24);
}

int bytes_to_int(char b[4]) {
  return (int)(b[3]) << 24 | (int)(b[2]) << 16 | (int)(b[1]) << 8 | (int)b[0];
}

void create_new_layer(int rd, int wd) {
  int p, n;
  int is_next = 0;
  int next_pipe[2];

  if (fork() == 0) {
    close(wd);
    if (read(rd, &p, 4) == 4) {
      printf("prime %d\n", p);
    } else {
      close(rd);
      exit();
    }

    while (read(rd, &n, 4) == 4) {
      if (n % p != 0) {
        if (!is_next) {
          pipe(next_pipe);
          create_new_layer(next_pipe[0], next_pipe[1]);
          close(next_pipe[0]);
          is_next = 1;
        }
        write(next_pipe[1], &n, 4);
      }
    }
    close(next_pipe[1]);
    exit();
  }
}

int main(int argc, char* argv[]) {
  int next_pipe[2];

  pipe(next_pipe);

  create_new_layer(next_pipe[0], next_pipe[1]);
  close(next_pipe[0]);

  for (int i = 2; i <= 35; ++i) {
    write(next_pipe[1], &i, 4);
  }
  close(next_pipe[1]);
  exit();
}