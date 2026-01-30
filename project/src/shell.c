#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <unistd.h>
#include "shell.h"
#include "interpreter.h"
#include "shellmemory.h"

int parseInput(char ui[]);

// Start of everything
int main(int argc, char *argv[]) {
    printf("Shell version 1.5 created Dec 2025\n");

    char prompt = '$';  				// Shell prompt
    char userInput[MAX_USER_INPUT];		// user's input stored here
    int errorCode = 0;					// zero means no error, default
    int interactive = isatty(STDIN_FILENO);	// false when stdin is a file (batch mode)

    //init user input
    for (int i = 0; i < MAX_USER_INPUT; i++) {
        userInput[i] = '\0';
    }
    
    //init shell memory
    mem_init();
    while(1) {
        if (interactive) {
            printf("%c ", prompt);
        }
        if (fgets(userInput, MAX_USER_INPUT - 1, stdin) == NULL) {
            /* EOF in batch mode: exit after running all instructions */
            break;
        }
        errorCode = parseInput(userInput);
        if (errorCode == -1) exit(99);	// ignore all other errors
        memset(userInput, 0, sizeof(userInput));
    }

    return 0;
}

int wordEnding(char c) {
    return c == '\0' || c == '\n' || c == ' ';
}

// Parse a single command segment (without semicolons) into words and execute it
static int parseCommand(char *cmd) {
    char tmp[200], *words[100];
    int ix = 0, w = 0;
    int wordlen;
    int errorCode;
    
    // Skip leading spaces
    for (ix = 0; cmd[ix] == ' ' && ix < 1000; ix++);
    
    // Parse words
    while (cmd[ix] != '\n' && cmd[ix] != '\0' && cmd[ix] != ';' && ix < 1000) {
        // extract a word
        for (wordlen = 0; !wordEnding(cmd[ix]) && cmd[ix] != ';' && ix < 1000; ix++, wordlen++) {
            tmp[wordlen] = cmd[ix];
        }
        tmp[wordlen] = '\0';
        if (wordlen > 0) {
            words[w] = strdup(tmp);
            w++;
        }
        if (cmd[ix] == '\0' || cmd[ix] == ';') break;
        ix++;
    }
    
    if (w == 0) {
        return 0; // Empty command, skip
    }
    
    errorCode = interpreter(words, w);
    
    // Free allocated words
    for (int i = 0; i < w; i++) {
        free(words[i]);
    }
    
    return errorCode;
}

int parseInput(char inp[]) {
    char segments[10][1000];
    int seg_count = 0;
    int seg_start = 0;
    int errorCode = 0;
    
    // Split input by semicolons
    for (int i = 0; inp[i] != '\0' && i < 1000 && seg_count < 10; i++) {
        if (inp[i] == ';') {
            // Copy segment
            int len = i - seg_start;
            if (len >= 0 && len < 1000) {
                strncpy(segments[seg_count], inp + seg_start, len);
                segments[seg_count][len] = '\0';
                seg_count++;
            }
            seg_start = i + 1;
        }
    }
    
    // Add the last segment (after last semicolon or if no semicolons)
    if (seg_start < 1000) {
        int len = 0;
        for (int i = seg_start; inp[i] != '\0' && inp[i] != '\n' && i < 1000; i++) {
            len++;
        }
        if (len >= 0 && len < 1000) {
            strncpy(segments[seg_count], inp + seg_start, len);
            segments[seg_count][len] = '\0';
            seg_count++;
        }
    }
    
    // Execute each command segment
    for (int i = 0; i < seg_count; i++) {
        errorCode = parseCommand(segments[i]);
        if (errorCode != 0) {
            break; // Stop on error
        }
    }
    
    return errorCode;
}
