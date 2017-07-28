#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

#define RED     "\x1b[31m"
#define CLEAR   "\x1b[0m"

int main(int argc, char** argv)
{
  pid_t pid_child;

  pid_child = fork();
  
  if(pid_child == -1) {
    fprintf(stderr, "Failed to fork: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
  else if (pid_child == 0){
    
  }
  else {
  }
  
  return 0;
}
