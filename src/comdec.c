#include "const.h"
#include "sequitur.h"
#include "debug.h"

// Function prototoypes
int isMarker(int byte);
int isValidMarker(int byte);
int isMarkerValue(int markerRep, int bytes);
int isSOT(int b);
int isEOT(int b);
int isSOB(int b);
int isEOB(int b);
int isRD(int b);
int isNonterminalStart(int byte);
int getNextNonterminalByte(FILE *in, FILE *out);
int makeNonterminalNext(int span, int prevbyte, FILE *in, FILE *out);
int isTerminalSingle(int byte);
int isTerminalDouble(int byte);
int getNextTerminalByte(FILE *in, FILE *out);
int makeTerminalNext(int prevbyte, FILE *in, FILE *out);
int getUTF1(int num);
int getUTF2(int num);
int getUTF3(int num);
int getUTF4(int num);
int readRuleData(FILE *in, FILE *out);
int readBlockData(FILE *in, FILE *out);
int mapBodyRules(SYMBOL *head, FILE *in, FILE *out);

int determineUTFByteSize(int value);
int convertToUTF(int value, int bytesize, FILE *out);

SYMBOL *compressInitBlockFunctions();
int compressBlockRules(int byte, SYMBOL *head, FILE *in);
int compressWriteRuleBody(SYMBOL *rule, FILE *out);

int writeouts = 0;
int compressedbytes = 0;

/*
 * You may modify this file and/or move the functions contained here
 * to other source files (except for main.c) as you wish.
 */

/**
 * Main compression function.
 * Reads a sequence of bytes from a specified input stream, segments the
 * input data into blocks of a specified maximum number of bytes,
 * uses the Sequitur algorithm to compress each block of input to a list
 * of rules, and outputs the resulting compressed data transmission to a
 * specified output stream in the format detailed in the header files and
 * assignment handout.  The output stream is flushed once the transmission
 * is complete.
 *
 * The maximum number of bytes of uncompressed data represented by each
 * block of the compressed transmission is limited to the specified value
 * "bsize".  Each compressed block except for the last one represents exactly
 * "bsize" bytes of uncompressed data and the last compressed block represents
 * at most "bsize" bytes.
 *
 * @param in  The stream from which input is to be read.
 * @param out  The stream to which the block is to be written.
 * @param bsize  The maximum number of bytes read per block.
 * @return  The number of bytes written, in case of success,
 * otherwise EOF.
 */
int compress(FILE *in, FILE *out, int bsize) {
    int compret = 0;
    compressedbytes = 0; // Number of bytes written out
    char byte = fgetc(in);

    int puttedc = fputc(0x81, out); // SOT
    compressedbytes++;
    if(puttedc == EOF) {
        return EOF;
    }

    while(byte != EOF) {
        debug("Reading byte: %c (0x%x)", byte, byte);
        SYMBOL *head = compressInitBlockFunctions();

        // Loop for reading text file of blocksize
        int bsizeCounter = 0;
        int kilobsize = bsize * 1024;
        while(bsizeCounter < kilobsize) {
            int cbrRet = compressBlockRules(byte, head, in);
            if(!cbrRet) { // Break out of the loop when the byte is EOF
                break;
            }
            bsizeCounter++;
            byte = fgetc(in);
            if(byte == EOF) {
                break;
            }
            debug("Got byte: %d", byte);
            debug("bsizeCounter: %d", bsizeCounter);
        }

        puttedc = fputc(0x83, out); // SOB
        compressedbytes++;
        if(puttedc == EOF) {
            return EOF;
        }


        SYMBOL *ruleptr = head;
        do { // Loop to write output file with existing rules
            compret = compressWriteRuleBody(ruleptr, out);
            if(!compret) {
                return EOF;
            }
            ruleptr = ruleptr->nextr;
            if(ruleptr != head) { // RD
                puttedc = fputc(0x85, out);
                compressedbytes++;
                if(puttedc == EOF) {
                    return EOF;
                }
            }
        } while(ruleptr != head);

        puttedc = fputc(0x84, out); // EOB
        compressedbytes++;
        if(puttedc == EOF) {
            return EOF;
        }
    }
    puttedc = fputc(0x82, out); // EOT
    compressedbytes++;
    if(puttedc == EOF) {
        return EOF;
    }

    fflush(out);
    return compressedbytes;
}


