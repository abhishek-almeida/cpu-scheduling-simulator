#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>
#include <time.h>

#define RNG_LIMIT 5
#define RNG_OFFSET 3

enum algos { fcfs, sjf, srtf, pri, pri_np, rr } algo;

struct process {
  int pid;
  int arrival_t;
  int burst_t;
  int priority;
  char state;

  // to be calculated
  int exec_t;
  int complete_t;
  int turnaround_t;
  int waiting_t;
  int response_t;
};

struct ready_q {
  struct process *entry;
  STAILQ_ENTRY(ready_q) entries;
};

struct gantt_chart {
  int time_marker;
  int pid;
  STAILQ_ENTRY(gantt_chart) entries;
};

STAILQ_HEAD(r_head, ready_q);
STAILQ_HEAD(gc_head, gantt_chart);

struct r_head r_h;
struct gc_head gc_h;

struct process *p;
struct ready_q *r, *nr, *temp;
struct gantt_chart *gc;
int process_count = 0;
int counter = 0;
int time_quantum = 0;
float avg_complete_t = 0;
float avg_turnaround_t = 0;
float avg_waiting_t = 0;
float avg_response_t = 0;

void help();
int rng();
int compare_fn(const void *, const void *);
void randomize_inputs();
void get_manual_inputs();
bool incoming_process();
void arrange_ready_q();
void dispatch(struct ready_q *, bool);
void schedule();
void get_srt_process();
void do_fcfs();
void do_sjf();
void do_srtf();
void do_pri();
void do_pri_np();
void do_rr();
void calculate_times();
void calculate_avg_times();
void print_times();
void print_avg_times();
void add_chart_block(int, int);
void draw_chart();

void help() {
  printf("\nUsage: mini-project [-m] -a algorithm");
  printf("\nBy default, random values will be generated.");
  printf("\nPass the -m option to manually provide input");
  printf("\n\nAlgorithms are:");
  printf("\n fcfs: first in, first out");
  printf("\n sjf: shortest job first");
  printf("\n srtf: shortest remaining time first");
  printf("\n pri: priority");
  printf("\n pri_np: priority (non - preemptive)");
  printf("\n rr: round robin");
  printf("\n\nExample: mini-project -a srtf");
  printf("\n\n");
  exit(0);
}

int rng() { return (rand() % RNG_LIMIT) + RNG_OFFSET; }

int compare_fn(const void *a, const void *b) {
  const struct process *pa = (struct process *)a;
  const struct process *pb = (struct process *)b;

  return (pa->arrival_t > pb->arrival_t) - (pa->arrival_t < pb->arrival_t);
}

void randomize_inputs() {
  process_count = rng();
  p = malloc(sizeof(struct process) * process_count);
  for (int i = 0; i < process_count; ++i) {
    p[i].pid = i;
    p[i].arrival_t = rng();
    p[i].burst_t = rng();
    if (algo == pri || algo == pri_np)
      p[i].priority = rng();
    else
      p[i].priority = 0;
    // to be calculated
    p[i].state = 'C';
    p[i].exec_t = 0;
    p[i].complete_t = 0;
    p[i].turnaround_t = 0;
    p[i].waiting_t = 0;
    p[i].response_t = -1;
  }
  if (algo == rr)
    time_quantum = rng();
}

void get_manual_inputs() {
  printf("Enter the number of processes: ");
  scanf("%d", &process_count);
  p = malloc(sizeof(struct process) * process_count);
  if (algo == rr) {
    printf("Enter time quantum: ");
    scanf("%d", &time_quantum);
  }
  for (int i = 0; i < process_count; ++i) {
    p[i].pid = i;
    printf("\n> for process %d,", i);
    printf("\nArrival time: ");
    scanf("%d", &p[i].arrival_t);
    printf("Burst time: ");
    scanf("%d", &p[i].burst_t);
    if (algo == pri || algo == pri_np) {
      printf("Priority: ");
      scanf("%d", &p[i].priority);
    } else
      p[i].priority = -1;

    // to be calculated
    p[i].state = 'C';
    p[i].exec_t = 0;
    p[i].complete_t = 0;
    p[i].turnaround_t = 0;
    p[i].waiting_t = 0;
    p[i].response_t = -1;
  }
}

