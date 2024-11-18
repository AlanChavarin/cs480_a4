

void init_queue(Queue* queue, int size);
void push_to_queue(Queue* queue, RequestType request);
RequestType pop_from_queue(Queue* queue);
void* general_greeter(void* args);
int is_empty(Queue* queue);
void* concierge_robot(void* args);