/**
 * Writes out the rule body to FILE out
 *
 * @return 0 if fail write, 1 if success
 */
int compressWriteRuleBody(SYMBOL *rule, FILE *out) {
    debug("compressWriteRuleBody value of rule: %d", rule->value);
    SYMBOL *symptr = rule;
    int value = rule->value;
    int bytesize = determineUTFByteSize(value);
    int convtoUTF = convertToUTF(value, bytesize, out);
    if(!convtoUTF) {
        return 0;
    }
    symptr = symptr->next;
    while(symptr != rule) {
        value = symptr->value;
        bytesize = determineUTFByteSize(value);
        convtoUTF = convertToUTF(value, bytesize, out);
        if(!convtoUTF) {
            return 0;
        }
        symptr = symptr->next;
    }
    return 1;
}


/**
 * Processes the inner body of the while loop for each block
 *
 * @return 1 If successfully completed the inner functions
 * @return 0 If the
 */
int compressBlockRules(int byte, SYMBOL *head, FILE *in) {
    if(byte == EOF) {
        return 0;
    }
    SYMBOL *sym = new_symbol(byte, NULL);
    insert_after(head->prev, sym);
    check_digram(sym->prev);
    return 1;
}

/**
 * Helper to initialize functions and variables for the start of each block
 * in compress
 * Side note: not sure if recycling gets reset
 */
SYMBOL *compressInitBlockFunctions() {
    init_symbols();
    init_rules();
    init_digram_hash();
    SYMBOL *head = new_rule(next_nonterminal_value);
    next_nonterminal_value++;
    add_rule(head);
    return head;
}

/**
 * Given a value, convert the value to bytes in UTF and fputc it
 *
 * @return 0 fail file write, 1 for success file write
 */
int convertToUTF(int value, int bytesize, FILE *out) {
    debug("convertToUTF");
    int puttedc;
    if(bytesize == 1) {
        puttedc = fputc(value, out);
        if(puttedc == EOF) {
            return 0;
        }
        compressedbytes += 1;
        return 1;
    }
    else if(bytesize == 2) {
        int byte1 = 0xC0 | ((value >> 6) & 0x1F);
        int byte2 = 0x80 | (value & 0x3F);
        
        puttedc = fputc(byte1, out);
        if(puttedc == EOF) {
            return 0;
        }
        puttedc = fputc(byte2, out);
        if(puttedc == EOF) {
            return 0;
        }
        compressedbytes += 2;
        return 1;
    }
    else if(bytesize == 3) {
        
        int byte1 = 0xE0 | ((value >> 12) & 0x0F);
        int byte2 = 0x80 | ((value >> 6) & 0x3F);
        int byte3 = 0x80 | (value & 0x3F);
        
        puttedc = fputc(byte1, out);
        if(puttedc == EOF) {
            return 0;
        }
        puttedc = fputc(byte2, out);
        if(puttedc == EOF) {
            return 0;
        }
        puttedc = fputc(byte3, out);
        if(puttedc == EOF) {
            return 0;
        }
        compressedbytes += 3;
        return 1;
    }
    else if(bytesize == 4) {
       
        int byte1 = 0xF0 | ((value >> 18) & 0x07);
        int byte2 = 0x80 | ((value >> 12) & 0x3F);
        int byte3 = 0x80 | ((value >> 6) & 0x3F);
        int byte4 = 0x80 | (value & 0x3F);
        
        puttedc = fputc(byte1, out);
        if(puttedc == EOF) {
            return 0;
        }
        puttedc = fputc(byte2, out);
        if(puttedc == EOF) {
            return 0;
        }
        puttedc = fputc(byte3, out);
        if(puttedc == EOF) {
            return 0;
        }
        puttedc = fputc(byte4, out);
        if(puttedc == EOF) {
            return 0;
        }
        compressedbytes += 4;
        return 1;
    }
    else {
        debug("Invalid convert.");
        return 0;
    }
}
/**
 * Given a value, determine the length of the UTF value in bytes
 */
