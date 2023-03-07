#include <stdlib.h>
#include <stdbool.h>
#include <stdint-gcc.h>

typedef struct PriorityQueue {
    size_t size;
    uint16_t *distances;
} PriorityQueue;


void swap_elems(uint16_t *array, uint32_t index_one, uint32_t index_two) {
    uint16_t temp = array[index_one];
    array[index_one] = array[index_two];
    array[index_two] = temp;
}

uint32_t get_parent_index(uint32_t child_index) {
    return (child_index - 1) / 2;
}

uint32_t get_left_child_index(uint32_t parent_index) {
    return parent_index * 2 + 1;
}

uint32_t get_right_child_index(uint32_t parent_index) {
    return parent_index * 2 + 2;
}

bool is_index_in_bounds(PriorityQueue *queue, uint32_t index) {
    return index < queue->size;
}

void sift_up(uint16_t *array, uint32_t index) {
    while (array[index] < array[get_parent_index(index)]) {
        swap_elems(array, index, get_parent_index(index));
    }
}

void sift_down(PriorityQueue *queue) {
    uint16_t *distances = queue->distances;
    uint32_t current_index = 0;
    while (is_index_in_bounds(queue, get_left_child_index(current_index))) {
        uint32_t min_index = current_index;
        if (queue->distances[get_left_child_index(current_index)] < queue->distances[min_index]) {
            min_index = get_left_child_index(current_index);
        }
        if (is_index_in_bounds(queue, get_right_child_index(current_index))) {
            if (queue->distances[get_right_child_index(current_index)] < queue->distances[min_index]) {
                min_index = get_right_child_index(current_index);
            }
        }
        if (min_index == current_index) break;
        swap_elems(distances, current_index, min_index);
        current_index = min_index;
    }
}

void decrease_key(PriorityQueue *queue, uint32_t index, uint32_t value) {
    queue->distances[index] = value;
    sift_up(queue->distances, index);
}

uint16_t extract_min(PriorityQueue *queue) {
    uint16_t value = queue->distances[0];
    swap_elems(queue->distances, 0, queue->size - 1);
    queue->size--;
    sift_down(queue);
    return value;
}

PriorityQueue *priority_queue_new(uint16_t size, uint16_t* array) {
    PriorityQueue *queue = malloc(sizeof(PriorityQueue));
    queue->size = size;
    queue->distances = array;
    for (int i = 0; i < size / 2; i++) {
        sift_down(queue);
    }
    return queue;
}

void priority_queue_free(PriorityQueue *queue) {
    if (queue == NULL) return;
    free(queue->distances);
    free(queue);
}