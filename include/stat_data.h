#ifndef STAT_DATA_H
#define STAT_DATA_H

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

#endif // STAT_DATA_H
