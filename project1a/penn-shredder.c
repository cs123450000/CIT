#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
    if (kill(childPid, SIGKILL) == -1) {
        perror("Error in kill");
        exit(EXIT_FAILURE);
    }
}

/* Signal handler for SIGALRM. Catches SIGALRM signal and
 * kills the child process if it exists and is still executing.
 * It then prints out penn-shredder's catchphrase to standard output */
void alarmHandler(int sig)
{
    if (childPid != 0){
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
    if (signal (SIGALRM, alarmHandler) == SIG_ERR){
       perror("Error in signal");
       exit(EXIT_FAILURE);
    }
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
    char* command;
    int status;
    char minishell[] = "penn-shredder# ";
    writeToStdout(minishell);
    
    command = getCommandFromInput();
    alarm(timeout);
    if (command != NULL) {
        childPid = fork();
        if (childPid < 0) {
            perror("Error in creating child process");
            free (command);
            exit(EXIT_FAILURE);
        }
        if (childPid == 0) {
            char* const envVariables[] = { NULL };
            char* const args[] = { command, NULL };
            if (execve(command, args, envVariables) == -1) {
                perror("Error in execve");
                free (command);
                exit(EXIT_FAILURE);
            }
        } else {
            do {
                if (wait(&status) == -1) {
                    perror("Error in child process termination");
                    free (command);
                    exit(EXIT_FAILURE);
                }
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));
            alarm(0);
        }
    }
    free(command);
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
        exit(EXIT_FAILURE);
    }
    //Check if read error
    if (nread == -1) {
        perror("Error in read");
        exit(EXIT_FAILURE);
    }

    //trimming white spaces and new line;
    user_input[nread] = '\0';
    char* token = strtok(user_input, separator);
    char* command = malloc(INPUT_SIZE*sizeof(char));
    if (token != NULL) {
        for (int m = 0; m < strlen(token); m++) {
            command[m] = token[m];
        }
    } else {
        command = NULL;
    }
    return command;
}
