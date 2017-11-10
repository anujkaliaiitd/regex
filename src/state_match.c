/*
  match regex rule from minimization dfa
*/

#include "ds_queue.h"
#include "reg_list.h"
#include "reg_stream.h"
#include "regex.h"
#include "state_pattern.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#define debug 0

static int _match_dfa_state(struct reg_pattern *pattern, size_t node_pos,
                            struct reg_stream *source);
static int _match_nfa_state(struct reg_pattern *pattern, size_t node_pos,
                            struct reg_stream *source);
struct timespec start, end;

int state_match(struct reg_pattern *pattern, const char *s, int len) {
  assert(_match_nfa_state); // filter warning

  struct reg_stream *source = stream_new((const unsigned char *)s, len);
  int success =
      _match_dfa_state(pattern, pattern->min_dfa_start_state_pos, source);

  stream_free(source);
  return success;
}

int state_match_opt(struct reg_pattern *pattern, const char *s, int len) {
  struct fast_dfa_t fast_dfa;
  postprocess_dfa(pattern, &fast_dfa);
  // Now we must not use @pattern anymore

  int cur_state = fast_dfa.root_state;
  for (int i = 0; i < len; i++) {
    if (fast_dfa.state_arr[cur_state].is_match == 1)
      return 1;
    uint8_t c = (uint8_t)s[i];

    assert(fast_dfa.state_arr[cur_state].transition_arr != NULL);
    cur_state = fast_dfa.state_arr[cur_state].transition_arr[c];
  }

  if (fast_dfa.state_arr[cur_state].is_match == 1)
    return 1;
  return 0;
}

int only_state_match_opt(struct fast_dfa_t *fast_dfa, const char *s, int len) {
  // struct fast_dfa_t fast_dfa;
  // postprocess_dfa(pattern, &fast_dfa);
  // Now we must not use @pattern anymore

  int cur_state = fast_dfa->root_state;
  for (int i = 0; i < len; i++) {
    if (fast_dfa->state_arr[cur_state].is_match == 1)
      return 1;
    uint8_t c = (uint8_t)s[i];

    assert(fast_dfa->state_arr[cur_state].transition_arr != NULL);
    cur_state = fast_dfa->state_arr[cur_state].transition_arr[c];
  }

  if (fast_dfa->state_arr[cur_state].is_match == 1)
    return 1;
  return 0;
}

void postprocess_dfa(struct reg_pattern *pattern, struct fast_dfa_t *fast_dfa) {
  assert(pattern->min_dfa_start_state_pos <= MONETDB_MAX_DFA_STATES);
  fast_dfa->root_state = (uint16_t)pattern->min_dfa_start_state_pos;

  // Set all DFA states to non-matching and all transition arrays to NULL
  for (size_t i = 0; i < MONETDB_MAX_DFA_STATES; i++) {
    fast_dfa->state_arr[i].is_match = 0;
    fast_dfa->state_arr[i].transition_arr = NULL;
  }

  // Initialize the BFS
  struct ds_queue_t state_queue;
  ds_queue_init(&state_queue);
  uint8_t visited[MONETDB_MAX_DFA_STATES] = {0};

  ds_queue_add(&state_queue, (int)fast_dfa->root_state);

  while (ds_queue_size(&state_queue) > 0) {
    int cur_state_i = ds_queue_remove(&state_queue);

    // It's possible that cur_state_i is visited. This happens when a state is
    // added to the queue more than once.
    if (visited[cur_state_i] == 1)
      continue;

    // cur_state_i is unvisited, so it's uninitialized
    struct fast_dfa_state_t *cur_fast_dfa_state =
        &fast_dfa->state_arr[cur_state_i];
    assert(cur_fast_dfa_state->is_match == 0);
    assert(cur_fast_dfa_state->transition_arr == NULL);

    struct reg_node *node = state_node_pos(pattern, cur_state_i);
    assert(node != NULL);

    // Initialize this state. Set all transitions to root.
    cur_fast_dfa_state->is_match = node->is_end;
    cur_fast_dfa_state->transition_arr = malloc(256 * sizeof(uint16_t));
    for (size_t t_i = 0; t_i < 256; t_i++) {
      cur_fast_dfa_state->transition_arr[t_i] = fast_dfa->root_state;
    }

    // printf("Visiting state %d. Is match = %u.\n",
    // cur_state_i, cur_fast_dfa_state->is_match);

    struct reg_list *edges = node->edges;
    struct _reg_path *path = NULL;
    for (size_t edge_i = 0; (path = list_idx(edges, edge_i)); edge_i++) {
      int next_node_pos = path->next_node_pos;
      assert(next_node_pos < MONETDB_MAX_DFA_STATES);

      struct reg_range *range =
          &(state_edge_pos(pattern, path->edge_pos)->range);
      assert(range != NULL);

      for (uint8_t c = (char)(range->begin); c <= (char)(range->end); c++) {
        assert(cur_fast_dfa_state->transition_arr[c] == fast_dfa->root_state);
        cur_fast_dfa_state->transition_arr[c] = (uint16_t)next_node_pos;
        // printf("Adding edge state[%d][%u] to state %d.\n",
        // cur_state_i, c, next_node_pos);
      }

      // Add unvisited state to BFS queue
      if (visited[next_node_pos] == 0)
        ds_queue_add(&state_queue, next_node_pos);
    }

    visited[cur_state_i] = 1;
  }
}

