#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include "Logger.h"
#include "Position.h"


typedef struct BlockNode {
    uint32_t index;
    uint16_t distance;
} BlockNode;

BlockNode *block_node_new(uint32_t index, uint16_t distance) {
    BlockNode *node = malloc(sizeof(BlockNode));
    node->index = index;
    node->distance = distance;
    return node;
}

typedef struct PriorityQueue {
    size_t max_nodes;
    size_t size;
    BlockNode **nodes;
} PriorityQueue;


void swap_elems(BlockNode **array, uint32_t index_one, uint32_t index_two) {
    BlockNode *temp = array[index_one];
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

void sift_up(BlockNode **array, uint32_t index) {
    while (array[index]->distance < array[get_parent_index(index)]->distance) {
        swap_elems(array, index, get_parent_index(index));
    }
}

void sift_down(PriorityQueue *queue) {
    BlockNode **nodes = queue->nodes;
    uint32_t current_index = 0;
    while (is_index_in_bounds(queue, get_left_child_index(current_index))) {
        uint32_t min_index = current_index;
        if (nodes[get_left_child_index(current_index)]->distance < nodes[min_index]->distance) {
            min_index = get_left_child_index(current_index);
        }
        if (is_index_in_bounds(queue, get_right_child_index(current_index))) {
            if (nodes[get_right_child_index(current_index)]->distance < nodes[min_index]->distance) {
                min_index = get_right_child_index(current_index);
            }
        }
        if (min_index == current_index) break;
        swap_elems(nodes, current_index, min_index);
        current_index = min_index;
    }
}

void insert_node(PriorityQueue *queue, BlockNode *node) {
    if (queue->size + 1 > queue->max_nodes) {
        exit(EXIT_FAILURE);
    }
    queue->size++;
    queue->nodes[queue->size - 1] = node;
    sift_up(queue->nodes, queue->size - 1);
}

BlockNode *extract_min(PriorityQueue *queue) {
    BlockNode *value = queue->nodes[0];
    swap_elems(queue->nodes, 0, queue->size - 1);
    queue->size--;
    sift_down(queue);
    return value;
}

PriorityQueue *priority_queue_new(uint64_t max_nodes) {
    PriorityQueue *queue = malloc(sizeof(PriorityQueue));
    queue->size = 0;
    queue->max_nodes = max_nodes;
    queue->nodes = malloc(sizeof(BlockNode*) * max_nodes);
    return queue;
}

void priority_queue_free(PriorityQueue *queue) {
    if (queue == NULL) return;
    free(queue->nodes);
    free(queue);
}

typedef struct NodePosition {
    uint16_t x;
    uint16_t y;
    uint16_t z;
} NodePosition;

typedef struct SearchArea {
    Position *abs_pos_of_origin;
    uint16_t x_length;
    uint16_t y_length;
    uint16_t z_length;
} SearchArea;

NodePosition *index_to_rel_pos(SearchArea *area, uint16_t index_one) {
    uint16_t blocks_per_x = 1;
    uint16_t blocks_per_z = area->x_length;
    uint16_t blocks_per_y = area->x_length * area->y_length;
    uint16_t x_index = (index_one / blocks_per_x) % area->x_length;
    uint16_t z_index = (index_one / blocks_per_z) % area->z_length;
    uint16_t y_index = (index_one / blocks_per_y) % area->y_length;
    NodePosition *position = malloc(sizeof(NodePosition));
    position->x = x_index;
    position->y = y_index;
    position->z = z_index;
    return position;
}

uint32_t abs_pos_to_index(SearchArea *area, Position *pos) {
    int16_t x_delta = (int16_t) fabs((pos->x - area->abs_pos_of_origin->x));
    int16_t y_delta = (int16_t) fabs((pos->y - area->abs_pos_of_origin->y));
    int16_t z_delta = (int16_t) fabs((pos->z - area->abs_pos_of_origin->z));
    return x_delta + z_delta * area->x_length + y_delta * area->x_length * area->z_length;
}

uint32_t get_heuristic(NodePosition *pos_1, NodePosition *pos_2) {
    return abs(pos_1->x - pos_2->x)+ abs(pos_1->y - pos_2->y) + abs(pos_1->z - pos_2->z);
}

SearchArea *search_area_new(Position *pos_1, Position *pos_2) {
    uint16_t x_delta = (uint16_t) fabs((pos_1->x - pos_2->x));
    uint16_t y_delta = (uint16_t) fabs((pos_1->y - pos_2->y));
    uint16_t z_delta = (uint16_t) fabs((pos_1->z - pos_2->z));

    int16_t x_low = (int16_t) (pos_1->x < pos_2->x ? pos_1->x : pos_2->x);
    int16_t y_low = (int16_t) (pos_1->x < pos_2->y ? pos_1->y : pos_2->y);
    int16_t z_low = (int16_t) (pos_1->x < pos_2->z ? pos_1->z : pos_2->z);

    SearchArea *area = malloc(sizeof(SearchArea));
    area->abs_pos_of_origin = position_new(x_low, y_low, z_low, 0, 0);
    area->x_length = x_delta;
    area->y_length = y_delta;
    area->z_length = z_delta;

    return area;
}

bool position_equals(BlockNode *pos1, BlockNode *pos2) {
    return false;
}

void get_neighbours(BlockNode *node) {

}

Position** find_path(Position *start, Position *goal) {
    SearchArea *area = search_area_new(start, goal);
    PriorityQueue *queue = priority_queue_new(area->x_length * area->y_length * area->z_length);
    BlockNode *costs[area->x_length * area->y_length * area->z_length];
    BlockNode *parents[area->x_length * area->y_length * area->z_length];

    BlockNode *start_node = block_node_new(abs_pos_to_index(area, start), 0);
    BlockNode *goal_node = block_node_new(abs_pos_to_index(area, goal), 0);
    insert_node(queue, start_node);
    costs[start_node->index] = 0;
    parents[start_node->index] = start_node;

    BlockNode *neighbours = malloc(sizeof(BlockNode) * 4);

    while(queue->size != 0) {
        BlockNode *current = extract_min(queue);

        if (position_equals(current, goal_node)) {
            break;
        }

        for (int i = 0; i < )
    }
}