int determineUTFByteSize(int value) {
    if(value <= 0x7f) {
        return 1;
    }
    else if(value <= 0x7ff) {
        return 2;
    }
    else if(value < 0xffff) {
        return 3;
    }
    else if(value < 0x10ffff) {
        return 4;
    }
    debug("Invalid return.");
    return 0;
}

/**
 * Main decompression function.
 * Reads a compressed data transmission from an input stream, expands it,
 * and and writes the resulting decompressed data to an output stream.
 * The output stream is flushed once writing is complete.
 *
 * @param in  The stream from which the compressed block is to be read.
 * @param out  The stream to which the uncompressed data is to be written.
 * @return  The number of bytes written, in case of success, otherwise EOF.
 */
int decompress(FILE *in, FILE *out) {
    writeouts = 0;
    init_symbols();
    init_rules();
    int byte;
    int rbdflag;
    int ret;

    // Start of transmission
    byte = fgetc(in);
    if(!isSOT(byte)) {
        return EOF;
    }

    // Parse blocks, check using isSOB
    byte = fgetc(in);
    while(isSOB(byte)) {
        rbdflag = readBlockData(in, out);
        if(!rbdflag) {
            return EOF;
        }

        ret = mapBodyRules(main_rule, in, out);
        if(!ret) {
            return EOF;
        }

        init_symbols();
        init_rules();
        byte = fgetc(in);
    }

    // End of transmission
    if(!isEOT(byte)) {
        return EOF;
    }
    byte = fgetc(in);
    if(byte != EOF) {
        return EOF;
    }

    fflush(out);

    return writeouts;
}


/**
 * Maps the body symbol's rule variable the rules in the rule_map.
 * After this, expansion will happen
 *
 * @precondition The link list of all rules, along with their body. The
 * rule_map is also made and should contain the rules of this block.
 * @return 0 on fail, 1 on success
 */
int mapBodyRules(SYMBOL *head, FILE *in, FILE *out) {
    int max = (1 << 21);
    int ret = 0;
    SYMBOL *ptr = (*head).next;
    while(ptr != head) {
        if((*ptr).value < FIRST_NONTERMINAL) {
            writeouts++;
            int puttedc = 0;
            puttedc = fputc((*ptr).value, out);
            if(puttedc == EOF) {
                return 0;
            }
            compressedbytes++;
            ptr = (*ptr).next;
        }
        else if((*ptr).value > max) {
            return 0;
        }
        else if(*(rule_map + (*ptr).value) == NULL) {
            return 0;
        }
        else {
            SYMBOL *nonterm = NULL;
            nonterm = (*ptr).rule;
            nonterm = *(rule_map + ((*ptr).value));
            ret = mapBodyRules(nonterm, in, out);
            if(!ret) {
                return 0;
            }
            ptr = (*ptr).next;
        }
    }
    return 1;
}

/**
 * Reads the block of data and parses it
 *
 * @return 1 on successful parse
 * 0 on unsuccessful parse
 */
int readBlockData(FILE *in, FILE *out) {
    debug("reached readBlockData");
    int rrdflag = 0x85;
    while(isRD(rrdflag)) {
        rrdflag = readRuleData(in, out);
    }

    if(isEOB(rrdflag)) {
        return 1;
    }
    return 0;
}

/**
 * Reads and checks the single rule in the block after the SOB or RD
 *
 * @return EOB or RD if sucussful, 0 if unsuccessful
 */
