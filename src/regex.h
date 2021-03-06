#ifndef _REGEX_H_
#define _REGEX_H_
#include "state_pattern.h"
#include <stddef.h>
#include <stdint.h>

struct reg_env;
struct reg_pattern;
struct fast_dfa_t;

struct reg_env *reg_open_env();
void reg_close_env(struct reg_env *env);

struct reg_pattern *reg_new_pattern(struct reg_env *env, const char *rule);
struct fast_dfa_t *lvzixun_regex_get_fast_dfa(struct reg_env *env, const char *rule);
void reg_free_pattern(struct reg_pattern *pattern);
struct reg_longjump **reg_get_exception(struct reg_env *env);

int reg_match(struct reg_pattern *pattern, const char *source, int len);
int lvzixun_fast_dfa_reg_match(struct fast_dfa_t *fast_dfa, const char *source);
void lvzixun_fast_dfa_reg_sum_batch(struct fast_dfa_t *fast_dfa, char *source[8], int ret[8]);
void lvzixun_fast_dfa_reg_match_batch(struct fast_dfa_t *fast_dfa, char *source[8], int ret[8]);
void lvzixun_fast_dfa_state_match_batch_same_len(const struct fast_dfa_t *fast_dfa, char *s[8], int ret[8]);
#endif
