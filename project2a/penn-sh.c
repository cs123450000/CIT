#include "tokenizer.h"
#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define INPUT_SIZE 1024

pid_t childPid = 0;

char** redirect();

void executeShell(int timeout);

void writeToStdout(char* text);

void alarmHandler(int sig);

void sigintHandler(int sig);

char* getCommandFromInput();

void registerSignalHandlers();

void killChildProcess();

int main(int argc, char** argv)
{
    registerSignalHandlers();

    int timeout = 0;

    if (argc == 2) {
        timeout = atoi(argv[1]);
    }

    if (timeout < 0) {
        writeToStdout("Invalid input detected. Ignoring timeout value.\n");
        timeout = 0;
    }

    while (1) {
        executeShell(timeout);
    }

    return 0;
}

/* Sends SIGKILL signal to a child process.
 * Error checks for kill system call failure and exits program if
 * there is an error */
void killChildProcess()
{
    kill(childPid, SIGKILL);
    //if (kill(childPid, SIGKILL) == -1) {
        //perror("Error in kill");
        //exit(EXIT_FAILURE);
    
}

/* Signal handler for SIGALRM. Catches SIGALRM signal and
 * kills the child process if it exists and is still executing.
 * It then prints out penn-shredder's catchphrase to standard output */
void alarmHandler(int sig)
{
    if (childPid != 0) {
        killChildProcess();
        writeToStdout("Bwahaha ... tonight I dine on turtle soup\n");
    }
}

/* Signal handler for SIGINT. Catches SIGINT signal (e.g. Ctrl + C) and
 * kills the child process if it exists and is executing. Does not
 * do anything to the parent process and its execution */
void sigintHandler(int sig)
{
    if (childPid != 0) {
        killChildProcess();
    }
}

/* Registers SIGALRM and SIGINT handlers with corresponding functions.
 * Error checks for signal system call failure and exits program if
 * there is an error */
void registerSignalHandlers()
{
    if (signal(SIGINT, sigintHandler) == SIG_ERR) {
        perror("Error in signal");
        exit(EXIT_FAILURE);
    }
}

/* Prints the shell prompt and waits for input from user.
 * Takes timeout as an argument and starts an alarm of that timeout period
 * if there is a valid command. It then creates a child process which
 * executes the command with its arguments.
 *
 * The parent process waits for the child. On unsuccessful completion,
 * it exits the shell. */
void executeShell(int timeout)
{
    int status;
    char minishell[] = "penn-sh>";
    writeToStdout(minishell);

    //command = getCommandFromInput();
    //alarm(timeout);
    TOKENIZER* tokenizer;
    char string[256] = "";
    char* filename;
    int br;
    char* tok;
    int input_num = 0;
    int output_num = 0;
    int new_stdin, new_stdout;
    char** args;
    int i = 1;
    args = malloc(100 * sizeof(char*));

    /*read string from user input*/
    br = read(STDIN_FILENO, string, INPUT_SIZE);
    if (br == 0) {
        perror("End of File");
        exit(EXIT_SUCCESS);
    }
    //Check if read error
    if (br == -1) {
        perror("Error in read");
        exit(EXIT_FAILURE);
    }
    if (strcmp(string, "") == 0) {
        args = NULL;
        args[0] = NULL;
    } else {
        string[br - 1] = '\0';
        /*tokenize string*/
        tokenizer = init_tokenizer(string);
        args[0] = get_next_token(tokenizer);
    }
    //command = getCommandFromInput();
    if (args[0] != NULL) {
        childPid = fork();
        if (childPid < 0) {
            perror("Error in creating child process");
            exit(EXIT_FAILURE);
        }
        if (childPid == 0) {
            while ((tok = get_next_token(tokenizer)) != NULL) {
                //printf("gottoken %s\n", tok);
                if (tok[0] == '<') {      //input redirection
                    if (input_num == 1) { //check if multiple input redirects
                        printf("Invalid: Multiple standard input redirects or redirect in invalid location\n");
                        free(tok);
                        free(tokenizer);
                        exit(EXIT_FAILURE);
                    }
                    /*check for open error*/
                    filename = get_next_token(tokenizer); //get the filename
                    //printf("The intput filename is %s\n", filename);
                    if ((new_stdin = open(filename, O_RDONLY, 0644)) < 0) {
                        perror("Invalid standard input redirect");
                        free(tok);
                        free(tokenizer);
                        exit(EXIT_FAILURE);
                    }
                    dup2(new_stdin, STDIN_FILENO);
                    close(new_stdin);
                    input_num = 1;
                } else if (tok[0] == '>') { //output redirection
                    if (output_num == 1) {
                        perror("Invalid: Multiple standard output redirects");
                        free(tok);
                        free(tokenizer);
                        exit(EXIT_FAILURE);
                    }
                    /*check for open error*/
                    filename = get_next_token(tokenizer);
                    if ((new_stdout = open(filename, O_WRONLY | O_TRUNC | O_CREAT, 0644)) < 0) {
                        perror("Invalid standard output");
                        exit(EXIT_FAILURE);
                    }
                    dup2(new_stdout, STDOUT_FILENO);
                    close(new_stdout);
                    output_num = 1;
                } else { //neither input nor output redirection
                    args[i] = malloc((strlen(tok) + 1) * sizeof(char));
                    strcpy(args[i], tok);
                    //printf("arg[i] is %s\n", args[i]);
                    i++;
                    free(args[i]);
                }
                free(tok);
            }
            args[i] = NULL;
            //printf("args[0] is %s\n", args[0]);
            //printf("args[1] is %s\n", args[1]);
            free(tokenizer);
            if (execvp(args[0], args) == -1) {
                perror("Error in exexvp");
                exit(EXIT_FAILURE);
            }
            free(args);
        } else {
            do {
                if (wait(&status) == -1) {
                    perror("Error in child process termination");
                    //free(command);
                    exit(EXIT_FAILURE);
                }
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));
            //alarm(0);
        }
    }
}

