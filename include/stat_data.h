#ifndef STAT_DATA_H
#define STAT_DATA_H

#include <stdio.h>

// Error codes
typedef enum {
    OK              =  0,
    ERR_FILE_OPEN   = -1,
    ERR_FILE_WRITE  = -2,
    ERR_FILE_READ   = -3,
    ERR_NOMEM       = -4,
    ERR_MERGE       = -5,
    ERR_NULL        = -6,
    ERR_INV_SIZE    = -7,
    ERR_INV_DATA    = -8
} status;

// Struct for binary data
typedef struct StatData {
    long            id;
    int             count;
    float           cost;
    unsigned int    primary : 1;
    unsigned int    mode    : 3;
} StatData;

// utility func
extern inline int stat_equal(const StatData *a, const StatData *b) {
    return a->id == b->id &&
           a->count == b->count &&
           a->cost == b->cost &&
           a->primary == b->primary &&
           a->mode == b->mode;
}

extern inline void print_status(short rc) {
    switch (rc) {
        case OK: printf("OK\n"); break;
        case ERR_FILE_OPEN: printf("Error opening file\n"); break;
        case ERR_FILE_WRITE: printf("Error writing to file\n"); break;
        case ERR_FILE_READ: printf("Error reading from file\n"); break;
        case ERR_NOMEM: printf("Memory allocation failed\n"); break;
        case ERR_MERGE: printf("Error merging data\n"); break;
        case ERR_NULL: printf("Null pointer error\n"); break;
        case ERR_INV_SIZE: printf("Invalid size error\n"); break;
        case ERR_INV_DATA: printf("Invalid data error\n"); break;
        default: printf("Unknown error code: %d\n", rc); break;
	}
}

#endif // STAT_DATA_H
