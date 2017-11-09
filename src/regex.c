#include "reg_malloc.h"
#include "reg_parse.h"
#include "reg_state.h"
#include "state_pattern.h"

#include <string.h>
#include "reg_error.h"
#include "regex.h"
#include "reg_list.h"



struct reg_env {
  struct reg_longjump* exception_chain;

  struct reg_parse* parse_p;
  struct reg_state* state_p;
};


struct reg_longjump** reg_get_exception(struct reg_env* env){
  return &(env->exception_chain);
}


REG_API struct reg_env* reg_open_env(){
  struct reg_env* env = (struct reg_env*)malloc(sizeof(struct reg_env));
  env->exception_chain = NULL;
  env->parse_p = parse_new(env);
  env->state_p = state_new(env);
  return env;
}

REG_API void reg_close_env(struct reg_env* env){
  parse_free(env->parse_p);
  state_free(env->state_p);
  free(env);
}

struct _pattern_arg{
  struct reg_env* env;
  const char* rule;
  struct reg_pattern* pattern;
};

static void _pgen_pattern(struct _pattern_arg* argv){
  struct reg_ast_node* root = parse_exec(argv->env->parse_p, argv->rule, strlen(argv->rule));
  int is_match_tail = parse_is_match_tail(argv->env->parse_p);
  argv->pattern = state_new_pattern(argv->env->state_p, root, is_match_tail);
}

uint8_t** dfa_2D_vector(struct reg_pattern* pattern) {
  size_t state_list_len = list_len(pattern->state_list);
  for (size_t i = 1; i < state_list_len; i++) {
    struct reg_node* node = state_node_pos(pattern, i);
    assert(node != NULL);
  }

  #define MAX_NUM_STATES 255
  const size_t initial_dfa_node_pos = pattern->min_dfa_start_state_pos;
  size_t node_pos = initial_dfa_node_pos;

  // seen[i] == 1 means that I have "seen" state i. It does not mean that
  // I have "visited" state i (in the traditional BFS visited sense).
  uint8_t seen[MAX_NUM_STATES] = {0};
  uint8_t visited[MAX_NUM_STATES] = {0};
  static uint8_t *states[MAX_NUM_STATES] = {NULL};

  // bool_new_visited becomes false when I don't visit any new states in a pass
  uint8_t bool_new_visited = 1;
  seen[initial_dfa_node_pos] = 1;

  while (bool_new_visited == 1) {
    bool_new_visited = 0;  // Assume that we won't "see" any new states in this pass

    for (size_t state_i = 0; state_i < MAX_NUM_STATES; state_i++) {
      if (seen[state_i] == 0) continue;
      if (visited[state_i] == 1) continue;

      struct reg_node* node = state_node_pos(pattern, state_i);
      assert(node != NULL);
      struct reg_list* edges = node->edges;
      struct _reg_path* path = NULL;

      // This state is seen but not visited. Now visiting state_i.
      //printf("Visiting state %zu.\n", state_i);

      assert(states[state_i] == NULL);
      states[state_i] = malloc((MAX_NUM_STATES+1) * sizeof(uint8_t));
      for (size_t edge_i = 0; edge_i <= MAX_NUM_STATES; edge_i++) {
        assert(initial_dfa_node_pos <= MAX_NUM_STATES);
        states[state_i][edge_i] = (uint8_t)node_pos;
      }

      for (size_t edge_i = 0; (path = list_idx(edges, edge_i)); edge_i++) {
        size_t next_node_pos = path->next_node_pos;
        assert(next_node_pos < MAX_NUM_STATES);

        struct reg_range* range = &(state_edge_pos(pattern, path->edge_pos)->range);
        assert(range != NULL);

        for (uint8_t c = (char)(range->begin); c <= (char)(range->end); c++) {
          assert(states[state_i][c] == initial_dfa_node_pos);
          states[state_i][c] = (uint8_t)next_node_pos;
          //printf("Adding edge state[%zu][%u] to state %zu.\n", state_i, c, next_node_pos);
        }

        if (seen[next_node_pos] == 0) {
          //printf("Visiting new state %zu from node %zu.\n", next_node_pos, state_i);
          // Record that we "saw" a new state in this pass, so that we do
          // another pass. The next pass will "visit" this state (in the
          // traditional BFS visiting sense).
          bool_new_visited = 1;
          seen[next_node_pos] = 1;
        }
      }

      visited[state_i] = 1;
    }
  }
  return states;
}

REG_API struct reg_pattern* reg_new_pattern(struct reg_env* env, const char* rule){
  if(rule == NULL || env == NULL) return NULL;

  parse_clear(env->parse_p);
  
  // set exception handling
  struct _pattern_arg argv = {
    .env = env,
    .rule = rule,
    .pattern = NULL,
  };

  if(reg_cpcall(env, (pfunc)_pgen_pattern, &argv)){
    return NULL;
  }
  
  return argv.pattern;
}

REG_API void reg_free_pattern(struct reg_pattern* pattern){
  state_free_pattern(pattern);
}


REG_API int reg_match(struct reg_pattern* pattern, const char* source, int len){
  return state_match(pattern, source, len);
}


