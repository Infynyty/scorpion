#include <stdlib.h>
#include "AStar.h"
#include <math.h>

typedef struct PriorityQueue {
    size_t size;
    uint16_t *distances;
} PriorityQueue;

PriorityQueue *priority_queue_new(uint16_t ) {
    PriorityQueue *queue = malloc(sizeof(PriorityQueue));
    return queue;
}

void priority_queue_free(PriorityQueue *queue) {
    if (queue == NULL) return;
    free(queue->distances);
    free(queue);
}

int32_t get_parent_index(int32_t child_index) {
    return ceil((child_index / 2.0) - 1);
}

int32_t get_child_index(int32_t parent_index) {
    return parent_index * 2 + 1;
}

void sift_up() {

}

void sift_down() {

}

void insert_element(PriorityQueue *queue) {
    // add to end, sift up
}

void restore_heap_condition(PriorityQueue *queue) {
    // sift up
}

uint16_t poll_element(PriorityQueue *queue) {
    //remove sift down
}

Position* find_path(Position *start, Position *goal) {

}