bool incoming_process() {
  for (int i = 0; i < process_count; ++i)
    if (p[i].state == 'C')
      return true;
  return false;
}

void dispatch(struct ready_q *r, bool is_head) {
  if (r->entry->response_t == -1)
    r->entry->response_t = counter;
  add_chart_block(counter, r->entry->pid);
  ++counter;
  r->entry->exec_t += 1;
  arrange_ready_q();
  if (r->entry->burst_t == r->entry->exec_t) {
    r->entry->state = 'X';
    r->entry->complete_t = counter;
    if (is_head)
      STAILQ_REMOVE_HEAD(&r_h, entries);
    else
      STAILQ_FOREACH(nr, &r_h, entries) {
        if (nr == r)
          STAILQ_REMOVE(&r_h, r, ready_q, entries);
      }
  }
}

void arrange_ready_q() {
  for (int i = 0; i < process_count; ++i) {
    if (p[i].arrival_t <= counter && p[i].state == 'C') {
      temp = malloc(sizeof(struct ready_q));
      STAILQ_INSERT_TAIL(&r_h, temp, entries);
      temp->entry = &p[i];
      p[i].state = 'R';
    }
  }
}

void schedule() {
  while (incoming_process() || !STAILQ_EMPTY(&r_h)) {
    arrange_ready_q();
    if (STAILQ_EMPTY(&r_h)) {
      add_chart_block(counter, -1);
      ++counter;
    } else {
      switch (algo) {
      case fcfs:
        do_fcfs();
        break;
      case sjf:
        do_sjf();
        break;
      case srtf:
        do_srtf();
        break;
      case pri:
        do_pri();
        break;
      case pri_np:
        do_pri_np();
        break;
      case rr:
        do_rr();
        break;
      }
    }
  }
}

void get_srt_process() {
  int min_remain_t = INT_MAX;
  int min_remain_pid = 0;
  STAILQ_FOREACH(nr, &r_h, entries) {
    if ((nr->entry->burst_t - nr->entry->exec_t) < min_remain_t) {
      min_remain_t = nr->entry->burst_t - nr->entry->exec_t;
      min_remain_pid = nr->entry->pid;
    }
  }
  STAILQ_FOREACH(nr, &r_h, entries)
  if (nr->entry->pid == min_remain_pid)
    r = nr;
}

void get_pri_process() {
  int max_pri = INT_MIN;
  int max_pri_pid = 0;
  STAILQ_FOREACH(nr, &r_h, entries) {
    if (nr->entry->priority > max_pri) {
      max_pri = nr->entry->priority;
      max_pri_pid = nr->entry->pid;
    }
  }
  STAILQ_FOREACH(nr, &r_h, entries)
  if (nr->entry->pid == max_pri_pid)
    r = nr;
}

void do_fcfs() {
  r = STAILQ_FIRST(&r_h);
  for (int i = 0; i < r->entry->burst_t; ++i)
    dispatch(r, true);
}

void do_sjf() {
  get_srt_process();
  for (int i = 0; i < r->entry->burst_t; ++i)
    dispatch(r, false);
}

void do_srtf() {
  get_srt_process();
  dispatch(r, false);
}

void do_pri() {
  get_pri_process();
  dispatch(r, false);
}

void do_pri_np() {
  get_pri_process();
  for (int i = 0; i < r->entry->burst_t; ++i)
    dispatch(r, false);
}

void do_rr() {
  r = STAILQ_FIRST(&r_h);
  for (int i = 0; i < time_quantum; ++i)
    if (r->entry->state != 'X')
      dispatch(r, true);
  if (r == STAILQ_FIRST(&r_h)) {
    STAILQ_REMOVE_HEAD(&r_h, entries);
    STAILQ_INSERT_TAIL(&r_h, r, entries);
  }
}

void calculate_times() {
  for (int i = 0; i < process_count; ++i) {
    p[i].turnaround_t = p[i].complete_t - p[i].arrival_t;
    p[i].waiting_t = p[i].turnaround_t - p[i].burst_t;
  }
}

