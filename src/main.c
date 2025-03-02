#include <stdio.h>
#include <stdlib.h>

#include "const.h"
#include "debug.h"

int main(int argc, char **argv) {
    if(validargs(argc, argv)) {
        // If returns -1(fail), which is true.
        debug("Options: 0x%x", global_options);
        USAGE(*argv, EXIT_FAILURE);
    }


    int flagC = 0x2;
    int flagD = 0x4;
    debug("Options: 0x%x", global_options);
    if(global_options & 1) {
        USAGE(*argv, EXIT_SUCCESS);
    }
    else if(global_options & flagC) {
        int ret = 0;
        ret = compress(stdin, stdout, (global_options>>16));

        if(ret == EOF) {
            USAGE(*argv, EXIT_FAILURE);
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;

    }
    else if(global_options & flagD) {
        int ret = decompress(stdin, stdout);
        if(ret == EOF) {
            USAGE(*argv, EXIT_FAILURE);
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }

    return EXIT_SUCCESS;

}

/*
 * Just a reminder: All non-main functions should
 * be in another file not named main.c
 */
