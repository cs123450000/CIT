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

void executeShell(int timeout);

void writeToStdout(char* text);

void alarmHandler(int sig);

void sigintHandler(int sig);

char* getCommandFromInput();

void registerSignalHandlers();

void killChildProcess();

void pipeline(char** args, int n);

void redirect(char** args);

void regularprocess(char** args);

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

void pipeline(char** args, int n)
{
    pid_t pid;
    int status;
    int fp[2];

    char** args1 = malloc(sizeof(char*) * (n + 1));
    for (int i = 0; i < n; i++) {
        args1[i] = strdup(args[i]);
    }
    args1[n] = 0;
    //char *in[] = {"cat","test",NULL};
    //char *out[] = {"grep","brain",NULL};

    if (pipe(fp) == -1) {
        perror("pipe error");
        exit(EXIT_FAILURE);
    }
    pid = fork();
    if (pid == 0) {
        dup2(fp[1], STDOUT_FILENO);
        close(fp[0]);
        close(fp[1]);
        redirect(args1);
        //redirect(in);
        exit(1);
    } else {
        if (fork() == 0) {
            dup2(fp[0], STDIN_FILENO);
            close(fp[1]);
            close(fp[0]);
            redirect(args + n + 1);
            //redirect(out);
            exit(1);
        } else {
            for (int i = 0; i < 2; i++) {
                close(fp[0]);
                close(fp[1]);
                wait(&status);
                //wait(&status);
            }
        }
    }
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
    TOKENIZER* tokenizer;
    char string[256] = "";
    int br;
    char* tok;
    int k = 0;
    int input_num = 0;
    int output_num = 0;
    char** args;
    int i = 1;
    int pipecount = 0;
    int pipeloc;
    args = malloc(100 * sizeof(char*));
    int first_input = 0;
    int first_output = 0;
    int second_input, second_output;

    char minishell[] = "penn-sh>";
    writeToStdout(minishell);

    /*read string from user input*/
    br = read(STDIN_FILENO, string, INPUT_SIZE);
    //Check if read error
    if (br == -1) {
        perror("Error in read");
        exit(EXIT_FAILURE);
    }
    if (br == 0) {
        exit(EXIT_SUCCESS);
    }
    if (strcmp(string, "") == 0) {
        args = NULL;
        args[0] = NULL;
    } else {
        string[br - 1] = '\0';
        /*tokenize string*/
        tokenizer = init_tokenizer(string);
        /*save the list of tokens into arguments*/
        while ((tok = get_next_token(tokenizer)) != NULL) {
            args[k] = tok;
            k++;
        }
    }
    if (args[0] != NULL) {
        while (args[i] != NULL) {
            if (strcmp(args[i], "<") == 0) {
                input_num++;
            } else if (strcmp(args[i], ">") == 0) {
                output_num++;
            } else if (strcmp(args[i], "|") == 0) {
                pipecount++;
                pipeloc = i;
            }
            i++;
        }
        if (pipecount == 0) {
            if (input_num <= 1 && output_num <= 1) {
                redirect(args);
            } else {
                if (input_num >= 2) {
                    printf("Invalid input redirection\n");
                }
                if (output_num >= 2) {
                    printf("Invalid output redirection\n");
                }
            }
        } else if (pipecount == 1) {
            int k = 1;
            while (k < pipeloc) {
                if (strcmp(args[k], "<") == 0) {
                    first_input++;
                } else if (strcmp(args[k], ">") == 0) {
                    first_output++;
                }
                k++;
            }
            second_input = input_num - first_input;
            second_output= output_num - first_output;
            if ((first_input <= 1) && (first_output == 0) && (second_input == 0) && (second_output <= 1)) {
                pipeline(args, pipeloc);
            } else {
                printf("Invalid redirection\n");
            }
        } else {
            printf("Invalid many pipes\n");
        }
    }
}

/*This redirect function redirects STD input or output*/
void redirect(char** args)
{
    /*read string from user input*/
    pid_t childPid;
    int status = 0;
    int new_stdout, new_stdin;

    int i = 1;
    int input_num = 0;
    int output_num = 0;
    int m = 1;
    char** trimmed = malloc(100 * sizeof(char*));
    if (args[0] == NULL) {
        exit(0);
    }
    trimmed[0] = args[0];

    //Check if pipe error
    //command = getCommandFromInput();
    childPid = fork();
    if (childPid < 0) {
        perror("Error in creating child process");
        exit(EXIT_FAILURE);
    }
    if (childPid == 0) {
        while (args[i] != NULL) {
            if (strcmp(args[i], "<") == 0) {
                if ((new_stdin = open(args[i + 1], O_RDONLY, 0644)) < 0) {
                    printf("Invalid standard input redirect\n");
                    break;
                } else {
                    dup2(new_stdin, STDIN_FILENO);
                    close(new_stdin);
                    input_num++;
                }
            } else if (strcmp(args[i], ">") == 0) {
                if ((new_stdout = open(args[i + 1], O_WRONLY | O_TRUNC | O_CREAT, 0644)) < 0) {
                    printf("Invalid standard output");
                    break;
                } else {
                    dup2(new_stdout, STDOUT_FILENO);
                    close(new_stdout);
                    output_num++;
                }
            } else {
                trimmed[m] = args[i];
            }
            i++;
            m++;
        }
        if (execvp(trimmed[0], trimmed) == -1) {
            perror("Invalid: Error in exexvp");
            exit(EXIT_FAILURE);
        }
    } else {
        do {
            if (wait(&status) == -1) {
                perror("Error in child process termination");
                //free(command);
                exit(EXIT_FAILURE);
            }
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
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
