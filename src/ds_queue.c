#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "ds_queue.h"

void ds_queue_init(struct ds_queue_t *q) {
  // ds_queue_printf("Initializing queue %p\n", q);
  q->head = NULL;
  q->tail = NULL;
  q->count = 0;
}

void ds_queue_add(struct ds_queue_t *q, int data) {
  // Create a new null-terminated node
  struct ds_qnode_t *new_node = malloc(sizeof(struct ds_qnode_t));
  assert(new_node != NULL);

  new_node->data = data;
  new_node->next = NULL;

  // If the queue is empty
  if (q->head == NULL) {
    q->head = new_node;
    q->tail = new_node;
  } else {
    q->tail->next = new_node;
    q->tail = new_node;
  }

  q->count++;
}

int ds_queue_remove(struct ds_queue_t *q) {
  int data;
  assert(q->head != NULL);

  struct ds_qnode_t *old_head;
  old_head = q->head;
  data = old_head->data;

  q->head = q->head->next;
  q->count--;

  if (q->head == NULL) {
    assert(q->count == 0);
    q->tail = NULL;
  }

  free(old_head);

  return data;
}

int ds_queue_is_empty(struct ds_queue_t *q) {
  if (q->count == 0) return 1;
  return 0;
}

int ds_queue_size(struct ds_queue_t *q) { return q->count; }

void ds_queue_free(struct ds_queue_t *q) {
  while (ds_queue_size(q) != 0) ds_queue_remove(q);
}

void ds_queue_print(struct ds_queue_t *q) {
  assert(q != NULL);
  struct ds_qnode_t *t = q->head;
  while (t != NULL) {
    printf("%d ", t->data);
    t = t->next;
  }
  printf("\n");
}
