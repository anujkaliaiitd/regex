#ifndef _REG_STATE_pattern_
#define _REG_STATE_pattern_

#include "reg_parse.h"
#include <stdint.h>

/*
  the interior private headfile
*/

#define DEF_EDGE 4

#define DEF_EDGES 128
#define DEF_NODES DEF_EDGES *DEF_EDGE
#define DEF_FRAMES DEF_EDGES * 2

#define DEF_SUBSET_COUNT 2

struct reg_capture;

struct reg_edge {
  struct reg_range range;
};

//  the posation is index add 1
struct _reg_path {
  size_t edge_pos;      // the posation of edges_list
  size_t next_node_pos; // the posation of state_list
};

//  state node
struct reg_node {
  size_t node_pos; // the posation of state_list

  size_t merge_pos; // the merge posation

  int subset_tag;
  int is_end; // is end state

  struct reg_list *subset; // the state of subset
  struct reg_list *edges;  // the struct _reg_path object list
};

struct reg_pattern {
  struct reg_state *state;

  int match_tail; // is match end of lines
  int match_head; // is match begin of lines

  // nfa
  size_t start_state_pos;

  // dfa
  size_t dfa_start_state_pos;

  // min dfa
  size_t min_dfa_start_state_pos;
  int minsubset_max;

  int closure_tag;
  struct reg_list *eval_subset;

  int is_match_tail;

  struct reg_list *state_list; // the struct reg_node object list
  struct reg_list *edges_list; // the struct reg_edge object list
};

void state_gen(struct reg_pattern *pattern, struct reg_ast_node *ast);
int state_match(struct reg_pattern *pattern, const char *s, int len);
int state_match_opt(struct reg_pattern *pattern, const char *s, int len);

// Maximum number of DFA states, less than 65535
#define MONETDB_MAX_DFA_STATES 300

struct fast_dfa_state_t {
  uint8_t is_match;         // Is this state a matching state?
  uint16_t *transition_arr; // Array of 256 transitions
};

// The DFA is just an array of states
struct fast_dfa_t {
  uint16_t root_state; // Index of the root state
  struct fast_dfa_state_t state_arr[MONETDB_MAX_DFA_STATES];
};

// Post-process the DFA represented in @pattern into @fast_dfa
void postprocess_dfa(struct reg_pattern *pattern, struct fast_dfa_t *fast_dfa);
int lvzixun_fast_dfa_state_match(struct fast_dfa_t *fast_dfa, const char *s, int len);

// state op
struct reg_edge *state_edge_pos(struct reg_pattern *pattern, size_t pos);
struct reg_node *state_node_pos(struct reg_pattern *pattern, size_t pos);
#endif