void calculate_avg_times() {
  int total_complete_t = 0;
  int total_turnaround_t = 0;
  int total_waiting_t = 0;
  int total_response_t = 0;
  for (int i = 0; i < process_count; ++i) {
    total_complete_t += p[i].complete_t;
    total_turnaround_t += p[i].turnaround_t;
    total_waiting_t += p[i].waiting_t;
    total_response_t += p[i].response_t;
  }
  avg_complete_t = (float)total_complete_t / process_count;
  avg_turnaround_t = (float)total_turnaround_t / process_count;
  avg_waiting_t = (float)total_waiting_t / process_count;
  avg_response_t = (float)total_response_t / process_count;
}

void print_times() {
  if (algo == rr)
    printf("\nTime Quantum: %d\n", time_quantum);
  if (counter) {
    printf("\n%3s %7s %5s %3s %8s %3s %4s %8s", "PID", "ARRIVAL", "BURST",
           "PRI", "COMLPETE", "TAT", "WAIT", "RESPONSE");
    for (int i = 0; i < process_count; ++i)
      printf("\n%3d %7d %5d %3d %8d %3d %4d %8d", p[i].pid, p[i].arrival_t,
             p[i].burst_t, p[i].priority, p[i].complete_t, p[i].turnaround_t,
             p[i].waiting_t, p[i].response_t);
  } else {
    printf("\n%3s %7s %5s %3s", "PID", "ARRIVAL", "BURST", "PRI");
    for (int i = 0; i < process_count; ++i)
      printf("\n%3d %7d %5d %3d", p[i].pid, p[i].arrival_t, p[i].burst_t,
             p[i].priority);
  }
}

void print_avg_times() {
  printf("\n\n\nAverage Completion time = %.2f", avg_complete_t);
  printf("\nAverage Turnaround time = %.2f", avg_turnaround_t);
  printf("\nAverage Waiting time = %.2f", avg_waiting_t);
  printf("\nAverage Response time = %.2f", avg_response_t);
}

void add_chart_block(int tm, int pid) {
  gc = malloc(sizeof(struct gantt_chart));
  STAILQ_INSERT_TAIL(&gc_h, gc, entries);
  gc->time_marker = tm;
  gc->pid = pid;
}

void draw_chart() {
  printf("\nGantt Chart:\n");
  int prev_pid = INT_MIN;
  STAILQ_FOREACH(gc, &gc_h, entries) {
    if (gc->pid == prev_pid)
      continue;
    else {
      printf("\n----%d", gc->time_marker);
      if (gc->pid != -1)
        printf("\nP%d", gc->pid);
      prev_pid = gc->pid;
    }
  }
  printf("\n----%d\n\n", counter);
}

int main(int argc, char *argv[]) {
  srand(time(NULL));
  bool is_random = true;
  if (argc == 1)
    help();
  for (int i = 1; i < argc; ++i) {
    if (!strcmp(argv[i], "-a")) {
      ++i;
      if (i >= argc) {
        printf("\nMissing option value.");
        help();
      }
      if (!strcmp(argv[i], "fcfs"))
        algo = fcfs;
      else if (!strcmp(argv[i], "sjf"))
        algo = sjf;
      else if (!strcmp(argv[i], "srtf"))
        algo = srtf;
      else if (!strcmp(argv[i], "pri"))
        algo = pri;
      else if (!strcmp(argv[i], "pri_np"))
        algo = pri_np;
      else if (!strcmp(argv[i], "rr"))
        algo = rr;
      else {
        printf("\nInvalid argument ’%s’.", argv[i]);
        help();
      }
    } else if (!strcmp(argv[i], "-m"))
      is_random = false;
    else {
      printf("\nInvalid option ’%s’.", argv[i]);
      help();
    }
  }
  if (is_random)
    randomize_inputs();
  else
    get_manual_inputs();

  STAILQ_INIT(&r_h);
  STAILQ_INIT(&gc_h);

  if (is_random) {
    printf("\nInputs:\n");
    print_times();
    printf("\n\n");
  }

  qsort(p, process_count, sizeof(struct process), compare_fn);

  schedule();
  calculate_times();
  calculate_avg_times();

  printf("\nAfter scheduling:\n");
  print_times();
  print_avg_times();
  printf("\n\n");
  draw_chart();

  free(p);
  free(temp);
  return 0;
}