int readRuleData(FILE *in, FILE *out) {
    debug("reached readRuleData");
    void add_body(SYMBOL *bodysym, SYMBOL *rule);
    SYMBOL *head;
    int byte;
    int symval = 0;
    int symcount = 0; // Return 0 if this is less than 3
    int nonterminalspan = 0;
    int terminalspan = 0;

    // Valid rule head
    byte = fgetc(in);
    nonterminalspan = isNonterminalStart(byte);
    symval = makeNonterminalNext(nonterminalspan, byte, in, out);
    if(!(nonterminalspan && symval)) {
        return 0;
    }
    head = new_rule(symval); // make the symbol of the rule head
    add_rule(head);
    symcount++;

    // Make rule body
    // TODO: Make debug print statements of conditional body and value calculated
    while(1) {
        byte = fgetc(in);
        if(isMarker(byte)) {
            debug("Reached isMarker() in readRuleData()\n");
            if(symcount < 3 || !isValidMarker(byte)) {
                return 0;
            }
            else if(isEOB(byte) || isRD(byte)) {
                break;
            }
            else {
                return 0; // Guaranteed return
            }
        }
        else if((nonterminalspan = isNonterminalStart(byte))) {
            symval = makeNonterminalNext(nonterminalspan, byte, in, out);
            if(!symval) {
                return 0;
            }
            SYMBOL *body = new_symbol(symval, NULL);
            add_body(body, head);
            symcount++;
            // debug("Created a nonterminal value: %d", symval);
        }
        else if((terminalspan = isTerminalDouble(byte))) {
            symval = makeTerminalNext(byte, in, out);
            if(!symval) {
                return 0;
            }
            SYMBOL *body = new_symbol(symval, NULL);
            add_body(body, head);
            symcount++;
            // debug("Created a terminal 2 byte value: %d", symval);
        }
        else if(isTerminalSingle(byte)) {
            SYMBOL *body = new_symbol(byte, NULL);
            add_body(body, head);
            symcount++;
            // debug("Created a terminal 1 byte value: %d", byte);
        }
        else {
            // debug("Else case in readRuleData()");
            return 0;
        }
    }

    // Add to rule to rule_map
    int offset = (*head).value;
    *(rule_map + offset) = head;

    if(isEOB(byte) || isRD(byte)) {
        return byte;
    }
    return 0;
}

/**
 * Gets the value of a 1 byte utf for the sake of being complete
 */
int getUTF1(int num) {
    int mask0 = 0b01111111;
    int byte0 = mask0 & num;
    int word = byte0;
    return word;
}

/**
 * Gets the value of a 2 byte utf
 */

 int getUTF2(int num) {
    // Extract the 5 bits from first byte (110xxxxx)
    int firstByte = (num >> 8) & 0xFF;
    // Extract the 6 bits from second byte (10xxxxxx)
    int secondByte = num & 0xFF;
    
    // Combine the bits: 5 from first byte, 6 from second byte
    int value = ((firstByte & 0x1F) << 6) | (secondByte & 0x3F);
    return value;
}

/**
 * Gets the value of a 3 byte utf
 */
int getUTF3(int num) {
    // Extract bytes
    int firstByte = (num >> 16) & 0xFF;
    int secondByte = (num >> 8) & 0xFF;
    int thirdByte = num & 0xFF;
    
    // Combine the bits: 4 from first byte, 6 from second byte, 6 from third byte
    int value = ((firstByte & 0x0F) << 12) | ((secondByte & 0x3F) << 6) | (thirdByte & 0x3F);
    return value;
}


/**
 * Gets the value of a 4 byte utf
 */
int getUTF4(int num) {
    // Extract bytes
    int firstByte = (num >> 24) & 0xFF;
    int secondByte = (num >> 16) & 0xFF;
    int thirdByte = (num >> 8) & 0xFF;
    int fourthByte = num & 0xFF;
    
    // Combine the bits: 3 from first byte, 6 from second byte, 6 from third byte, 6 from fourth byte
    int value = ((firstByte & 0x07) << 18) | ((secondByte & 0x3F) << 12) | 
                ((thirdByte & 0x3F) << 6) | (fourthByte & 0x3F);
    return value;
}

