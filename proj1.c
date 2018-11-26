/*
 * Author: Jan Svabik, xsvabi00
 * Version: 2.1
 * Created: 2018-10-26
 * Last update: 2018-10-31
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// max line length
#define LINE_MAX 2048

// declare program startup functions
void checkProgramUsage(int argc);
void openCommandsFile(char* filename);

// declare program flow control functions
void runLoop(char* currentLine, char* currentCommand, int lineToEdit, int currentLineNum, int deleted, int searchMode, char* toFound);
int flowControlCommand(char* currentCommand, char* toFound, int* deleted, int* searchMode, int* lineToEdit);
void handleSearching(char* currentLine, char* toFound, int* searchMode, int* currentLineNum, int lineToEdit);
void editLine(char* currentCommand, char* currentLine);

// declare auxiliary functions
char* commandArgument(char* command);
char* removeEOL(char* s);
char* addEOL(char* s);
void resetFilePointer(FILE* f);
void setFilePointer(FILE* f, int line, char* currentCommand);
char* substr(char* s, unsigned int from, unsigned int to);
int strpos(char* find, char* in);
char* strreplace(char* find, char* replace, char* in);
char* strreplaceall(char* find, char* replace, char* in);
void quit();

// declare editor functions
void newLine(char* arg, int* lineToEdit);
void deleteLine(char* arg, int* deleted, int* lineToEdit);
void prependLine(char* s);
void prependString(char* s, char* currentLine);
void appendString(char* s, char* currentLine);
void gotoCommand(const char* arg, char* currentCommand);
void conditionedGotoCommand(char* arg, char* currentCommand, char* currentLine);
void replaceFirst(char* arg, char* currentLine);
void replaceAll(char* arg, char* currentLine);
void findAndJumpTo(char* pattern, char* toFound, int* searchMode);

// declare command file pointer
FILE* commands = NULL;

/// MAIN ///
int main(int argc, char* argv[]) {
    // check that the command file was given in argument
    checkProgramUsage(argc);

    // open file (and check for error while opening)
    openCommandsFile(argv[1]);

    /*
     * DECLARE VARIABLES
     */
    // line content variables
    char currentLine[LINE_MAX];
    char currentCommand[LINE_MAX];
    
    // line control variables
    int lineToEdit = 1;
    int currentLineNum = 1;
    int deleted = 0;

    // vars for searching
    int searchMode = 0;
    char toFound[LINE_MAX];

    /*
     * RUN THE MAIN LOOP
     */
    runLoop(currentLine, currentCommand, lineToEdit, currentLineNum, deleted, searchMode, toFound);

    // close the commands file
    fclose(commands);

    // good job!
    return 0;
}

/************************************/
/** PROGRAM FLOW CONTROL FUNCTIONS **/
/************************************/

// the main loop
void runLoop(char* currentLine, char* currentCommand, int lineToEdit, int currentLineNum, int deleted, int searchMode, char* toFound) {
    // load new line
    while (fgets(currentLine, LINE_MAX, stdin) != NULL) {
        // check that some string have to be searched for
        handleSearching(currentLine, toFound, &searchMode, &currentLineNum, lineToEdit);

        // load new command (if current line is the line which has to be edited)
        while (!searchMode && currentLineNum == lineToEdit && fgets(currentCommand, LINE_MAX, commands) != NULL) {
            // check that current command is not empty line/command
            if (currentCommand[0] == '\n')
                continue;
            
            // remove EOL from the end of the current command
            strcpy(currentCommand, removeEOL(currentCommand));

            // if command q, n or d: do its job and break command loop (don't edit the line)
            if (flowControlCommand(currentCommand, toFound, &deleted, &searchMode, &lineToEdit))
                break;

            // do the job depending to the command and its arg(s)
            editLine(currentCommand, currentLine);
        }

        // end the line loop if 'q' command
        if (currentCommand[0] == 'q')
            break;

        // deletion check (and printing the line if not deleted)
        if (deleted)
            deleted--;
        else
            printf("%s", currentLine);

        // increment current line number
        currentLineNum++;
    }
}

// check currentCommand - if it is control command, do its job or idle
int flowControlCommand(char* currentCommand, char* toFound, int* deleted, int* searchMode, int* lineToEdit) {
    int ret = 1;
    char c = currentCommand[0];

    // set new line to edit
    if (c == 'n')
        newLine(commandArgument(currentCommand), lineToEdit);
    // jump to line with pattern
    else if (c == 'f')
        findAndJumpTo(commandArgument(currentCommand), toFound, searchMode);
    // delete line(s)
    else if (c == 'd')
        deleteLine(commandArgument(currentCommand), deleted, lineToEdit);
    // stop editing
    else if (c == 'q')
        ;
    // not control command
    else
        ret = 0;

    return ret;
}

