#include <assert.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
struct cli {
  int show_pids;
  int numeric_sort;
  int version;
};

void traverce_proc_dir_and_print(struct cli);
void process_version() { printf("pstree (PSmisc) 23.7\n"); }

void process(struct cli cli) {
  if (cli.version) {
    process_version();
    return;
  }
  traverce_proc_dir_and_print(cli);
}

typedef struct {
  char pid[16];
  char name[64];
} Process;

// Get the process name from the /proc directory through the process ID
Process get_process(char *pid) {
  Process process;
  char path[64];
  snprintf(path, sizeof(path), "%s/%s/%s", "/proc", pid, "comm");
  FILE *file = fopen(path, "r");
  if (!file) {
    perror("fopen");
    exit(1);
  }
  char buffer[256];
  ssize_t bytesRead = fread(buffer, sizeof(char), 255, file);
  if (bytesRead < 0) {
    perror("fread");
    exit(1);
  }
  // 确保字符串以空字符结尾
  buffer[bytesRead - 1] = '\0';

  strncpy(process.pid, pid, sizeof(process.pid) - 1);
  process.pid[sizeof(process.pid) - 1] = '\0';
  strncpy(process.name, buffer, sizeof(process.name) - 1);
  process.name[sizeof(process.name) - 1] = '\0';
  return process;
}

int process_cmp_of_name(const void *a, const void *b) {
  Process *pa = (Process *)a;
  Process *pb = (Process *)b;
  return strcmp(pa->name, pb->name);
}

void dfs_print_process_tree(Process proc, int depth, int show_pids,
                            int numeric_sort) {

  for (int i = 0; i < depth; i++) {
    printf("        ");
    if(i==depth-1){
      printf("├───");
    }
  }
  printf("%s", proc.name);
  if (show_pids) {
    printf("{%s}", proc.pid);
  }
  printf("\n");
  char path[64];
  snprintf(path, sizeof(path), "%s/%s/%s/%s/%s", "/proc", proc.pid, "task",
           proc.pid, "children");
  FILE *file = fopen(path, "r");
  if (!file) {
    return;
  }

  // 分配缓冲区
  char buffer[256];
  // 使用fread读取文件内容
  size_t bytesRead = fread(buffer, sizeof(char), 255, file);
  if (bytesRead <= 0) {
    return;
  }
  // 确保字符串以空字符结尾
  buffer[bytesRead] = '\0';
  char *token = strtok(buffer, " ");
  int ans[64] = {};
  int count = 0;
  while (token != NULL) {
    int num = atoi(token);
    ans[count++] = num;
    token = strtok(NULL, " "); // 获取下一个token
  }

  if (numeric_sort) {
    for (int i = 0; i < count; i++) {
      char child_pid[16];
      sprintf(child_pid, "%d", ans[i]);
      dfs_print_process_tree(get_process(child_pid), depth + 1, show_pids,
                             numeric_sort);
    }
  } else {
    Process children[count];
    for (int i = 0; i < count; i++) {
      char child_pid[16];
      sprintf(child_pid, "%d", ans[i]);
      children[i] = get_process(child_pid);
    }
    qsort(children, count, sizeof(Process), process_cmp_of_name);

    for (int i = 0; i < count; i++) {
      dfs_print_process_tree(children[i], depth + 1, show_pids, numeric_sort);
    }
  }
  return;
}

void traverce_proc_dir_and_print(struct cli cli) {
  dfs_print_process_tree(get_process("1"), 0, cli.show_pids, cli.numeric_sort);
}

int main(int argc, char *argv[]) {
  struct cli cli = {0, 0, 0};
  for (int i = argc - 1; i > 0; i--) {
    if (strstr(argv[i], "-p") || strstr(argv[i], "--show-pids")) {
      cli.show_pids = 1;
    }
    if (strstr(argv[i], "-n") || strstr(argv[i], "numeric-sort")) {
      cli.numeric_sort = 1;
    }
    if (strstr(argv[i], "-V") || strstr(argv[i], "--version")) {
      cli.version = 1;
    }
  }
  process(cli);
  assert(!argv[argc]);
  return 0;
}