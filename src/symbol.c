#include <stdlib.h>
#include "const.h"
#include "sequitur.h"

/*
 * Symbol management.
 *
 * The functions here manage a statically allocated array of SYMBOL structures,
 * together with a stack of "recycled" symbols.
 */

/*
 * Initialization of this global variable that could not be performed in the header file.
 */
int next_nonterminal_value = FIRST_NONTERMINAL;

/**
 * Initialize the symbols module.
 * Frees all symbols, setting num_symbols to 0, and resets next_nonterminal_value
 * to FIRST_NONTERMINAL;
 */
void init_symbols(void) {
    // TODO - Free all symbols
    // Setting the pointer back to zero will in a way free all symbols
    num_symbols = 0;
    next_nonterminal_value = FIRST_NONTERMINAL;
}

/**
 * Get a new symbol.
 *
 * @param value  The value to be used for the symbol.  Whether the symbol is a terminal
 * symbol or a non-terminal symbol is determined by its value: terminal symbols have
 * "small" values (i.e. < FIRST_NONTERMINAL), and nonterminal symbols have "large" values
 * (i.e. >= FIRST_NONTERMINAL).
 * @param rule  For a terminal symbol, this parameter should be NULL.  For a nonterminal
 * symbol, this parameter can be used to specify a rule having that nonterminal at its head.
 * In that case, the reference count of the rule is increased by one and a pointer to the rule
 * is stored in the symbol.  This parameter can also be NULL for a nonterminal symbol if the
 * associated rule is not currently known and will be assigned later.
 * @return  A pointer to the new symbol, whose value and rule fields have been initialized
 * according to the parameters passed, and with other fields zeroed.  If the symbol storage
 * is exhausted and a new symbol cannot be created, then a message is printed to stderr and
 * abort() is called.
 */
SYMBOL *new_symbol(int value, SYMBOL *rule) {
    // TODO: Maybe also do something with next_nonterminal_value

    // Include helpers
    SYMBOL *get_recycled_symbol();
    void set_new_symbol_values(SYMBOL *sym, SYMBOL *rule, int value);


    // Return null if terminal symbol and terminal is not null
    if(value < FIRST_NONTERMINAL && rule != NULL) {
        return NULL; // Take care of error if this is null
    }

    // Check for recycled symbols
    SYMBOL *sym = get_recycled_symbol();
    if(sym) {
	set_new_symbol_values(sym, rule, value);
	if(rule != NULL) {
	    ref_rule(rule);
	}

        // Returns the contents in sym, which is the pointer returned from get_recycled_symbol()
        return sym;
    }

    if (num_symbols >= MAX_SYMBOLS) {
        abort();
    }

    // Get the space from symbol_storage
    sym = (symbol_storage + num_symbols);
    set_new_symbol_values(sym, rule, value);
    if(rule != NULL) {
        ref_rule(rule);
    }

    // Increment num_symbols
    num_symbols = num_symbols + 1;
    return sym;
}

/**
 * @brief Gets a recycled symbol.
 *
 * @return A recycled symbol
 */
SYMBOL *get_recycled_symbol() {
    for(SYMBOL *sym = symbol_storage + num_symbols - 1; sym >= symbol_storage; sym--) {
	if((*sym).refcnt == -1) {
	    return sym;
	}
    }
    return NULL;
}

/**
 * @brief Set new symbol values as specified in the new_rule() description
 * Initialize values and rules field, zero other fields.
 */
void set_new_symbol_values(SYMBOL *sym, SYMBOL *rule, int value) {
    // Initialize value and rule
    (*sym).value = value;
    (*sym).rule = rule;

    // Zero other fields.
    (*sym).refcnt = 0;
    (*sym).next = NULL;
    (*sym).prev = NULL;
    (*sym).nextr = NULL;
    (*sym).prevr = NULL;
}

/**
 * Recycle a symbol that is no longer being used.
 *
 * @param s  The symbol to be recycled.  The caller must not use this symbol any more
 * once it has been recycled.
 */
void recycle_symbol(SYMBOL *s) {
    (*s).refcnt = -1;
}