// do the edit command
void editLine(char* currentCommand, char* currentLine) {
    char c = currentCommand[0];

    // append content
    if (c == 'a')
        appendString(commandArgument(currentCommand), currentLine);
    // prepend content
    else if (c == 'b')
        prependString(commandArgument(currentCommand), currentLine);
    // prepend line
    else if (c == 'i')
        prependLine(commandArgument(currentCommand));
    // remove EOL
    else if (c == 'r')
        strcpy(currentLine, removeEOL(currentLine));
    // add EOL
    else if (c == 'e')
        strcpy(currentLine, addEOL(currentLine));
    // goto
    else if (c == 'g')
        gotoCommand(commandArgument(currentCommand), currentCommand);
    // conditioned goto
    else if (c == 'c')
        conditionedGotoCommand(commandArgument(currentCommand), currentCommand, currentLine);
    // replace first
    else if (c == 's')
        replaceFirst(commandArgument(currentCommand), currentLine);
    // replace all
    else if (c == 'S')
        replaceAll(commandArgument(currentCommand), currentLine);
    // wrong command, exit
    else {
        printf("Unknown command [%c].\n", c);
        quit(1);
    }
}

// if search mode active, do the search iteration job
void handleSearching(char* currentLine, char* toFound, int* searchMode, int* currentLineNum, int lineToEdit) {
    if (*searchMode && strpos(toFound, currentLine) != -1) {
        *searchMode = 0;
        *currentLineNum = lineToEdit;
        memset(toFound, 0, LINE_MAX);
    }
}

/*******************************/
/** PROGRAM STARTUP FUNCTIONS **/
/*******************************/

// check program arguments (view help and exit or do nothing)
void checkProgramUsage(int argc) {
    if (argc != 2) {
        printf("Missing command file.\n\nUsage: ./proj1 filewithcommands [< filewithtext]\n\nI am a simple text editor you will fall in love with.\n");
        quit(0);
    }
}

// open file with commands
void openCommandsFile(char* filename) {
    commands = fopen(filename, "r");
    if (commands == NULL) {
        printf("Error while trying to open the file with arguments. Maybe it doesn't exist.\n");
        quit(1);
    }
}

/**********************/
/** EDITOR FUNCTIONS **/
/**********************/

// do the job for 'n' command
void newLine(char* arg, int* lineToEdit) {
    if (arg[0] == '\0')
        *lineToEdit += 1;
    else
        *lineToEdit += strtol(arg, NULL, 10);
}

// do the job for 'd' command
void deleteLine(char* arg, int* deleted, int* lineToEdit) {
    if (arg[0] == '\0') {
        *lineToEdit += 1;
        *deleted += 1;
    }
    else {
        int skip = strtol(arg, NULL, 10);
        *lineToEdit += skip;
        *deleted += skip;
    }
}

// prepend line
void prependLine(char* s) {
    printf("%s\n", s);
}

// prepend string
void prependString(char* s, char* currentLine) {
    if (LINE_MAX < strlen(s) + strlen(currentLine)) {
        printf("Reached max line length when trying to do something with the string given.\n");
        quit(1);
    }

    strcpy(currentLine, strcat(s, currentLine));
}

// append string
void appendString(char* s, char* currentLine) {
    if (LINE_MAX < strlen(s) + strlen(currentLine)) {
        printf("Reached max line length when trying to do something with the string given.\n");
        quit(1);
    }

    strcpy(currentLine, strcat(removeEOL(currentLine), addEOL(s)));
}

// goto command
void gotoCommand(const char* arg, char* currentCommand) {
    if (arg[0] == '\0') {
        printf("Line number not specified in goto command.\n");
        quit(1);
    }

    int line = strtol(arg, NULL, 10);
    setFilePointer(commands, line, currentCommand);
}

// conditioned goto command
void conditionedGotoCommand(char* arg, char* currentCommand, char* currentLine) {
    char line[LINE_MAX];
    char pattern[LINE_MAX];

    int spacePos = strpos(" ", arg);

    strcpy(line, substr(arg, 0, spacePos-1));
    strcpy(pattern, substr(arg, spacePos+1, strlen(arg)-1));

    if (strpos(pattern, currentLine) != -1)
        gotoCommand(line, currentCommand);
}

// replace first match with pattern at the current line
void replaceFirst(char* arg, char* currentLine) {
    char* pattern;
    char* replacement;

    char delimiter[2];
    delimiter[0] = arg[0];
    delimiter[1] = '\0';

    pattern = strtok(arg, delimiter);
    replacement = strtok(NULL, delimiter);

    strcpy(currentLine, strreplace(pattern, replacement, currentLine));
}