/* Writes particular text to standard output */
void writeToStdout(char* text)
{
    if (write(STDOUT_FILENO, text, strlen(text)) == -1) {
        perror("Error in write");
        exit(EXIT_FAILURE);
    }
}

/* Reads input from standard input till it reaches a new line character.
 * Checks if EOF (Ctrl + D) is being read and exits penn-shredder if that is the case
 * Otherwise, it checks for a valid input and adds the characters to an input buffer.
 *
 * From this input buffer, the first 1023 characters (if more than 1023) or the whole
 * buffer are assigned to command and returned. An \0 is appended to the command so
 * that it is null terminated */
char* getCommandFromInput()
{
    char user_input[INPUT_SIZE];
    char* separator = " \n";
    int nread = read(STDIN_FILENO, user_input, (INPUT_SIZE - 1));

    //Check if EOF is being read
    if (nread == 0) {
        perror("End of File");
        //exit(EXIT_FAILURE);
    }
    //Check if read error
    if (nread == -1) {
        perror("Error in read");
        exit(EXIT_FAILURE);
    }

    //trimming white spaces and new line;
    user_input[nread] = '\0';
    char* token = strtok(user_input, separator);
    char* command = malloc(INPUT_SIZE * sizeof(char));
    if (token != NULL) {
        for (int m = 0; m < strlen(token); m++) {
            command[m] = token[m];
        }
    } else {
        command = NULL;
    }
    return command;
}
/*
void redirect()
{
    TOKENIZER* tokenizer;
    char string[256] = "";
    char* filename;
    int br;
    char* tok;
    int input_num = 0;
    int output_num = 0;
    int new_stdin, new_stdout;
    char** args;
    int i = 1;
    args = malloc(100 * sizeof(char*));

    read string from user input
    br = read(STDIN_FILENO, string, INPUT_SIZE);
    if (strcmp(string, "") == 0) {
        args = NULL;
    }
    if (br == 0) {
        perror("End of File");
        exit(EXIT_FAILURE);
    }
    //Check if read error
    if (br == -1) {
        perror("Error in read");
        exit(EXIT_FAILURE);
    }
    string[br - 1] = '\0';
    //tokenize string
    tokenizer = init_tokenizer(string);
    args[0] = get_next_token(tokenizer);

    //command = getCommandFromInput();
    if (args != NULL) {
        childPid = fork();
        if (childPid < 0) {
            perror("Error in creating child process");
            exit(EXIT_FAILURE);
        }
        if (childPid == 0) {
            while ((tok = get_next_token(tokenizer)) != NULL) {
                //printf("gottoken %s\n", tok);
                if (tok[0] == '<') {      //input redirection
                    if (input_num == 1) { //check if multiple input redirects
                        printf("Invalid: Multiple standard input redirects or redirect in invalid location\n");
                        free(tok);
                        free(tokenizer);
                        exit(EXIT_FAILURE);
                    }
                    //check for open error
                    filename = get_next_token(tokenizer); //get the filename
                    //printf("The intput filename is %s\n", filename);
                    if ((new_stdin = open(filename, O_RDONLY, 0644)) < 0) {
                        perror("Invalid standard input redirect");
                        free(tok);
                        free(tokenizer);
                        exit(EXIT_FAILURE);
                    }
                    dup2(new_stdin, STDIN_FILENO);
                    close(new_stdin);
                    input_num = 1;
                } else if (tok[0] == '>') { //output redirection
                    if (output_num == 1) {
                        perror("Invalid: Multiple standard output redirects");
                        free(tok);
                        free(tokenizer);
                        exit(EXIT_FAILURE);
                    }
                    //check for open error
                    filename = get_next_token(tokenizer);
                    if ((new_stdout = open(filename, O_WRONLY | O_TRUNC | O_CREAT, 0644)) < 0) {
                        perror("Invalid standard output");
                        exit(EXIT_FAILURE);
                    }
                    dup2(new_stdout, STDOUT_FILENO);
                    close(new_stdout);
                    output_num = 1;
                } else { //neither input nor output redirection
                    args[i] = malloc((strlen(tok) + 1) * sizeof(char));
                    strcpy(args[i], tok);
                    //printf("arg[i] is %s\n", args[i]);
                    i++;
                    free(args[i]);
                }
                free(tok);
            }
            args[i] = NULL;
            printf("args[0] is %s\n", args[0]);
            free(tokenizer);
        }
    }
}*/
