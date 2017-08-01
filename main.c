#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <elf.h>
#include <sys/mman.h>
#include <sys/ptrace.h>

#define RED     "\x1b[31m"
#define CLEAR   "\x1b[0m"

char** parseArgs(char** argv_l, int argc_l)
{
  char** args;
  int num_args = argc_l - 1;
  int count = 0;

  args = (char**) malloc(num_args*sizeof(char*));
  
  while(count < num_args) {
    if(count != num_args-1) {
      args[count] = strdup(argv_l[count+2]);
      
      if(args[count] == NULL) {
	fprintf(stderr, "Failed to strdup: %s\n", strerror(errno));
	exit(EXIT_FAILURE);
      }	
    }
    else
      args[count] = NULL;
    count++;
  }

  return args;
};

int main(int argc, char** argv)
{
  pid_t pid_child;
  Elf64_Addr sym_addr;
  char* sym_name;
  char** args_parsed; 
  void* mapped_mem;
  Elf64_Ehdr* elf_hdr; 
  Elf64_Phdr* program_hdr;
  Elf64_Shdr* section_hdr;

  args_parsed = parseArgs(argv, argc);
  
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
