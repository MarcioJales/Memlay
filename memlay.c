#include <libgen.h>
#include <sys/mman.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include "retdwarf.h"

#define BUF_LENGTH          256

#define TEXT                0
#define DATA                1
#define BSS                 2
#define HEAP                3
#define STACK               4


void getSection(FILE *fs, char *readbuf, uint8_t section)
{
  char *start_addr, *end_addr;
  int offset;

  if(section == TEXT)
    printf(GREEN "[Text section]\n" CLEAR);
  if(section == DATA)
    printf(GREEN "[Data section]\n" CLEAR);
  if(section == BSS)
    printf(GREEN "[Bss section]\n" CLEAR);
  if(section == HEAP)
    printf(GREEN "[Heap]\n" CLEAR);
  if(section == STACK)
    printf(GREEN "[Stack]\n" CLEAR);

  offset = ftell(fs);

  fgets(readbuf, BUF_LENGTH, fs);
  if(section == HEAP) {
    if(strncmp(readbuf + 74, "heap", 4)) {
      printf(RED "None\n" CLEAR);
	    fseek(fs, offset, SEEK_SET);
      return;
    }
  }
  if(section == STACK) {
    while(strncmp(readbuf + 74, "stack", 5)) {
      offset = ftell(fs);
      fgets(readbuf, BUF_LENGTH, fs);
    }
    fseek(fs, offset, SEEK_SET);
  }

  start_addr = strtok(readbuf, "-");
  printf(GREEN "%s" CLEAR, start_addr);

  fseek(fs, offset + strlen(start_addr), SEEK_SET);
  fgets(readbuf, BUF_LENGTH, fs);
  end_addr = strtok(readbuf, " ");
  printf(GREEN "%s\n" CLEAR, end_addr);
};


void parseMapsFile(char *maps_path, char *prog_path)
{
  FILE *fs;
  uint8_t section;
  char readbuf[BUF_LENGTH];
  char *basename_prog;

  fs = fopen(maps_path, "r");
  if(fs == NULL) {
    fprintf(stderr, RED "Failed to open: %s\n" CLEAR, strerror(errno));
    exit(EXIT_FAILURE);
  }

  /* 73 is the offset to get to the pathname of the file */
  if(fseek(fs, 73, SEEK_SET) == -1) {
    fprintf(stderr, RED "Failed to operate on the file: %s\n" CLEAR, strerror(errno));
    exit(EXIT_FAILURE);
  }
  fgets(readbuf, BUF_LENGTH, fs);
  rewind(fs);

  basename_prog = basename(prog_path);
  if(!strncmp(basename_prog, basename(readbuf), strlen(basename_prog))) {
    for(section = 0; section < 5; section++)
      getSection(fs, readbuf, section);
  }
  else
    fprintf(stderr, RED "File described in \"maps\" do not match the tracee\n" CLEAR);

  fclose(fs);
};


void printLayout(pid_t pid, char* prog_path)
{
  char* maps_path = (char*) malloc(16); /* 'pid' may have at most 5 digits */

  printf(YELLOW "\nPrinting the layout:\n\n" CLEAR);
  if(sprintf(maps_path, "/proc/%d/maps", pid) < 0)
    fprintf(stderr, RED "Error to parse the path to \"maps\"\n" CLEAR);

  parseMapsFile(maps_path, prog_path);
  printf("\n");
};


void adjustRegisters(pid_t pid, Dwarf_Addr sym_addr, long orig_data, long trap)
{
  struct user_regs_struct registers;

  if(ptrace(PTRACE_GETREGS, pid, NULL, &registers) == -1) {
    fprintf(stderr, RED "Failed to get registers information: %s\n" CLEAR, strerror(errno));
    exit(EXIT_FAILURE);
  }

  if(ptrace(PTRACE_POKEDATA, pid, sym_addr, orig_data) == -1) {
    fprintf(stderr, RED "Failed to poke data: %s\n" CLEAR, strerror(errno));
    exit(EXIT_FAILURE);
  }

  registers.rip = registers.rip - 1;
  if(ptrace(PTRACE_SETREGS, pid, NULL, &registers) == -1) {
    fprintf(stderr, RED "Failed to get registers information: %s\n" CLEAR, strerror(errno));
    exit(EXIT_FAILURE);
  }

  if(ptrace(PTRACE_SINGLESTEP, pid, NULL, NULL) == -1) {
    fprintf(stderr, RED "Failed to continue the tracing: %s\n" CLEAR, strerror(errno));
    exit(EXIT_FAILURE);
  }
  wait(NULL);

  if(ptrace(PTRACE_POKEDATA, pid, sym_addr, trap) == -1) {
    fprintf(stderr, RED "Failed to poke data: %s\n" CLEAR, strerror(errno));
    exit(EXIT_FAILURE);
  }

};