/**
 * Gets the value after utf from utf num and number of utf bytes
 *
 * Return 0 on error
 */
int getUTFs(int num, int span) {
    if(span == 1) {
        return getUTF1(num);
    }
    else if(span == 2) {
        return getUTF2(num);
    }
    else if(span == 3) {
        return getUTF3(num);
    }
    else if(span == 4) {
        return getUTF4(num);
    }
    return 0;
}

/**
 * Determines if this is a terminal with 2 bytes
 *
 * @return 1 if this is a 2 byte terminal, 0 if not.
 */
int isTerminalDouble(int byte) {
    int maskc0 = 0b11000000;
    int maskc1 = 0b11000001;
    int maskc2 = 0b11000010;
    int maskc3 = 0b11000011;
    if(byte == maskc0 || byte == maskc1 || byte == maskc2 || byte == maskc3) {
        return 1; // Terminal masks
    }
    return 0;
}

/**
 * Makes and interpret the rest of the terminal value based on
 * the previous byte.
 *
 * @return the terminal symbol value extracted from utf.
 * 0 if failed representation of terminal.
 */
int makeTerminalNext(int prevbyte, FILE *in, FILE *out) {
    int byte = prevbyte << 8;
    int nextbyte = getNextTerminalByte(in, out);
    if(!nextbyte) {
        return 0;
    }
    byte = byte | nextbyte; // 110x xxxx 10xx xxxx
    int value = getUTF2(byte);
    return value;
}

/**
 * Return the next int value of the terminal byte.
 *
 * @return the value of the next terminal byte
 * 0 if invlaid byte
 */
int getNextTerminalByte(FILE *in, FILE *out) {
    int byte = fgetc(in);
    int shiftedbyte = byte >> 6;
    int mask = 0b10;
    if(mask == shiftedbyte) {
        return byte;
    }
    return 0;
}

/**
 * Determines if this is a terminal with 1 byte.
 * Make sure to not use this as a span
 *
 * @return 1 if this is a 1 byte terminal, 0 if not.
 */
int isTerminalSingle(int byte) {
    int mask = 0x0;
    int shiftedbyte = byte >> 7;
    if(shiftedbyte == mask) {
        return 1;
    }
    return 0;
}

/**
 * Determines if the byte is the first part of a nonterminal
 *
 * @param byte The byte read from fgetc();
 * @return The nonterminal span of the number,
 * 0 if not a valid nonterminal start byte, 1 or more for success
 */
int isNonterminalStart(int byte) {
    int shiftedbyte;
    int maskc0 = 0b11000000;
    int maskc1 = 0b11000001;
    int maskc2 = 0b11000010;
    int maskc3 = 0b11000011;
    if(byte == maskc0 || byte == maskc1 || byte == maskc2 || byte == maskc3) {
        return 0; // Terminal masks
    }

    shiftedbyte = byte >> 5;
    int maskspan1 = 0b110;
    if(shiftedbyte == maskspan1) {
        return 1;
    }

    shiftedbyte = byte >> 4;
    int maskspan2 = 0b1110;
    if(shiftedbyte == maskspan2) {
        return 2;
    }

    shiftedbyte = byte >> 3;
    int maskspan3 = 0b11110;
    if(shiftedbyte == maskspan3) {
        return 3;
    }
    return 0;
}

/**
 * Makes and interpret the rest of the nonterminal value based on
 * the nonterminalspan and the previous byte.
 *
 * @return the value of the full nonterminal.
 * 0 if failed representation of nonterminal.
 */
int makeNonterminalNext(int span, int prevbyte, FILE *in, FILE *out) {
    int utfspan = span + 1;
    int shiftamount = 8 * span;
    int byte = prevbyte << shiftamount;

    while(span > 0) {
        span--;
        int nextbyte = getNextNonterminalByte(in, out);
        if(!nextbyte) {
            return 0;
        }
        shiftamount = 8 * span;
        nextbyte = nextbyte << shiftamount;
        byte = byte | nextbyte;
    }
    int value = getUTFs(byte, utfspan);
    return value;
}

