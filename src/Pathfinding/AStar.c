#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include "Logger.h"
#include "Position.h"
#include "WorldState.h"

#define SEARCH_AREA_SIZE 8

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
    uint32_t current_index = index;
    while (current_index != 0) {
        if (current_index < 0 || get_parent_index(current_index) < 0) {
            sc_log(ERR, "Incorrect index reading");
        }
        if (current_index > 99999 || get_parent_index(current_index) > 99999) {
            sc_log(ERR, "Incorrect index reading");
        }
        if (array[current_index]->distance > array[get_parent_index(current_index)]->distance) break;
        swap_elems(array, current_index, get_parent_index(current_index));
        current_index = get_parent_index(current_index);
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
    uint16_t current_distance;
    Position *abs_pos_of_origin;
    uint16_t x_length;
    uint16_t y_length;
    uint16_t z_length;
} SearchArea;

void search_area_free(SearchArea *area) {
    if (area == NULL) return;
    position_free(area->abs_pos_of_origin);
    free(area);
}

NodePosition *index_to_rel_pos(SearchArea *area, uint16_t index_one) {
    uint16_t blocks_per_x = 1;
    uint16_t blocks_per_z = area->x_length;
    uint16_t blocks_per_y = (area->x_length) * (area->z_length);
    uint16_t x_index = (index_one / blocks_per_x) % area->x_length;
    uint16_t z_index;
    uint16_t y_index;
    if (blocks_per_z != 0 && area->z_length) {
        z_index = (index_one / blocks_per_z) % area->z_length;
    } else {
        z_index = 0;
    }
    if (blocks_per_y != 0 && area->y_length) {
        y_index = (index_one / blocks_per_y) % area->y_length;
    } else {
        y_index = 0;
    }

    NodePosition *position = malloc(sizeof(NodePosition));
    position->x = x_index;
    position->y = y_index;
    position->z = z_index;
    return position;
}

Position *index_to_abs_pos(SearchArea *area, uint16_t index) {
    NodePosition *rel_pos = index_to_rel_pos(area, index);
    double x = rel_pos->x + area->abs_pos_of_origin->x;
    double y = rel_pos->y + area->abs_pos_of_origin->y;
    double z = rel_pos->z + area->abs_pos_of_origin->z;
    free(rel_pos);
    return position_new(x, y, z, 0, 0);
}

uint32_t abs_pos_to_index(SearchArea *area, Position *pos) {
    int16_t x_delta = (int16_t) fabs((pos->x - area->abs_pos_of_origin->x));
    int16_t y_delta = (int16_t) fabs((pos->y - area->abs_pos_of_origin->y));
    int16_t z_delta = (int16_t) fabs((pos->z - area->abs_pos_of_origin->z));
    return x_delta + z_delta * area->x_length + y_delta * area->x_length * area->z_length;
}

uint32_t get_heuristic(NodePosition *pos_1, NodePosition *pos_2) {
    return abs(pos_1->x - pos_2->x) + abs(pos_1->y - pos_2->y) + abs(pos_1->z - pos_2->z);
}

SearchArea *search_area_new(Position *pos_1, Position *pos_2) {
    uint16_t x_delta = (uint16_t) fabs((pos_1->x - pos_2->x)) + 1 + 2 *SEARCH_AREA_SIZE;
    uint16_t y_delta = (uint16_t) fabs((pos_1->y - pos_2->y)) + 1 + 2 *SEARCH_AREA_SIZE;
    uint16_t z_delta = (uint16_t) fabs((pos_1->z - pos_2->z)) + 1 + 2 *SEARCH_AREA_SIZE;

    int16_t x_low = (int16_t) (pos_1->x < pos_2->x ? pos_1->x - SEARCH_AREA_SIZE : pos_2->x - SEARCH_AREA_SIZE);
    int16_t y_low = (int16_t) (pos_1->x < pos_2->y ? pos_1->y - SEARCH_AREA_SIZE : pos_2->y - SEARCH_AREA_SIZE);
    int16_t z_low = (int16_t) (pos_1->x < pos_2->z ? pos_1->z - SEARCH_AREA_SIZE : pos_2->z - SEARCH_AREA_SIZE);

    SearchArea *area = malloc(sizeof(SearchArea));
    area->current_distance = 1;
    area->abs_pos_of_origin = position_new(x_low, y_low, z_low, 0, 0);
    area->x_length = x_delta;
    area->y_length = y_delta;
    area->z_length = z_delta;

    return area;
}

bool position_equals(BlockNode *pos1, BlockNode *pos2) {
    return pos1->index == pos2->index;
}

bool is_pos_in_area(Position *pos, SearchArea *area) {
    return pos->x >= area->abs_pos_of_origin->x && pos->x <= area->abs_pos_of_origin->x + area->x_length
        && pos->y >= area->abs_pos_of_origin->y && pos->y <= area->abs_pos_of_origin->y + area->y_length
        && pos->z >= area->abs_pos_of_origin->z && pos->z <= area->abs_pos_of_origin->z + area->z_length;
}

