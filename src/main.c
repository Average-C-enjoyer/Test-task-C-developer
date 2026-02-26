#include "handle.h"
#include "stat_data.h"
#include <complex.h>

int main(int argc, char **argv)
{
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <file1> <file2> <output>\n", argv[0]);
        return EXIT_FAILURE;
    }

    size_t dest_size = 0;
    short rc = HandleData(argv[1], argv[2], argv[3], &dest_size);
    if (rc != OK) print_status(rc); return rc;

    return EXIT_SUCCESS;
}