static int _match_dfa_state(struct reg_pattern *pattern, size_t node_pos,
                            struct reg_stream *source) {
  size_t state_list_len = list_len(pattern->state_list);
  for (size_t i = 1; i < state_list_len; i++) {
    struct reg_node *node = state_node_pos(pattern, i);
    assert(node != NULL);
  }

  // printf("_match_dfa_state: node_pos = %zu, state list size = %zu\n",
  // node_pos, list_len(pattern->state_list));

  for (; !stream_end(source);) {
    printf("node_pos = %zu.\n", node_pos);
    // dump edge
    struct reg_node *node = state_node_pos(pattern, node_pos);
    struct reg_list *edges = node->edges;
    struct _reg_path *path = NULL;
    unsigned char c = stream_char(source);

    if (node->is_end && !pattern->is_match_tail)
      return 1;

    for (size_t i = 0; (path = list_idx(edges, i)); i++) {
      struct reg_range *range =
          &(state_edge_pos(pattern, path->edge_pos)->range);
      size_t next_node_pos = path->next_node_pos;

      assert(range);
      printf("range: %c, %c. next node = %zu.\n", (char)(range->begin),
             (char)(range->end), next_node_pos);
      if (c >= range->begin && c <= range->end) { // range
        node_pos = next_node_pos;
        stream_next(source);
        break;
      }
    }

    if (!path)
      return 0;
  }

  // match is end of source
  return state_node_pos(pattern, node_pos)->is_end;
}

static int _match_nfa_state(struct reg_pattern *pattern, size_t node_pos,
                            struct reg_stream *source) {
  // pass end state
  if (stream_end(source) && state_node_pos(pattern, node_pos)->is_end) {
    return 1;
  }

  // dump edge
  struct reg_list *edges = state_node_pos(pattern, node_pos)->edges;
  struct _reg_path *path = NULL;
  unsigned char c = stream_char(source);

  int success = 0;
  for (size_t i = 0; (path = list_idx(edges, i)); i++) {
    struct reg_range *range = &(state_edge_pos(pattern, path->edge_pos)->range);
    size_t next_node_pos = path->next_node_pos;

    if (range == NULL) { // edsilone
      success = _match_nfa_state(pattern, next_node_pos, source);
    } else if (c >= range->begin && c <= range->end) { // range
      if (stream_next(source)) {
        assert(success == 0);
        success = _match_nfa_state(pattern, next_node_pos, source);
        stream_back(source);
      }
    }

    if (success)
      return 1;
  }

  return 0;
}
