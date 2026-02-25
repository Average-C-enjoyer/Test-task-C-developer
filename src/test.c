#include "handle.h"

#define RUN_TEST(func) do {            \
    printf("Running %s...\n", #func);  \
    clock_t start = clock();           \
                                       \
    short rc = func();                 \
    if (rc == OK) {                    \
            printf("[PASS]\n");        \
            passed++;                  \
        } else {                       \
            printf("[FAIL]\n");        \
            print_status(rc);          \
            failed++;                  \
        }                              \
                                                             \
    clock_t end = clock();                                   \
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC; \
                                                             \
    printf("%s passed!\n", #func);                           \
    printf("Execution time: %.6f seconds\n", elapsed);       \
    } while(0)


#define TMP_A "tmp_a.bin"
#define TMP_B "tmp_b.bin"
#define TMP_OUT "tmp_out.bin"

static int passed = 0;
static int failed = 0;

static short test_basic() {
    const StatData in_a[2] = {
        {.id = 90889, .count = 13, .cost = 3.567, .primary = 0, .mode=3 },
        {.id = 90089, .count = 1,  .cost = 88.90, .primary = 1, .mode=0 }
    };

    const StatData in_b[2] = {
        {.id = 90089, .count = 13,   .cost = 0.011,   .primary = 0, .mode=2 },
        {.id = 90189, .count = 1000, .cost = 1.00003, .primary = 1, .mode=2 }
    };

    const StatData expected[3] = {
        {.id = 90189, .count = 1000, .cost = 1.00003, .primary = 1, .mode = 2 },
        {.id = 90889, .count = 13,   .cost = 3.567,   .primary = 0, .mode = 3 },
        {.id = 90089, .count = 14,   .cost = 88.911,  .primary = 0, .mode = 2 }
    };

    size_t dest_size = 0;
    short rc;

    rc = StoreDump(TMP_A, in_a, 2);
    if (rc != OK) {
        fprintf(stderr, "Test 1: failed to create file A\n");
        return rc;
    }
    
    rc = StoreDump(TMP_B, in_b, 2);
    if (rc != OK) {
        fprintf(stderr, "Test 1: failed to create file B\n");
        return rc;
    }

    rc = HandleData(TMP_A, TMP_B, TMP_OUT, &dest_size);
    if (rc != OK) {
        fprintf(stderr, "Test 1: HandleData failed\n");
        return rc;
    }

    StatData *result = NULL;
    size_t count = 0;

    rc = LoadDump(TMP_OUT, &result, &count); 
    if (rc != OK) {
        fprintf(stderr, "Test 1: failed to read output\n");
        return rc;
    }

    if (count != 3) {
        fprintf(stderr, "Test 1: wrong result size\n");
        free(result);
        return ERR_INV_SIZE;
    }

    for (size_t i = 0; i < count; i++) {
        if (!stat_equal(&result[i], &expected[i])) {
            fprintf(stderr, "Test 1: mismatch at index %zu\n", i);
            free(result);
            return ERR_INV_DATA;
        }
    }

    free(result);  

    return OK;
}

// Test for empty input files
static short test_empty()
{
    StoreDump(TMP_A, NULL, 0);
    StoreDump(TMP_B, NULL, 0);

    size_t dest_size = 0;

    short rc = HandleData(TMP_A, TMP_B, TMP_OUT, &dest_size);
    if (rc != OK) return rc;

    return OK;
}

// Test for corrupted file
static short test_corrupted()
{
    int fd = open(TMP_A, O_WRONLY|O_CREAT|O_TRUNC, 0644);
    write(fd, "abc", 3); // 3 isn't multiples of sizeof(StatData)
    close(fd);

    size_t out_size = 0;

    return HandleData(TMP_A, TMP_A, TMP_OUT, &out_size) == ERR_FILE_READ
           ? OK : ERR_INV_DATA;
}

// Test for correct mode merging
static short test_mode_max()
{
    const StatData a[1] = {{1,1,1.0,1,1}};
    const StatData b[1] = {{1,1,1.0,1,7}};

    size_t dest_size = 0;

    StoreDump(TMP_A, a, 1);
    StoreDump(TMP_B, b, 1);

    HandleData(TMP_A, TMP_B, TMP_OUT, &dest_size);

    StatData *result;
    size_t count;

    LoadDump(TMP_OUT, &result, &count);

    short rc = (result[0].mode == 7) ? OK : ERR_INV_DATA;
    free(result);
    return rc;
}

// Test for arrays with max allowed size
static short test_large()
{
    size_t n = 100000;

    StatData *a = malloc(n * sizeof(StatData));
    StatData *b = malloc(n * sizeof(StatData));

    for (size_t i = 0; i < n; ++i) {
        a[i].id = i;
        a[i].count = 1;
        a[i].cost = (float)i;
        a[i].primary = 1;
        a[i].mode = 1;

        b[i] = a[i];
    }

    StoreDump(TMP_A, a, n);
    StoreDump(TMP_B, b, n);

    size_t dest_size = 0;

    if (HandleData(TMP_A, TMP_B, TMP_OUT, &dest_size) != OK)
        return ERR_INV_DATA;

    if (dest_size != n)
        return ERR_INV_SIZE;

    StatData *result = NULL;
    size_t count = 0;

    if (LoadDump(TMP_OUT, &result, &count) != OK)
        return ERR_INV_DATA;

    for (size_t i = 0; i < n; ++i) {

        if (result[i].id != (long)i)
            return ERR_INV_DATA;

        if (result[i].count != 2)
            return ERR_INV_DATA;

        if (result[i].cost != (float)(2*i))
            return ERR_INV_DATA;

        if (result[i].primary != 1)
            return ERR_INV_DATA;

        if (result[i].mode != 1)
            return ERR_INV_DATA;
    }

    free(result);
    free(a);
    free(b);

    return OK;
}

int main(void)
{
    RUN_TEST(test_basic);
    RUN_TEST(test_empty);
    RUN_TEST(test_corrupted);
    RUN_TEST(test_mode_max);
    RUN_TEST(test_large);

    printf("\nPassed: %d\n", passed);
    printf("Failed: %d\n", failed);

    return failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

