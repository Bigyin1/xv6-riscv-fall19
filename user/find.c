#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

int match(char *, char *);

char *get_name(char *path) {
  char *start = path;
  path += (strlen(path) - 1);

  while (path >= start && *path != '/') {
    path--;
  }
  path++;
  return path;
}

void walk(char *re, char *path) {
  char path_buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  if ((fd = open(path, 0)) < 0) {
    fprintf(1, "find: cannot open %s\n", path);
    return;
  }

  if (fstat(fd, &st) < 0) {
    fprintf(1, "find: cannot stat %s\n", path);
    close(fd);
    return;
  }
  switch (st.type) {
    case T_FILE:
      if (match(re, get_name(path))) {
        printf("%s\n", path);
      }
      break;

    case T_DIR:
      if (strlen(path) + 1 + DIRSIZ + 1 > sizeof path_buf) {
        printf("%s\n", "46");
        break;
      }
      strcpy(path_buf, path);
      p = path_buf + strlen(path_buf);
      *p++ = '/';
      while (read(fd, &de, sizeof(de)) == sizeof(de)) {
        if (de.inum == 0) continue;
        if (!strcmp(de.name, ".") || !strcmp(de.name, "..")) continue;
        memmove(p, de.name, DIRSIZ);
        p[DIRSIZ] = 0;
        walk(re, path_buf);
      }
      break;
  }
  close(fd);
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    printf("find: not enouhg args\n");
    exit();
  }
  walk(argv[2], argv[1]);
  exit();
}

// Regexp matcher from Kernighan & Pike,
// The Practice of Programming, Chapter 9.

int matchhere(char *, char *);
int matchstar(int, char *, char *);

int match(char *re, char *text) {
  if (re[0] == '^') return matchhere(re + 1, text);
  do {  // must look at empty string
    if (matchhere(re, text)) return 1;
  } while (*text++ != '\0');
  return 0;
}

// matchhere: search for re at beginning of text
int matchhere(char *re, char *text) {
  if (re[0] == '\0') return 1;
  if (re[1] == '*') return matchstar(re[0], re + 2, text);
  if (re[0] == '$' && re[1] == '\0') return *text == '\0';
  if (*text != '\0' && (re[0] == '.' || re[0] == *text))
    return matchhere(re + 1, text + 1);
  return 0;
}

// matchstar: search for c*re at beginning of text
int matchstar(int c, char *re, char *text) {
  do {  // a * matches zero or more instances
    if (matchhere(re, text)) return 1;
  } while (*text != '\0' && (*text++ == c || c == '.'));
  return 0;
}
