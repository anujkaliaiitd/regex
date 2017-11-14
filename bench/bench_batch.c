#include "../src/regex.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define NUM_ITERS 10000000
#define STR_LEN 64

int main(int argc, char const *argv[]) {
  const char *rule = argv[1];
  printf("rule: %s\n", rule);

  struct reg_env *env = reg_open_env();

  struct timespec start, end;
  struct fast_dfa_t *fast_dfa = lvzixun_regex_get_fast_dfa(env, rule);

  char *source_batch[8];
  for (int i = 0; i < 8; i++) {
    source_batch[i] = (char *)malloc(STR_LEN);
    for (int j = 0; j < STR_LEN; j++) {
      source_batch[i][j] = 'A' + rand() % 70;
    }
  }

  clock_gettime(CLOCK_REALTIME, &start);

  int ret[8];
  int success = 0;
  for (int i = 0; i < NUM_ITERS; i++) {
    lvzixun_fast_dfa_state_match_batch_same_len(fast_dfa, source_batch, ret);
    //lvzixun_fast_dfa_reg_match_batch(fast_dfa, source_batch, ret);
    success += ret[0];
  }

  clock_gettime(CLOCK_REALTIME, &end);

  double nanoseconds =
      (end.tv_sec - start.tv_sec) * 1000000000 + (end.tv_nsec - start.tv_nsec);

  printf("Time per string = %f ns, success = %d\n", nanoseconds / NUM_ITERS / 8,
         success);
  reg_close_env(env);
  return 0;
}
