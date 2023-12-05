#include <stdio.h>
#include <pthread.h>


// This function prints a user error message along with details to stderr
// and terminates the thread using pthread_exit.
void DieWithUserMessage(const char *msg, const char *detail) {
    // Print the user error message and details to stderr
    fputs(msg, stderr);
    fputs(": ", stderr);
    fputs(detail, stderr);
    fputc('\n', stderr);

    // Terminate the thread
    pthread_exit(nullptr);
}

// This function prints a system error message using perror and terminates the thread.
void DieWithSystemMessage(const char *msg) {
    // Print the system error message using perror
    perror(msg);

    // Terminate the thread
    pthread_exit(nullptr);
}
