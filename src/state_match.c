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

int lvzixun_fast_dfa_state_match(struct fast_dfa_t *fast_dfa, const char *s) {
  // struct fast_dfa_t fast_dfa;
  // postprocess_dfa(pattern, &fast_dfa);
  // Now we must not use @pattern anymore

  int cur_state = fast_dfa->root_state;
  int len = strlen(s);

  for (int i = 0 ; i < len; i++) {
    //printf("cur state = %d, char = %c, len = %d\n", cur_state, s[i], len);
    //if (fast_dfa->bool_matching[cur_state] == 1)
    //  return 1;
    uint8_t c = (uint8_t)s[i];

    //assert(fast_dfa->state_arr[cur_state].transition_arr != NULL);
    cur_state = fast_dfa->state_arr[cur_state].transition_arr[c];
  }

  //printf("end state = %d\n", cur_state);
  return (fast_dfa->bool_matching[cur_state] == 1);
}

// Instead of doing a DFA match, just sum the strings
void lvzixun_fast_dfa_state_sum_batch(const struct fast_dfa_t *fast_dfa, char *s[8], int ret[8]) {
  int sum[8];
  int finished[8];
  int char_index[8];
  int tot_finished = 0;

  for (int i = 0; i < 8; i++) {
    sum[i] = 0;
    finished[i] = 0;
    char_index[i] = 0;
  }

  while (tot_finished != 8) {
    for (int i = 0; i < 8; i++) {
      if (finished[i] == 1) continue;

      uint8_t c = (uint8_t)s[i][char_index[i]];
      if (c == 0) {
        finished[i] = 1;
        tot_finished++;
        continue;
      }

      sum[i] += (uint8_t)s[i][char_index[i]];
      char_index[i]++;
    }
  }

  for (int i = 0; i < 8; i++) {
    ret[i] = ((sum[i] & 2) == 0);
  }
}

void lvzixun_fast_dfa_state_match_batch(const struct fast_dfa_t *fast_dfa, char *s[8], int ret[8]) {
  int cur_state[8];
  int finished[8];
  int char_index[8];
  int tot_finished = 0;
  for (int i = 0 ; i < 8; i++) {
    cur_state[i] = fast_dfa->root_state;
    finished[i] = 0;
    char_index[i] = 0;
  }

  while (tot_finished != 8) {
    for (int i = 0; i < 8; i++) {
      if (finished[i] == 1) continue;

      uint8_t c = (uint8_t)s[i][char_index[i]];
      if (c == 0) {
        finished[i] = 1;
        tot_finished++;
        continue;
      }

      cur_state[i] = fast_dfa->state_arr[cur_state[i]].transition_arr[c];
      char_index[i]++;
    }
  }

  for (int i = 0; i < 8; i++) {
    ret[i] = (fast_dfa->bool_matching[cur_state[i]] == 1);
  }
}

void lvzixun_fast_dfa_state_match_batch_same_len(
    const struct fast_dfa_t *fast_dfa, char *s[8], int ret[8]) {
  int cur_state[8];
  for (int batch_i = 0 ; batch_i < 8; batch_i++) {
    cur_state[batch_i] = fast_dfa->root_state;
  }

  int common_len = strlen(s[0]);

  for (int char_i = 0; char_i < common_len; char_i++) {
    for (int batch_i = 0; batch_i < 8; batch_i++) {
      uint8_t c = (uint8_t)s[batch_i][char_i];
      cur_state[batch_i] =
        fast_dfa->state_arr[cur_state[batch_i]].transition_arr[c];
    }
  }

  for (int batch_i = 0; batch_i < 8; batch_i++) {
    ret[batch_i] = (fast_dfa->bool_matching[cur_state[batch_i]] == 1);
  }
}

void postprocess_dfa(struct reg_pattern *pattern, struct fast_dfa_t *fast_dfa) {
  assert(pattern->min_dfa_start_state_pos <= MONETDB_MAX_DFA_STATES);
  fast_dfa->root_state = (uint8_t)pattern->min_dfa_start_state_pos;

  // Set all DFA states to non-matching and all transition arrays to NULL
  for (size_t i = 0; i < MONETDB_MAX_DFA_STATES; i++) {
    fast_dfa->bool_matching[i] = 0;
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
    assert(fast_dfa->bool_matching[cur_state_i] == 0);

    struct reg_node *node = state_node_pos(pattern, cur_state_i);
    assert(node != NULL);

    // Initialize this state
    fast_dfa->bool_matching[cur_state_i] = node->is_end;

    //printf("Visiting state %d. Is match = %d.\n", cur_state_i, node->is_end);

    // If this is a matching state, set all transitions to self
    if (node->is_end == 1) {
      for (size_t t_i = 0; t_i < 256; t_i++) {
        cur_fast_dfa_state->transition_arr[t_i] = cur_state_i;
      }

      visited[cur_state_i] = 1;
      continue;
    }

    for (size_t t_i = 0; t_i < 256; t_i++) {
      cur_fast_dfa_state->transition_arr[t_i] = fast_dfa->root_state;
    }


    struct reg_list *edges = node->edges;
    struct _reg_path *path = NULL;
    for (size_t edge_i = 0; (path = list_idx(edges, edge_i)); edge_i++) {
      int next_node_pos = path->next_node_pos;
      assert(next_node_pos < MONETDB_MAX_DFA_STATES);

      struct reg_range *range =
          &(state_edge_pos(pattern, path->edge_pos)->range);
      assert(range != NULL);

      //printf("Adding edge state[%d][%u-%u] to state %d. root state = %d\n",
          //cur_state_i, (uint8_t)range->begin, (uint8_t)range->end,
          //next_node_pos, fast_dfa->root_state);
      for (int c = range->begin; c <= range->end; c++) {
        assert(cur_fast_dfa_state->transition_arr[c] == fast_dfa->root_state);
        cur_fast_dfa_state->transition_arr[c] = (uint8_t)next_node_pos;
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
    //printf("node_pos = %zu.\n", node_pos);
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
      //printf("range: %c, %c. next node = %zu.\n", (char)(range->begin),
             //(char)(range->end), next_node_pos);
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
