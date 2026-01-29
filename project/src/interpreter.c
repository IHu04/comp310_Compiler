#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include "shellmemory.h"
#include "shell.h"

int MAX_ARGS_SIZE = 3;

int badcommand() {
    printf("Unknown Command\n");
    return 1;
}

// For source command only
int badcommandFileDoesNotExist() {
    printf("Bad command: File not found\n");
    return 3;
}

int help();
int quit();
int set(char *var, char *value);
int print(char *var);
int echo(char *token);
int my_ls(void);
int my_mkdir(char *dirname);
int source(char *script);
int badcommandFileDoesNotExist();

// Interpret commands and their arguments
int interpreter(char *command_args[], int args_size) {
    int i;

    if (args_size < 1 || args_size > MAX_ARGS_SIZE) {
        return badcommand();
    }

    for (i = 0; i < args_size; i++) {   // terminate args at newlines
        command_args[i][strcspn(command_args[i], "\r\n")] = 0;
    }

    if (strcmp(command_args[0], "help") == 0) {
        //help
        if (args_size != 1)
            return badcommand();
        return help();

    } else if (strcmp(command_args[0], "quit") == 0) {
        //quit
        if (args_size != 1)
            return badcommand();
        return quit();

    } else if (strcmp(command_args[0], "set") == 0) {
        //set
        if (args_size != 3)
            return badcommand();
        return set(command_args[1], command_args[2]);

    } else if (strcmp(command_args[0], "print") == 0) {
        if (args_size != 2)
            return badcommand();
        return print(command_args[1]);

    } else if (strcmp(command_args[0], "echo") == 0) {
        // echo takes exactly one token (echo + token)
        if (args_size != 2)
            return badcommand();
        return echo(command_args[1]);

    } else if (strcmp(command_args[0], "my_ls") == 0) {
        // no arguments — just list the current directory
        if (args_size != 1)
            return badcommand();
        return my_ls();

    } else if (strcmp(command_args[0], "my_mkdir") == 0) {
        if (args_size != 2)
            return badcommand();
        return my_mkdir(command_args[1]);

    } else if (strcmp(command_args[0], "source") == 0) {
        if (args_size != 2)
            return badcommand();
        return source(command_args[1]);

    } else
        return badcommand();
}

int help() {

    // note the literal tab characters here for alignment
    char help_string[] = "COMMAND			DESCRIPTION\n \
help			Displays all the commands\n \
quit			Exits / terminates the shell with “Bye!”\n \
set VAR STRING		Assigns a value to shell memory\n \
print VAR		Displays the STRING assigned to VAR\n \
echo STRING		Displays STRING or value of $VAR from shell memory\n \
my_ls			Lists files and directories in current directory (alphabetical)\n \
my_mkdir DIRNAME	Creates a new directory (dirname or $VAR from shell memory)\n \
source SCRIPT.TXT	Executes the file SCRIPT.TXT\n ";
    printf("%s\n", help_string);
    return 0;
}

int quit() {
    printf("Bye!\n");
    exit(0);
}

int set(char *var, char *value) {
    // Challenge: allow setting VAR to the rest of the input line,
    // possibly including spaces.

    // Hint: Since "value" might contain multiple tokens, you'll need to loop
    // through them, concatenate each token to the buffer, and handle spacing
    // appropriately. Investigate how `strcat` works and how you can use it
    // effectively here.

    mem_set_value(var, value);
    return 0;
}


int print(char *var) {
    printf("%s\n", mem_get_value(var));
    return 0;
}

// Echo: print the token, or the value of $VAR from shell memory (blank line if not set).
int echo(char *token) {
    if (token[0] == '$') {
        // Token is $varname — look up variable in shell memory
        char *var_name = token + 1;
        char *value = mem_get_value(var_name);
        if (strcmp(value, "Variable does not exist") == 0) {
            printf("\n");
        } else {
            printf("%s\n", value);
            free(value);  // strdup'd by mem_get_value
        }
        return 0;
    } else {
        // Plain string — print it as-is
        printf("%s\n", token);
        return 0;
    }
}

/* compare numbers before letters, uppercase before same lowercase, alphabetical */
static int my_ls_compare(const void *a, const void *b) {
    return strcmp(*(const char *const *)a, *(const char *const *)b);
}

// List current directory: names only (no recursion), one per line, alphabetical. Includes . and ..
int my_ls(void) {
    DIR *dir = opendir(".");
    if (dir == NULL)
        return 1;

    /* collect all names (we need copies because readdir reuses its buffer) */
    char *names[256];
    int count = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
        names[count++] = strdup(entry->d_name);
    closedir(dir);

    /* sort then print one per line */
    qsort(names, count, sizeof(char *), my_ls_compare);
    for (int i = 0; i < count; i++) {
        printf("%s\n", names[i]);
        free(names[i]);
    }
    return 0;
}

// True if s is non-empty and every char is 0-9, A-Z, or a-z (for my_mkdir $var value).
static int is_single_alnum_token(const char *s) {
    if (s == NULL || s[0] == '\0')
        return 0;
    // Reject if any character is not alphanumeric
    for (; *s; s++) {
        char c = *s;
        if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')))
            return 0;
    }
    return 1;
}

// Create a directory: dirname is the name, or $varname (then we use that variable's value if single alnum).
int my_mkdir(char *dirname) {
    char *name_to_use = dirname;

    if (dirname[0] == '$') {
        // Resolve $varname from shell memory
        char *var_name = dirname + 1;
        char *value = mem_get_value(var_name);
        if (strcmp(value, "Variable does not exist") == 0) {
            printf("Bad command: my_mkdir\n");
            return 1;
        }
        if (!is_single_alnum_token(value)) {
            printf("Bad command: my_mkdir\n");
            free(value);
            return 1;
        }
        name_to_use = value;
    }

    // Create directory with given name (0755 = rwxr-xr-x)
    if (mkdir(name_to_use, 0755) != 0) {
        if (name_to_use != dirname)
            free(name_to_use);
        return 1;
    }
    if (name_to_use != dirname)
        free(name_to_use);
    return 0;
}

int source(char *script) {
    int errCode = 0;
    char line[MAX_USER_INPUT];
    FILE *p = fopen(script, "rt");      // the program is in a file

    if (p == NULL) {
        return badcommandFileDoesNotExist();
    }

    fgets(line, MAX_USER_INPUT - 1, p);
    while (1) {
        errCode = parseInput(line);     // which calls interpreter()
        memset(line, 0, sizeof(line));

        if (feof(p)) {
            break;
        }
        fgets(line, MAX_USER_INPUT - 1, p);
    }

    fclose(p);

    return errCode;
}