int main(int argc, char** argv)
{
  int status;
  long orig_data, trap; /* The 'trap' is the opcode '0xcc', which means that, ate that address, the execution must stop */
  pid_t pid_child;
  Dwarf_Addr sym_addr;

  if(argc < 3) {
    printf("Usage: memlay <breakpoint> <program>\n");
    exit(EXIT_SUCCESS);
  }

  char* sym_name = (char*) malloc(strlen(argv[1]));
  char* prog_path = (char*) malloc(strlen(argv[2]));

  strcpy(prog_path, argv[2]);
  strcpy(sym_name, argv[1]);

  sym_addr = dwarfAnalysis(prog_path, sym_name);
  if(sym_addr == 0) {
    fprintf(stderr, RED "Error: " CLEAR "symbol not found\n");
    exit(EXIT_FAILURE);
  }
  else
    printf(GREEN "Symbol found\n" CLEAR);
  printf(YELLOW "Breakpoint at %llx\n" CLEAR, sym_addr);

  pid_child = fork();
  if(pid_child == -1) {
    fprintf(stderr, RED "Failed to fork: %s\n" CLEAR, strerror(errno));
    exit(EXIT_FAILURE);
  }
  if (pid_child == 0) {
    if(ptrace(PTRACE_TRACEME, 0, NULL, NULL) == -1) {
      fprintf(stderr, RED "Failed to start tracing: %s\n" CLEAR, strerror(errno));
      exit(EXIT_FAILURE);
    }
    if(execv(prog_path, argv + 2) == -1) {
      fprintf(stderr, RED "Failed to start the tracee: %s\n" CLEAR, strerror(errno));
      exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
  }

  wait(&status);
  printf(YELLOW "Beginning of the analysis of process %d (%s)\n" CLEAR, pid_child, prog_path);

  /* Since  the  value  returned by a successful PTRACE_PEEK* request may be
   * -1, the caller must clear errno before the  call,  and  then  check  it
   * afterward to determine whether or not an error occurred.
   */

  errno = 0;
  orig_data = ptrace(PTRACE_PEEKDATA, pid_child, sym_addr, NULL);
  if(orig_data == -1) {
    if(errno != 0) {
      fprintf(stderr, RED "Failed to peek data: %s\n" CLEAR, strerror(errno));
      exit(EXIT_FAILURE);
    }
  }

  trap = (orig_data & ~0xff) | 0xcc;
  if(ptrace(PTRACE_POKEDATA, pid_child, sym_addr, trap) == -1) {
    fprintf(stderr, RED "Failed to poke data: %s\n" CLEAR, strerror(errno));
    exit(EXIT_FAILURE);
  }

  while(1) {
    if(ptrace(PTRACE_CONT, pid_child, NULL, NULL) == -1) {
      fprintf(stderr, RED "Failed to continue the tracing: %s\n" CLEAR, strerror(errno));
      exit(EXIT_FAILURE);
    }
    wait(&status);

    if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGTRAP) {
      printf(YELLOW "Hit the breakpoint %llx\n" CLEAR, sym_addr);

      printLayout(pid_child, prog_path);
      adjustRegisters(pid_child, sym_addr, orig_data, trap);
    }

    if(WIFEXITED(status)) {
      printf(YELLOW "Completed tracing pid %d\n" CLEAR, pid_child);
      break;
    }
  }

  return 0;
}
