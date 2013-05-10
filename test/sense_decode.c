#include "sense_dump.h"
#include <stdio.h>
#include <stdbool.h>

int main(int argc, char **argv)
{
        unsigned char sense[256];
        int sense_len = 0;

        printf("Reading sense from args to buffer...\n");
        int i;
        bool is_top_nibble = true;
        for (i = 1; i < argc; i++) {
                char *ch;
                
                for (ch = argv[i]; *ch != 0; ch++) {
                        unsigned char val;
                        if (*ch >= '0' && *ch <= '9')
                                val = (*ch) - '0';
                        else if (*ch >= 'a' && *ch <= 'f')
                                val = (*ch) - 'a' + 0xa;
                        else if (*ch >= 'A' && *ch <= 'F')
                                val = (*ch) - 'A' + 0xA;
                        else if (*ch == ' ' || *ch == '+' || *ch == '\t') {
                                continue;
                        } else {
                                fprintf(stderr, "\n\nInvalid character '%c' (%d) in sequence\n", *ch, *ch);
                                return 1;
                        }

                        if (is_top_nibble) {
                                sense[sense_len] = val<<4;
                                is_top_nibble = false;
                        } else {
                                sense[sense_len] |= val;
                                is_top_nibble = true;
                                sense_len++;
                        }
                }
        }

        if (!is_top_nibble) {
                fprintf(stderr, "Missing a nibble!\n");
                return 2;
        }

        printf("Decoding sense...\n");
        sense_dump(sense, sense_len);
        return 0;
}
