#include "../src/regex.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define NUM_ITERS 10000000

int main(int argc, char const *argv[]) {
  assert(argc >= 3);
  const char *rule = argv[1];
  const char *source = argv[2];

  printf("rule: %s\n", rule);
  printf("source: %s\n", source);

  struct reg_env *env = reg_open_env();

  // struct reg_pattern* pattern = reg_new_pattern(env, rule);
  // int success = reg_match(pattern, source, strlen(source));

  struct timespec start, end;
  struct fast_dfa_t *fast_dfa = lvzixun_regex_get_fast_dfa(env, rule);

  clock_gettime(CLOCK_REALTIME, &start);

  int success = 0;
  for (int i = 0; i < NUM_ITERS; i++) {
    success += lvzixun_fast_dfa_reg_match(fast_dfa, source);
  }

  clock_gettime(CLOCK_REALTIME, &end);

  double nanoseconds =
      (end.tv_sec - start.tv_sec) * 1000000000 + (end.tv_nsec - start.tv_nsec);

  printf("Time per iter = %f ns, success = %d\n", nanoseconds / NUM_ITERS,
         success);
  reg_close_env(env);
  return 0;
}
