struct ds_qnode_t {
  int data;
  struct ds_qnode_t *next;
};

struct ds_queue_t {
  struct ds_qnode_t *head, *tail;
  int count;
};

void ds_queue_init(struct ds_queue_t *q);
void ds_queue_add(struct ds_queue_t *q, int data);
int ds_queue_remove(struct ds_queue_t *q);
int ds_queue_size(struct ds_queue_t *q);
void ds_queue_free(struct ds_queue_t *q);
int ds_queue_is_empty(struct ds_queue_t *q);
void ds_queue_print(struct ds_queue_t *q);
