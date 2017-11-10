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
struct fast_dfa_t *only_reg_new_pattern(struct reg_env *env, const char *rule);
void reg_free_pattern(struct reg_pattern *pattern);
struct reg_longjump **reg_get_exception(struct reg_env *env);

int reg_match(struct reg_pattern *pattern, const char *source, int len);
int only_reg_match(struct fast_dfa_t *fast_dfa, const char *source, int len);
#endif
