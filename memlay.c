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

static uint8_t opt_all, opt_verbose;

void getSection(FILE *fs, char *readbuf, uint8_t section)
{
  char *start_addr, *end_addr;
  int offset;

  if(section == TEXT && opt_all)
    printf(GREEN "[Text section]  " CLEAR);
  if(section == DATA && opt_all)
    printf(GREEN "[Data section]  " CLEAR);
  if(section == BSS && opt_all)
    printf(GREEN "[Bss section]   " CLEAR);
  if(section == HEAP)
    printf(GREEN "[Heap]          " CLEAR);
  if(section == STACK)
    printf(GREEN "[Stack]         " CLEAR);

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
  if(opt_all || section > 2)
    printf(GREEN "%s" CLEAR, start_addr);

  fseek(fs, offset + strlen(start_addr), SEEK_SET);
  fgets(readbuf, BUF_LENGTH, fs);
  end_addr = strtok(readbuf, " ");
  if(opt_all || section > 2)
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
  static int count = 1;

  if(sprintf(maps_path, "/proc/%d/maps", pid) < 0)
    fprintf(stderr, RED "Error parsing the path to 'maps' file\n" CLEAR);

  printf(GREEN "----> [%d]\n" CLEAR, count);
  count++;
  parseMapsFile(maps_path, prog_path);
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


void getOpt(int argc, char** argv)
{
  /* "a" = show text, data and bss sections as well
   * "v" = verbose mode
   */
  int opt = getopt(argc, argv, "va");

  while(opt != -1) {
    if(opt == 'a')
      opt_all = 1;
    else if (opt == 'v')
      opt_verbose = 1;
    else if(opt == '?') {
      fprintf(stderr, RED "Error: option %c not recognized\n" CLEAR, optopt);
      exit(EXIT_FAILURE);
    }

    opt = getopt(argc, argv, "va");
  }
};


int main(int argc, char** argv)
{
  int status;
  long orig_data, trap; /* The 'trap' is the opcode '0xcc', which means that, ate that address, the execution must stop */
  pid_t pid_child;
  Dwarf_Addr sym_addr;

  getOpt(argc, argv);
  if(argc < 3 || argc - optind < 2) {
    printf("Usage: memlay [-a|-v] <breakpoint> <program>\n");
    exit(EXIT_SUCCESS);
  }

  char* sym_name = (char*) malloc(strlen(argv[optind]));
  char* prog_path = (char*) malloc(strlen(argv[optind+1]));
  strcpy(prog_path, argv[optind+1]);
  strcpy(sym_name, argv[optind]);

  sym_addr = dwarfAnalysis(prog_path, sym_name);
  if(sym_addr == 0) {
    fprintf(stderr, RED "Error: symbol not found\n" CLEAR);
    exit(EXIT_FAILURE);
  }

  if(opt_verbose) {
    printf(GREEN "Symbol found\n" CLEAR);
    printf(YELLOW "Breakpoint at %llx\n" CLEAR, sym_addr);
  }

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
    if(execv(prog_path, argv + optind + 1) == -1) {
      fprintf(stderr, RED "Failed to start the tracee: %s\n" CLEAR, strerror(errno));
      exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
  }

  wait(&status);

  if(opt_verbose)
    printf(YELLOW "Beginning of the analysis of process %d (%s)\n" CLEAR, pid_child, prog_path);

  /* Since the value returned by a successful PTRACE_PEEK* request may be
   * -1, the caller must clear errno before the  call, and then check it
   * afterwards to determine whether or not an error occurred.
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
      if(opt_verbose)
        printf(YELLOW "Hit the breakpoint %llx\n" CLEAR, sym_addr);
      printLayout(pid_child, prog_path);
      adjustRegisters(pid_child, sym_addr, orig_data, trap);
    }

    if(WIFEXITED(status)) {
      if(opt_verbose)
        printf(YELLOW "Completed tracing pid %d\n" CLEAR, pid_child);
      break;
    }
  }

  return 0;
}