/**
 * Return the next int value of the nonterminal byte
 *
 * @return The value of the next nonterminal byte
 * 0 if invalid byte
 */
int getNextNonterminalByte(FILE *in, FILE *out) {
    int byte = fgetc(in);
    int shiftedbyte = byte >> 6;
    int mask = 0b10;
    if(mask == shiftedbyte) {
        return byte;
    }
    return 0;
}


/**
 * Determines if the byte is any marker, does not have to be valid.
 * A marker begins with the continuation byte 0b10xxxxxx
 *
 * @return 1 if byte is a marker
 * 0 if the byte is not a marker
 */
int isMarker(int byte) {
    int mask = 0b10;
    int shiftedbyte = byte >> 6;
    if(mask == shiftedbyte) {
        return 1;
    }
    return 0;
}

/**
 * Determines if the byte is any valid marker
 *
 * @return 1 if byte is a valid marker
 * 0 if the byte is not a valid marker
 */
int isValidMarker(int byte) {
    if(isSOT(byte) || isEOT(byte) || isSOB(byte) || isEOB(byte) || isRD(byte)) {
        return 1;
    }
    return 0;
}

/**
 * Determines if this byte is the specific marker value
 *
 * @param markerRep The hex representation of the marker stored in an int
 * @bytes The bytes of the integer returned from fgetc(). 4 bytes in an int,
 * but only the least significant byte is being used for bitwise comparison.
 * @return 1 if this is the specified marker value, 0 if this is not
 * the specified marker of markerRep
 */
int isMarkerValue(int markerRep, int bytes) {
    if(markerRep == bytes) {
        return 1;
    }
    return 0;
}

int isSOT(int b) {
    return isMarkerValue(0x81, b);
}

int isEOT(int b) {
    return isMarkerValue(0x82, b);
}

int isSOB(int b) {
    return isMarkerValue(0x83, b);
}

int isEOB(int b) {
    return isMarkerValue(0x84, b);
}

int isRD(int b) {
    return isMarkerValue(0x85, b);
}


/**
 * @brief Validates command line arguments passed to the program.
 * @details This function will validate all the arguments passed to the
 * program, returning 0 if validation succeeds and -1 if validation fails.
 * Upon successful return, the selected program options will be set in the
 * global variable "global_options", where they will be accessible
 * elsewhere in the program.
 *
 * @param argc The number of arguments passed to the program from the CLI.
 * @param argv The argument strings passed to the program from the CLI.
 * @return 0 if validation succeeds and -1 if validation fails.
 * Refer to the homework document for the effects of this function on
 * global variables.
 * @modifies global variable "global_options" to contain a bitmap representing
 * the selected options.
 */
int validargs(int argc, char **argv) {
    // Include helpers
    int stringCompare(char *string1, char *string2);
    int parseBlocksize(char *string);
    void modifyGlobalOptions(int blocksize, char *flag);

    // Variables
    char *flagH = "-h";
    char *flagC = "-c";
    char *flagD = "-d";
    char *flagB = "-b";
    int defaultblocksize = 1024;

    // Return PASS and modify global_options if -h is the first flag.
    // There is at least 1 flag.
    if (argc > 1 && stringCompare(flagH, *(argv + 1))) {
        modifyGlobalOptions(defaultblocksize, flagH);
        return 0;
    }

    // Return PASS and modify global options if -d is the only flag.
    // Or if -c is the only flag.
    // There is only 1 flag.
    if(argc == 2) {
        if(stringCompare(flagD, *(argv + 1))) {
            modifyGlobalOptions(defaultblocksize, flagD);
            return 0;
        }
        else if(stringCompare(flagC, *(argv + 1))) {
            modifyGlobalOptions(defaultblocksize, flagC);
            return 0;
        }
    }

    // Return PASS and modify global options if -c is the first flag,
    // -b is the second flag, and the third input is a number in [1, 1024].
    // There are 3 additional arguments to the initial function.
    if(argc == 4 && stringCompare(flagC, *(argv + 1)) && stringCompare(flagB, *(argv + 2))) {
        int blocksize = parseBlocksize(*(argv + 3));
        if(blocksize != -1) {
            modifyGlobalOptions(blocksize, flagC);
            return 0;
        }
    }

    // Else return FAIL;
    return -1;
}

