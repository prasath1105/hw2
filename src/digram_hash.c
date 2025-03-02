#include "const.h"
#include "sequitur.h"

/*
 * Digram hash table.
 *
 * Maps pairs of symbol values to first symbol of digram.
 * Uses open addressing with linear probing.
 * See, e.g. https://en.wikipedia.org/wiki/Open_addressing
 */

// Function prototypes
int isDigramMatchValues(SYMBOL *digram, int v1, int v2);



/**
 * Clear the digram hash table.
 */
void init_digram_hash(void) {
    int count = 0;
    while(count < MAX_DIGRAMS) {
        *(digram_table + count) = NULL;
        count++;
    }
}

/**
 * Look up a digram in the hash table.
 *
 * @param v1  The symbol value of the first symbol of the digram.
 * @param v2  The symbol value of the second symbol of the digram.
 * @return  A pointer to a matching digram (i.e. one having the same two
 * symbol values) in the hash table, if there is one, otherwise NULL.
 */
SYMBOL *digram_get(int v1, int v2) {
    int index = DIGRAM_HASH(v1, v2);
    int ret = 0;
    SYMBOL *sym = NULL;

    // Loop part 1
    for(int i = index; i < MAX_DIGRAMS; i++) {
        sym = *(digram_table + i);
        if(sym == NULL) {
            return NULL;
        }
        else if(sym == TOMBSTONE) {
            continue;
        }

        ret = isDigramMatchValues(sym, v1, v2);
        if(ret) {
            return sym;
        }
    }

    // Loop part 2
    for(int i = 0; i < index; i++) {
        sym = *(digram_table + i);
        if(sym == NULL) {
            return NULL;
        }
        else if(sym == TOMBSTONE) {
            continue;
        }

        ret = isDigramMatchValues(sym, v1, v2);
        if(ret) {
            return sym;
        }
    }

    return NULL;
}

/**
 * Deterimes if the digram symbol values match the given values
 * Helper function for digram_get
 *
 * @param SYMBOL *digram This digram to check the values of and see if it is similar to v1 and v2
 * @param int v1 The first symbol's value
 * @param int v2 The second symbol's value
 * @return 0 if nothing found, 1 if found match
 */
int isDigramMatchValues(SYMBOL *digram, int v1, int v2) {
    SYMBOL *sym1 = digram;
    SYMBOL *sym2 = (*digram).next;
    if((sym1 = NULL) || (sym2 = NULL)) {
        // Error checking
        return 0;
    }

    int sym1val = (*sym1).value;
    int sym2val = (*sym2).value;
    if((sym1val == v1) && (sym2val == v2)) {
        return 1;
    }

    return 0;
}

/**
 * Delete a specified digram from the hash table.
 *
 * @param digram  The digram to be deleted.
 * @return 0 if the digram was found and deleted, -1 if the digram did
 * not exist in the table.
 *
 * Note that deletion in an open-addressed hash table requires that a
 * special "tombstone" value be left as a replacement for the value being
 * deleted.  Tombstones are treated as vacant for the purposes of insertion,
 * but as filled for the purpose of lookups.
 *
 * Note also that this function will only delete the specific digram that is
 * passed as the argument, not some other matching digram that happens
 * to be in the table.  The reason for this is that if we were to delete
 * some other digram, then somebody would have to be responsible for
 * recycling the symbols contained in it, and we do not have the information
 * at this point that would allow us to be able to decide whether it makes
 * sense to do it here.
 */
int digram_delete(SYMBOL *digram) {
    SYMBOL *disym1 = NULL;
    SYMBOL *disym2 = NULL;
    SYMBOL *sym1 = digram;
    SYMBOL *sym2 = (*digram).next;
    int sym1val = (*sym1).value;
    int sym2val = (*sym2).value;
    int index = DIGRAM_HASH(sym1val, sym2val);

    // Input debug
    debug("digram_delete: digramValue: %d, digramValue2: %d", sym1val, sym2val);

    // TODO: Need to test for null within the loops

    // Loop part 1
    for(int i = index; i < MAX_DIGRAMS; i++) {

        debug("index: %d", i);

        disym1 = *(digram_table + i);
        if(disym1 == NULL) {
            return -1;
        }
        else if(disym1 == TOMBSTONE) {
            continue;
        }

        debug("disym1 is not null or tombstone");

        disym2 = (*disym1).next;
        if(disym2 == NULL) {
            return -1;
        }

        debug("disym2 is not null");

        if((disym1 == sym1) && (disym2 == sym2)) {
            *(digram_table + i) = TOMBSTONE;
            return 0;
        }
    }

    // Loop part 2
    for(int i = 0; i < index; i++) {
        debug("index: %d", i);

        disym1 = *(digram_table + i);
        if(disym1 == NULL) {
            return -1;
        }
        else if(disym1 == TOMBSTONE) {
            continue;
        }

        debug("disym1 is not null or tombstone");

        disym2 = (*disym1).next;
        if(disym2 == NULL) {
            return -1;
        }

        debug("disym2 is not null");

        if((disym1 == sym1) && (disym2 == sym2)) {
            *(digram_table + i) = TOMBSTONE;
            return 0;
        }
    }

    return -1;
}



/**
 * Attempt to insert a digram into the hash table.
 *
 * @param digram  The digram to be inserted.
 * @return  0 in case the digram did not previously exist in the table and
 * insertion was successful, 1 if a matching digram already existed in the
 * table and no change was made, and -1 in case of an error, such as the hash
 * table being full or the given digram not being well-formed.
 */
int digram_put(SYMBOL *digram) {
    SYMBOL *disym1 = NULL;
    SYMBOL *disym2 = NULL;

    SYMBOL *sym1 = digram;
    if(sym1 == NULL) {
        return -1;
    }

    SYMBOL *sym2 = (*digram).next;
    if(sym2 == NULL) {
        return -1;
    }

    debug("digram_put symbol is well formed");

    int sym1val = (*sym1).value;
    int sym2val = (*sym2).value;
    int index = DIGRAM_HASH(sym1val, sym2val);

    debug("sym1val: %d, sym2val: %d, index: %d", sym1val, sym2val, index);


    // Loop part 1
    for(int i = index; i < MAX_DIGRAMS; i++) {
        disym1 = *(digram_table + i);

        if((disym1 == NULL) || (disym1 == TOMBSTONE)) {
            // Did not exist, successful insert into digram
            *(digram_table + i) = digram;
            return 0;
        }

        disym2 = (*disym1).next;
        if(((*disym1).value == sym1val) && ((*disym2).value == sym2val)) {
            // Same digram values, already exist
            return 1;
        }

    }

    // Loop part 2
    for(int i = 0; i < index; i++) {
        disym1 = *(digram_table + i);

        if((disym1 == NULL) || (disym1 == TOMBSTONE)) {
            // Did not exist, successful insert into digram
            *(digram_table + i) = digram;
            return 0;
        }

        disym2 = (*disym1).next;
        if(((*disym1).value == sym1val) && ((*disym2).value == sym2val)) {
            // Same digram values, already exist
            return 1;
        }
    }

    return -1;
}