// replace all matches with pattern at the current line
void replaceAll(char* arg, char* currentLine) {
    char* pattern;
    char* replacement;

    char delimiter[2];
    delimiter[0] = arg[0];
    delimiter[1] = '\0';

    pattern = strtok(arg, delimiter);
    replacement = strtok(NULL, delimiter);

    strcpy(currentLine, strreplaceall(pattern, replacement, currentLine));
}

// run searching loop (print/skip all lines until some of them includes pattern)
void findAndJumpTo(char* pattern, char* toFound, int* searchMode) {
    *searchMode = 1;
    strcpy(toFound, pattern);
}

/*************************/
/** AUXILIARY FUNCTIONS **/
/*************************/

// exit the program (close commands file and exit())
void quit(int error) {
    // close commands file only if the file was opened before
    if(commands != NULL)
        fclose(commands);
    
    // exit depending to the 'error' argument (success/failure)
    if (error == 0)
        exit(EXIT_SUCCESS);
    else
        exit(EXIT_FAILURE);
}

// get command argument (e.g. aCONTENT -> CONTENT)
char* commandArgument(char* command) {
    static char buffer[LINE_MAX];
    memset(buffer, 0, LINE_MAX);

    for (unsigned int i = 1; command[i] != '\0'; i++)
        buffer[i-1] = command[i];

    return buffer;
}

// remove EOL from string
char* removeEOL(char* s) {
    static char buffer[LINE_MAX];
    strcpy(buffer, s);

    for (unsigned int i = 0; i < strlen(buffer); i++)
        if (buffer[i] == '\n')
            buffer[i] = '\0';

    return buffer;
}

// add EOL to the end of the string
char* addEOL(char* s) {
    if (strlen(s) == LINE_MAX) {
        printf("Reached max line length when trying to do something with the string given.\n");
        quit(1);
    }

    static char buffer[LINE_MAX];
    strcpy(buffer, strcat(s, "\n"));

    return buffer;
}

// reset file pointer
void resetFilePointer(FILE* f) {
    if (fseek(f, 0L, SEEK_SET) != 0) {
        printf("Error when trying to move the file pointer to the start of the file.\n");
        quit(1);
    }
}

// set file pointer
void setFilePointer(FILE* f, int line, char* currentCommand) {
    resetFilePointer(f);

    for (int i = 1; i < line; i++)
        if (fgets(currentCommand, LINE_MAX, commands) == NULL)
            break;
}

// return substring (from index to index)
char* substr(char* s, unsigned int from, unsigned int to) {
    static char buffer[LINE_MAX];
    memset(buffer, 0, LINE_MAX);

    int j = 0;

    for (unsigned int i = 0; i < strlen(s); i++)
        if (i >= from && i <= to)
            buffer[j++] = s[i];

    return buffer;
}

// return index of first char of string find position in string in
int strpos(char* find, char* in) {
    char* x = strstr(in, find);
    if (x == NULL)
        return -1;

    return strlen(in) - strlen(x);
}

// replace string find by string replace in string in
char* strreplace(char* find, char* replace, char* in) {
    // prepare buffer
    static char buffer[LINE_MAX];
    memset(buffer, 0, LINE_MAX);

    // length of replace string (must be signed int for comparing with pos (strpos)
    int replaceLen = strlen(replace);

    // get position of string find
    int pos = strpos(find, in);

    // if not found, return -1
    if (pos == -1)
        return in;

    // prepare loop control variable
    int i = 0;
    int j = 0; // position in string in

    // add part before pos
    while (i < pos) {
        buffer[i] = in[i];
        i++;
        j++;
    }

    // add replacement
    while (i - pos < replaceLen) {
        buffer[i] = replace[i-pos];
        i++;
    }

    j += strlen(find);

    // add the remaining part of string in
    while (in[j] != '\0') {
        buffer[i] = in[j];
        i++;
        j++;
    }

    return buffer;
}

// replace all found find by string replace in string in
char* strreplaceall(char* find, char* replace, char* in) {
    static char buffer[LINE_MAX];
    memset(buffer, 0, LINE_MAX);

    char sub[LINE_MAX];
    memset(sub, 0, LINE_MAX);

    for (unsigned int i = 0; i < strlen(in);) {
        memset(sub, 0, LINE_MAX);
        strcpy(sub, substr(in, i, strlen(in)-1));
        int pos = strpos(find, sub);

        if (pos > -1) {
            // add part before pos
            strcpy(buffer + strlen(buffer), substr(sub, 0, pos-1));

            // add replacement
            strcpy(buffer + strlen(buffer), substr(replace, 0, strlen(replace)-1));

            // move in original string
            i += pos + strlen(find);
        }
        else {
            // add the end of the string
            strcpy(buffer + strlen(buffer), substr(sub, 0, strlen(sub)));
            break;
        }
    }

    return buffer;
}