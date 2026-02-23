#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "stat_data.h"
#include "hash_table.h"
#include "quick_sort.h"

#define DEBUG

#ifdef DEBUG
static inline void print_stat(const StatData *data) {
    printf("ID: %ld, Count: %d, Cost: %.2f, Primary: %u, Mode: %u\n",
        data->id, data->count, data->cost, data->primary, data->mode);
}
#endif // DEBUG

// StoreDump writes in binary format
static inline short StoreDump(const char *dest, const StatData *data, size_t count) {
    int fd = open(dest, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    char buffer[256];

    if (fd == -1) {
        perror("Error opening file");
        return ERR_FILE_OPEN;
    }

    if (write(fd, data, sizeof(StatData) * count) == -1) {
        perror("Error writing to file");
        close(fd);
        return ERR_FILE_WRITE;
    }

    close(fd);
    return OK;
}

// LoadDump allocates memory for out_data, caller must free()
// Reads entire file because in tech task max amount of writes - 100000.
// 100000 * 24 = 2.4MB. Small enough
short LoadDump(const char *src, StatData **out_data, size_t *out_count)
{
    int fd = open(src, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        return ERR_FILE_OPEN;
    }

	// Get file size
    off_t file_size = lseek(fd, 0, SEEK_END);
    if (file_size == -1) {
        perror("lseek failed");
        close(fd);
        return ERR_FILE_OPEN;
    }

    if (file_size % sizeof(StatData) != 0) {
        fprintf(stderr, "Corrupted file: size is not multiple of StatData\n");
        close(fd);
        return ERR_FILE_READ;
    }

    size_t count = file_size / sizeof(StatData);

	// Reset file pointer to beginning
    if (lseek(fd, 0, SEEK_SET) == -1) {
        perror("lseek reset failed");
        close(fd);
        return ERR_FILE_READ;
    }

    StatData *data = malloc(file_size);
    if (!data) {
        perror("malloc failed");
        close(fd);
        return ERR_NOMEM;
    }

	// Read file in a loop to handle partial reads
    size_t total_read = 0;
    while (total_read < file_size) {
        ssize_t n = read(fd,
            (char *)data + total_read,
            file_size - total_read);

        if (n == -1) {
            perror("read failed");
            free(data);
            close(fd);
            return ERR_FILE_READ;
        }

        if (n == 0)
			break; // EOF

        total_read += n;
    }

    if (total_read != file_size) {
        fprintf(stderr, "Unexpected EOF\n");
        free(data);
        close(fd);
        return ERR_FILE_READ;
    }

    close(fd);

    *out_data = data;
    *out_count = count;

    return OK;
}


short JoinDump(const StatData *src1, size_t count1,
    const StatData *src2, size_t count2,
    StatData *dest, size_t *curr_idx)
{
    if (!src1 || !src2 || !dest) return ERR_NULL;
    if (!count1 || !count2) return ERR_INV_SIZE;

    HashTable ht;
    ht_init(&ht, count1, count2);

    *curr_idx = 0;
    for (size_t i = 0; i < count1; i++) {
        ht_merge_or_insert(&ht, &src1[i], dest, curr_idx);
    }

    for (size_t i = 0; i < count2; i++) {
        ht_merge_or_insert(&ht, &src2[i], dest, curr_idx);
    }

    ht_free(&ht);
    return OK;
}


short SortDump(StatData *data, size_t count) {
    if (!data) return ERR_NULL;

    if (count == 0) return ERR_INV_SIZE;

    quick_sort(data, 0, (long)count - 1);

    return OK;
}


short HandleData(const char *path1,
                 const char *path2,
                 const char *dest_path, 
                 size_t *dest_size)
{
    if (!path1 || !path2 || !dest_path)
        return ERR_NULL;

    StatData *data1 = NULL;
    StatData *data2 = NULL;
    size_t count1 = 0;
    size_t count2 = 0;

    short rc;

    // Load first file
    rc = LoadDump(path1, &data1, &count1);
    if (rc != OK)
        return rc;

    // Load second file
    rc = LoadDump(path2, &data2, &count2);
    if (rc != OK) {
        goto cleanup;
    }

    // Allocate result buffer (max possible size)
    size_t max_count = count1 + count2;

    StatData *merged = malloc(max_count * sizeof(StatData));
    if (!merged) {
        rc = ERR_NOMEM;
        goto cleanup;
    }

    memset(merged, 0, max_count * sizeof(StatData));

    // Merge
    rc = JoinDump(data1, count1, data2, count2, merged, dest_size);
    if (rc != OK) {
        goto cleanup;
    }

    // Sort
    rc = SortDump(merged, *dest_size);
    if (rc != OK) {
        goto cleanup;
    }

    // Print first 10
    size_t to_print = *dest_size < 10 ? *dest_size : 10;

    printf("------------------------------------------------------------------\n");
    printf("| ID(hex) | Count | Cost(exp)    | Primary | Mode(bin) |\n");
    printf("------------------------------------------------------------------\n");

    for (size_t i = 0; i < to_print; ++i) {

        // mode -> binary
        char mode_bin[4] = {0};
        unsigned int mode = merged[i].mode;

        int idx = 0;
        for (int bit = 2; bit >= 0; --bit) {
            if ((mode >> bit) & 1)
                mode_bin[idx++] = '1';
            else if (idx)
                mode_bin[idx++] = '0';
        }
        if (idx == 0)
            mode_bin[idx++] = '0';
        mode_bin[idx] = '\0';

        printf("| %-7lx | %-5d | %10.3e | %-7c | %-8s |\n",
            merged[i].id,
            merged[i].count,
            merged[i].cost,
            merged[i].primary ? 'y' : 'n',
            mode_bin);
    }

    printf("------------------------------------------------------------------\n");

    // Save result
    rc = StoreDump(dest_path, merged, *dest_size);

cleanup:
    if (data1) free(data1);
    if (data2) free(data2);
    if (merged) free(merged);

    return rc;
}


short TestData(const StatData *in_a, 
               const StatData *in_b, 
               const StatData *out, size_t *dest_size)
{
    clock_t start = clock();

    const char *file_a = "test_a.bin";
    const char *file_b = "test_b.bin";
    const char *file_out = "test_out.bin";

    if (StoreDump(file_a, in_a, 2) != OK) {
        fprintf(stderr, "Test 1: failed to create file A\n");
        return ERR_FILE_WRITE;
    }

    if (StoreDump(file_b, in_b, 2) != OK) {
        fprintf(stderr, "Test 1: failed to create file B\n");
        return ERR_FILE_WRITE;
    }

    if (HandleData(file_a, file_b, file_out, dest_size) != OK) {
        fprintf(stderr, "Test 1: HandleData failed\n");
        return ERR_FILE_WRITE;
    }

    StatData *result = NULL;
    size_t result_count = 0;

    if (LoadDump(file_out, &result, &result_count) != OK) {
        fprintf(stderr, "Test 1: failed to read output\n");
        return ERR_FILE_READ;
    }

    if (result_count != 3) {
        fprintf(stderr, "Test 1: wrong result size\n");
        free(result);
        return ERR_INV_SIZE;
    }

    for (size_t i = 0; i < result_count; i++) {
        if (!stat_equal(&result[i], &out[i])) {
            fprintf(stderr, "Test 1: mismatch at index %zu\n", i);
            free(result);
            return ERR_INV_DATA;
        }
    }

    free(result);

    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;

    printf("All tests passed!\n");
    printf("Execution time: %.6f seconds\n", elapsed);

    return OK;
}


int main() {
    // Tests
    const StatData case_1_in_a[2] = {
        {.id = 90889, .count = 13, .cost = 3.567, .primary = 0, .mode=3 },
        {.id = 90089, .count = 1,  .cost = 88.90, .primary = 1, .mode=0 }
    };

    const StatData case_1_in_b[2] = {
        {.id = 90089, .count = 13,   .cost = 0.011,   .primary = 0, .mode=2 },
        {.id = 90189, .count = 1000, .cost = 1.00003, .primary = 1, .mode=2 }
    };

    const StatData case_1_out[3] = {
        {.id = 90189, .count = 1000, .cost = 1.00003, .primary = 1, .mode = 2 },
        {.id = 90889, .count = 13,   .cost = 3.567,   .primary = 0, .mode = 3 },
        {.id = 90089, .count = 14,   .cost = 88.911,  .primary = 0, .mode = 2 }
    };

    size_t dest_size = 0;

    TestData(case_1_in_a, case_1_in_b, case_1_out, &dest_size);

    return EXIT_SUCCESS;
}

