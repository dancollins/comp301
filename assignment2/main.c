/*
 * Simple Scheduler
 *
 * Dan Collins 2014
 * 1183446
 *
 * COMP301 - 14B
 */

/* TODO list
 * - Create instruction.c file to construct instructions
 * - Create job.c file to construct jobs
 * - Modify list entries to include deconstructor for elements
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "list.h"

#define MAX_LINE 1024
#define MAX_INSTRUCTION 32
#define MAX_NAME 256


/**
 * Generic instruction function
 * @param params Input parameters to instruction
 */
typedef void (*instruction_function_t)(char *params);

/**
 * Instruction structure. This will be added to a list of supported
 * instructions and compared against the input data.
 */
typedef struct instruction_s {
  /* instruction name */
  char *name;
  /* instruction function */
  instruction_function_t func;
} instruction_t;

/**
 * Job structure. This will be inserted into the schedule.
 */
typedef struct job_s {
  /* job name */
  char name[MAX_NAME];
  /* time until job runs */
  unsigned int time;
  /* time between executions */
  unsigned int repeat;
} job_t;

/*
 * Global variables
 */
static unsigned int current_time;
static list_t *schedule;

void usage(void) {
  printf("usage: scheduler input_file\n");
}

/**
 * Remove trailing newline character if it exists.
 * @param s The string to modify
 */
void rstrip(char *s) {
  char *tmp;

  tmp = strchr(s, (int)'\n');
  if (tmp != NULL)
    *tmp = '\0';
}

int job_compare_time(void *d1, void *d2) {
  job_t *j1, *j2;

  j1 = (job_t *)d1;
  j2 = (job_t *)d2;

  if (j1->time < j2->time)
    return -1;
  else if (j1->time > j2->time)
    return 1;

  return 0;
}

int job_compare_name(void *d1, void *d2) {
  job_t *j1, *j2;

  j1 = (job_t *)d1;
  j2 = (job_t *)d2;

  return strncmp(j1->name, j2->name, MAX_NAME);
}

/*
 * Instructions
 */
static void i_time(char *params) {
  current_time = (unsigned int)atoi(params);

  /* Scan the schedule to see if any jobs need to be processed */
}

static void i_add(char *params) {
  char *tmp;
  unsigned int time;
  job_t *job;

  /* Time */
  tmp = strtok(params, " ");
  time = (unsigned int)atoi(tmp);

  /* Job */
  tmp = strtok(NULL, " ");

  printf("adding %s at %d\n", tmp, time);
  
  /* Create a job structure */
  job = malloc(sizeof(job_t));
  if (job == NULL) {
    fprintf(stderr, "Failed to allocate memory for job.\n");
    return;
  }

  /* Copy job details */
  strncpy(job->name, tmp, MAX_NAME);
  job->time = time;
  job->repeat = 0;

  /* Add job to the schedule */
  list_insert(schedule, (void *)job, job_compare_time);
}

static void i_addrep(char *params) {
  char *tmp;
  unsigned int time, repeat;
  job_t *job;

  /* Time */
  tmp = strtok(params, " ");
  time = (unsigned int)atoi(tmp);

  /* Repeat */
  tmp = strtok(NULL, " ");
  repeat = (unsigned int)atoi(tmp);

  /* Job */
  tmp = strtok(NULL, " ");

  printf("adding %s at %d repeating every %d\n", tmp, time, repeat);

  /* Create a job structure */
  job = malloc(sizeof(job_t));
  if (job == NULL) {
    fprintf(stderr, "Failed to allocate memory for job.\n");
    return;
  }

  /* Copy job details */
  strncpy(job->name, tmp, MAX_NAME);
  job->time = time;
  job->repeat = repeat;

  /* Add job to the schedule */
  list_insert(schedule, (void *)job, job_compare_time);
}

static void i_del(char *params) {
  job_t *job;

  job = malloc(sizeof(job_t));
  if (job == NULL) {
    fprintf(stderr, "Failed to allocate memory for job.\n");
    return;
  }

  strncpy(job->name, params, MAX_NAME);
  
  printf("Removing %s from the schedule\n", params);
  list_remove(schedule, (void *)job, job_compare_name);
}

static void i_list(char *params) {
  list_el_t *tmp;
  job_t *job;

  printf("---- SCHEDULE -----\n");

  tmp = schedule->head;
  while(tmp != NULL) {
    job = (job_t *)tmp->data;
    printf("%s: %d (%d)\n", job->name, job->time,
	   job->repeat);
    tmp = tmp->next;
  }

  printf("--------------------\n");
}

static void i_clear(char *params) {
  printf("clear\n");
}

void add_instruction(list_t *list, char *name,
		     instruction_function_t func) {
  instruction_t *inst;

  /* Create a new instruction */
  inst = malloc(sizeof(instruction_t));
  if (inst == NULL) {
    fprintf(stderr, "Unable to allocate memory for instruction.\n");
    return;
  }
  inst->name = name;
  inst->func = func;

  /* Insert into the list */
  list_add(list, inst);
}

/* Create the map between instruction name and function */
void build_instructions(list_t *list) {
  add_instruction(list, "TIME", i_time);
  add_instruction(list, "ADD", i_add);
  add_instruction(list, "ADDREP", i_addrep);
  add_instruction(list, "DEL", i_del);
  add_instruction(list, "LIST", i_list);
  add_instruction(list, "CLEAR", i_clear);
}

void process_instruction(list_t *instructions, char *inst,
			 char *params) {
  list_el_t *tmp;
  instruction_t *instruction;

  /* Scan the list until the desired instruction is found */
  tmp = instructions->head;
  while (tmp != NULL) {
    instruction = (instruction_t *)tmp->data;
    if (strncmp(instruction->name, inst, MAX_INSTRUCTION) == 0) {
      instruction->func(params);
      return;
    }

    tmp = tmp->next;
  }

  fprintf(stderr, "Invalid instruction: %s\n", inst);
}

/*
 * Main Function
 */
int main(int argc, char **argv) {
  FILE *infile = NULL;
  char line[MAX_LINE];
  unsigned int time = 0;
  list_t *instructions;
  char *inst, *params, *tmp;

  /* Create the instruction list */
  instructions = list_init();
  build_instructions(instructions);

  /* Prepare the schedule */
  schedule = list_init();

  /* Check input arguments */
  if (argc != 2) {
    fprintf(stderr, "Invalid input arguments.\n");
    usage();
    return -1;
  }

  /* Open the input file */
  infile = fopen(argv[1], "r");
  if (infile == NULL) {
    fprintf(stderr, "Error (%d) opening the specified input file.\n",
	    errno);
    return -1;
  }

  /* Read the input file */
  while(fgets(line, MAX_LINE, infile) != NULL) {
    /* Create a pointer to the instruction and the parameters,
     * inserting a null character between */
    inst = line;
    tmp = strchr(line, (int)' ');
    if (tmp != NULL) {
      params = tmp+1;
      rstrip(params);
      *tmp = '\0';
    } else {
      rstrip(inst);
      *params = '\0';
    }


    /* Run the instruction */
    process_instruction(instructions, inst, params);
  }

  /* Close the input file */
  fclose(infile);

  return 0;
}