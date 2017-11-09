/*
  match regex rule from minimization dfa
*/

#include <stdio.h>
#include "state_pattern.h"
#include "reg_stream.h"
#include "reg_list.h"
#include <time.h>
#include <math.h>
#include <string.h>
#include <stdint.h>

#include "regex.h"
#define debug 0

static int _match_dfa_state(struct reg_pattern* pattern, size_t node_pos, struct reg_stream* source);
static int _match_nfa_state(struct reg_pattern* pattern, size_t node_pos, struct reg_stream* source);
struct timespec start, end;

int state_match(struct reg_pattern* pattern, const char* s, int len){
  assert(_match_nfa_state); // filter warning

  struct reg_stream* source = stream_new((const unsigned char*)s, len);
  int success = _match_dfa_state(pattern, pattern->min_dfa_start_state_pos, source);
    
  stream_free(source);
  return success;
}

static int _match_dfa_state1(struct reg_pattern* pattern, uint8_t** states, size_t node_pos, struct reg_stream* source){
  int len = source->size;
  int total_lookup = 0;
  int total_nns = 0;
  for(int idx = 0; idx < len; idx++) {
    unsigned char c = source->buff[idx];
    struct reg_node* node = state_node_pos(pattern, node_pos);

    if (node->is_end && !pattern->is_match_tail) {
      return 1;
    }
    if (!states[node_pos]) {
      return 0;
    }
    size_t next_node_pos = states[node_pos][c];
    node_pos = next_node_pos;
    total_lookup ++;
    stream_next(source);
  }
  printf("all finish totel look up %d and total ns is %d\n", total_lookup, total_nns);
  return state_node_pos(pattern, node_pos)->is_end;
}

static int _match_dfa_state(struct reg_pattern* pattern, size_t node_pos, struct reg_stream* source){
  size_t state_list_len = list_len(pattern->state_list);
  for (size_t i = 1; i < state_list_len; i++) {
    struct reg_node* node = state_node_pos(pattern, i);
    assert(node != NULL);
  }

  //printf("_match_dfa_state: node_pos = %zu, state list size = %zu\n", node_pos, list_len(pattern->state_list));

  uint8_t **states = dfa_2D_vector(pattern);
  int t = 1;
  if (t) { clock_gettime(CLOCK_REALTIME, &start);}
  int len = source->size;
  int total_lookup = 0;
  int total_nns = 0;
  for(int idx = 0; idx < len; idx++) {
    unsigned char c = source->buff[idx];
    if (debug) { clock_gettime(CLOCK_REALTIME, &start);}
    struct reg_node* node = state_node_pos(pattern, node_pos);
    if (debug) {
      clock_gettime(CLOCK_REALTIME, &end);
      size_t tot_ns = (end.tv_sec - start.tv_sec) * 1000000000 + (end.tv_nsec - start.tv_nsec);
      printf("state_node_pos Overhead: Time per measurement = %.2f ns\n", (double)tot_ns);
      total_nns += tot_ns;
    }

    if (node->is_end && !pattern->is_match_tail) {
      if (t) {
        clock_gettime(CLOCK_REALTIME, &end);
        size_t tot_ns = (end.tv_sec - start.tv_sec) * 1000000000 + (end.tv_nsec - start.tv_nsec);
        printf("total look up %d, regex match overhead: time per measurement = %.2f ns\n", total_lookup, (double)tot_ns);
        total_nns += tot_ns;
      }
      return 1;
    }
    if (!states[node_pos]) {
      if (t) {
        clock_gettime(CLOCK_REALTIME, &end);
        size_t tot_ns = (end.tv_sec - start.tv_sec) * 1000000000 + (end.tv_nsec - start.tv_nsec);
        printf("total look up %d, regex match overhead: time per measurement = %.2f ns\n", total_lookup, (double)tot_ns);
        total_nns += tot_ns;
      }
      return 0;
    }
    size_t next_node_pos = states[node_pos][c];
    node_pos = next_node_pos;
    total_lookup ++;

    if (debug) { clock_gettime(CLOCK_REALTIME, &start);}
    stream_next(source);
    if (debug) {
      clock_gettime(CLOCK_REALTIME, &end);
      size_t tot_ns = (end.tv_sec - start.tv_sec) * 1000000000 + (end.tv_nsec - start.tv_nsec);
      printf("stream_next Overhead: Time per measurement = %.2f ns\n", (double)tot_ns);
      total_nns += tot_ns;
    }
  }
  printf("all finish totel look up %d and total ns is %d\n", total_lookup, total_nns);
  if (t) {
    clock_gettime(CLOCK_REALTIME, &end);
    size_t tot_ns = (end.tv_sec - start.tv_sec) * 1000000000 + (end.tv_nsec - start.tv_nsec);
    printf("regex match overhead: time per measurement = %.2f ns\n", (double)tot_ns);
  }
  return state_node_pos(pattern, node_pos)->is_end;
/* 
  for(;!stream_end(source);){
    printf("node_pos = %zu.\n", node_pos);
    // dump edge
    struct reg_node* node = state_node_pos(pattern, node_pos);
    struct reg_list* edges = node->edges;
    struct _reg_path* path = NULL;
    unsigned char c = stream_char(source);    

    if(node->is_end && !pattern->is_match_tail) return 1;

    for(size_t i=0; (path = list_idx(edges, i)); i++){
      struct reg_range* range = &(state_edge_pos(pattern, path->edge_pos)->range);
      size_t next_node_pos = path->next_node_pos;

      assert(range);
      printf("range: %c, %c. next node = %zu.\n", (char)(range->begin), (char)(range->end), next_node_pos);
      if(c >= range->begin && c<=range->end){ // range
        node_pos = next_node_pos;
        stream_next(source);
        break;
      }
    }

    if(!path) return 0;
  }

  // match is end of source
  return state_node_pos(pattern, node_pos)->is_end;
  */
}


static int _match_nfa_state(struct reg_pattern* pattern, size_t node_pos, struct reg_stream* source){
  // pass end state
  if(stream_end(source) && state_node_pos(pattern, node_pos)->is_end){ 
    return 1;
  }

  // dump edge
  struct reg_list* edges = state_node_pos(pattern, node_pos)->edges;
  struct _reg_path* path = NULL;
  unsigned char c = stream_char(source);


  int success = 0;
  for(size_t i=0; (path = list_idx(edges, i)); i++){
    struct reg_range* range = &(state_edge_pos(pattern, path->edge_pos)->range);
    size_t next_node_pos = path->next_node_pos;

    if(range == NULL){  //edsilone
      success = _match_nfa_state(pattern, next_node_pos, source);
    }else if(c >= range->begin && c<=range->end){ // range
      if(stream_next(source)){
        assert(success==0);
        success = _match_nfa_state(pattern, next_node_pos, source);
        stream_back(source);
      }
    }

    if(success) return 1; 
  }

  return 0;
}