void get_neighbours(BlockNode *node, BlockNode **neighbours, SearchArea *area, WorldState *state) {
    Position *abs_pos = index_to_abs_pos(area, node->index);
    Position *abs_pos1 = position_new(abs_pos->x + 1, abs_pos->y, abs_pos->z, 0, 0);
    Position *abs_pos2 = position_new(abs_pos->x - 1, abs_pos->y, abs_pos->z, 0, 0);
    Position *abs_pos3 = position_new(abs_pos->x, abs_pos->y, abs_pos->z + 1, 0, 0);
    Position *abs_pos4 = position_new(abs_pos->x, abs_pos->y, abs_pos->z - 1, 0, 0);
    if (strcmp(get_block_at(abs_pos1, state)->name, "minecraft:air") != 0 && is_pos_in_area(abs_pos1, area)) {
        neighbours[0] = block_node_new(abs_pos_to_index(area, abs_pos1), area->current_distance);
    } else {
        neighbours[0] = NULL;
        position_free(abs_pos1);
    }
    if (strcmp(get_block_at(abs_pos2, state)->name, "minecraft:air") != 0 && is_pos_in_area(abs_pos2, area)) {
        neighbours[1] = block_node_new(abs_pos_to_index(area, abs_pos2), area->current_distance);
    } else {
        neighbours[1] = NULL;
        position_free(abs_pos2);
    }
    if (strcmp(get_block_at(abs_pos3, state)->name, "minecraft:air") != 0 && is_pos_in_area(abs_pos3, area)) {
        neighbours[2] = block_node_new(abs_pos_to_index(area, abs_pos3), area->current_distance);
    } else {
        neighbours[2] = NULL;
        position_free(abs_pos3);
    }
    if (strcmp(get_block_at(abs_pos4, state)->name, "minecraft:air") != 0 && is_pos_in_area(abs_pos4, area)) {
        neighbours[3] = block_node_new(abs_pos_to_index(area, abs_pos4), area->current_distance);
    } else {
        neighbours[3] = NULL;
        position_free(abs_pos4);
    }
}

void add_neighbour();

Position **translate_parents(uint32_t *parents, uint32_t goal_index, uint32_t start_index, SearchArea *area, uint32_t *steps) {
    uint32_t counter = 1;
    uint32_t current = goal_index;
    while (current != start_index) {
        counter++;
        current = parents[current];
    }
    memmove(steps, &counter, sizeof(uint32_t));
    Position **positions = malloc(sizeof(Position *) * counter);
    current = goal_index;
    for (int i = 0; i < counter - 1; i++) {
        Position *pos = index_to_abs_pos(area, current);
        pos->x += 0.5f;
        pos->z += 0.5f;
        positions[counter - 1 - i] = pos;
        current = parents[current];
    }
    Position *pos = index_to_abs_pos(area, current);
    pos->x += 0.5f;
    pos->z += 0.5f;
    positions[0] = pos;
    return positions;
}

void print_result(Position **positions, uint16_t count) {
    for (int i = 0; i < count; i++) {
        sc_log(INFO, "Go to X %lf Y %lf Z %lf.", positions[i]->x, positions[i]->y, positions[i]->z);
    }
}

Position** find_path(Position *start, Position *goal, WorldState *state, uint32_t *counter) {
    Position *start_copy = position_new(start->x, start->y, start->z, start->pitch, start->yaw);
    start_copy->x = floor(start_copy->x);
    start_copy->y = floor(start_copy->y - 1);
    start_copy->z = floor(start_copy->z);

    SearchArea *area = search_area_new(start_copy, goal);
    PriorityQueue *queue = priority_queue_new(100000);
    uint16_t *costs = malloc(sizeof(uint16_t) * area->x_length * area->y_length * area->z_length);
    uint32_t *parents = malloc(sizeof(uint32_t) * area->x_length * area->y_length * area->z_length);
    for (int i = 0; i < area->x_length * area->y_length * area->z_length; i++) {
        costs[i] = UINT16_MAX;
    }

    BlockNode *start_node = block_node_new(abs_pos_to_index(area, start_copy), 0);
    BlockNode *goal_node = block_node_new(abs_pos_to_index(area, goal), 0);
    uint32_t start_index = start_node->index;
    uint32_t goal_index = goal_node->index;
    insert_node(queue, start_node);
    costs[start_node->index] = 1;
    parents[start_node->index] = start_node->index;

    BlockNode **neighbours = malloc(sizeof(BlockNode) * 4);
    for (int i = 0; i < 4; i++) {
        neighbours[i] = NULL;
    }

    while(queue->size != 0) {
        BlockNode *current = extract_min(queue);
        area->current_distance++;

        if (position_equals(current, goal_node)) {
            break;
        }
        get_neighbours(current, neighbours, area, state);
        for (int i = 0; i < 4; i++) {
            if (neighbours[i] != NULL) {
                if (costs[neighbours[i]->index] == 0 || costs[neighbours[i]->index] > area->current_distance) {
                    neighbours[i]->distance =
                            area->current_distance
                            + get_heuristic(
                                    index_to_rel_pos(area, neighbours[i]->index),
                                    index_to_rel_pos(area, goal_node->index)
                                    );
                    costs[neighbours[i]->index] = area->current_distance;
                    parents[neighbours[i]->index] = current->index;
                    insert_node(queue, neighbours[i]);
                }
            }
        }
        free(current);
    }
    if (costs[goal_node->index] == UINT16_MAX) return NULL;
    uint32_t steps = 0;
    Position **result = translate_parents(parents, goal_index, start_index, area, &steps);
    memmove(counter, &steps, sizeof(uint32_t));
    free(parents);
    free(costs);
    position_free(start_copy);
    search_area_free(area);
    priority_queue_free(queue);
    print_result(result, steps);
    return result;
}