/**
 * @brief Modifies global_options.
 * @details Modifies global_options based on the given blocksize and flag.
 *
 * @param blocksize The blocksize of global_options
 * @param flag The flag
 */
void modifyGlobalOptions(int blocksize, char *flag) {
    // Include helper functions
    int stringCompare(char *string1, char *string2);

    int temp = 0;
    int flagbit = 0;
    if(stringCompare("-h", flag)) {
        flagbit = 0x1; // 0b0001
        temp = flagbit;
    }
    else if(stringCompare("-c", flag)) {
        flagbit = 0x2; // 0b0010
        temp = blocksize;
        temp = temp << 16;
        temp = temp | flagbit;
    }
    else if(stringCompare("-d", flag)) {
        flagbit = 0x4; // 0b0100
        temp = flagbit;
    }
    global_options = temp;
}

/**
 * @brief Parses a given blocksize string and returns an integer.
 * @details This function will parse a given blocksize string and determine
 * if it is valid or not. If the string is valid, it returns the integer size,
 * otherwise, it returns -1.
 *
 * @param string Pointer to the string
 * @return The blocksize if successful, -1 if it is not a valid
 * blocksize.
 */
int parseBlocksize(char *string) {
    // Include helpers
    int stringLength(char *string);

    // Constants
    int fail = -1;
    int number = 0;
    int loopCounter = 1;
    char zero = '0';
    char nine = '9';

    // Get the last index of the string array.
    int index = stringLength(string);
    index--;

    // Loop to construct integer value.
    while(index >= 0) {
        char c = *(string + index);
        if(c < zero || c > nine) {
            // Return FAIL if not a number character.
            return fail;
        }
        else if(loopCounter > 1000) {
            // Return FAIL if leading characters are not zeros
            if(c != zero) {
                return fail;
            }
        }
        else if(loopCounter <= 1000) {
            // Change into integer and add to number.
            c = c - 48; // Ex. '0' = 48; 48 - 48 = 0; The 'real' number.
            number = number + (c * loopCounter); // Ex. 0 = 0 + (0 * 1);
            loopCounter = loopCounter * 10; // Ex. loopCounter = 10;
        }
        index--;
    }

    // Range of current number is [0-9999]
    if(number < 1 || number > 1024) {
        // Return FAIL if number is not in correct range.
        return fail;
    }
    return number;
}

/**
 * @brief Returns the length of the given string.
 *
 * @param string Pointer to the string.
 * @return The length of the string.
 */
int stringLength(char *string) {
    int nullTerm = '\0';
    int length = 0;

    while(*string != nullTerm) {
        length++;
        string++;
    }
    return length;
}

/**
 * @brief Compares 2 strings and checks if they are equal.
 * @details This function will compare 2 strings passed in as arguments,
 * returning 1 if strings are equal and 0 if strings are not equal.
 *
 * @param string1 Pointer to the first string.
 * @param string2 Pointer to the second string.
 * @return 1 if the strings are equal and 0 if the strings are not equal.
 */
int stringCompare(char *string1, char *string2) {
    // Variables
    char nullTerm = '\0';
    int pass = 1;
    int fail = 0;

    while(1) {
        if(*string1 == nullTerm && *string2 == nullTerm) {
            // Return at the end of the string.
            // Matched args.
            return pass;
        }
        else if(*string1 == nullTerm || *string2 == nullTerm) {
            // Return when one string is longer than the other.
            // Unmatched args.
            return fail;
        }
        else if(*string1 != *string2) {
            // Return when characters at different.
            // Unmatched args.
            return fail;
        }
        else {
            // Increment pointers by size of char
            string1++;
            string2++;
        }
    }

    // Function never reaches here.
    return pass;
}