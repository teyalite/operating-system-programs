/**
 * Created by Abdoulkader Abdourhamane Haidara on 19.02.2022
*/
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/wait.h>

void output(ssize_t errors, ssize_t warnings) {
    printf("Unique lines with warnings: %zd\n", warnings);
    printf("Unique lines with errors: %zd\n", errors);
}

int main(__attribute__((unused)) int argc, char** args) {
    const char* file_name = args[1];
    const char* exec = "main";
    /// creating pipe
    int fds[2];
    pipe(fds);
    /// forking
    pid_t pid = fork();

    if (pid == 0) {
        /// child process, child doesn't read
        close(fds[0]);
        /// stderr => pipe's write, writing errors
        dup2(fds[1], 2);
        /// try to compile the .c file
        execlp("gcc", "gcc", file_name, "-o", exec, NULL);
    } else {
        /// parent doesn't write
        close(fds[1]);
        /// wait for the child to terminate
        waitpid(pid, NULL, 0);

        bool start_line_flag = true;
        ssize_t errors = 0;
        ssize_t warnings = 0;
        ssize_t previous_error_line = 0;
        ssize_t previous_warning_line = 0;

        char a;
        ssize_t read_position = read(fds[0], &a, sizeof(a));

        while (read_position > 0) {
            if (start_line_flag) {
                /// start of a new line
                ssize_t current_index = 0;
                while (a == file_name[current_index]) {
                    if (strlen(file_name) > current_index + 1) {
                        /// filename is still not complete
                        ++current_index;
                        read_position = read(fds[0], &a, sizeof(a));
                        continue;
                    }
                    /// filename and beginning of the line are equal which means that the current line is important.
                    /// jump the first :
                    read(fds[0], &a, sizeof(a));
                    read_position = read(fds[0], &a, sizeof(a));
                    /// get line number
                    char string_number[200] = "";

                    while (a != ':' && a != ' ') {
                        strncat(string_number, &a, 1);
                        read_position = read(fds[0], &a, sizeof(a));
                    }

                    if (a == ' ') break;

                    ssize_t current_line = strtol(string_number, NULL, 10);
                    /// jumping :word_position
                    while (a != ' ') read(fds[0], &a, sizeof(a));
                    read_position = read(fds[0], &a, sizeof(a));
                    /// is warning or error
                    if (a == 'e') {
                        /// error
                        errors += (current_line == previous_error_line ? 0 : 1);
                        previous_error_line = current_line;
                    }

                    if (a == 'w') {
                        /// warning
                        warnings += (current_line == previous_warning_line ? 0 : 1);
                        previous_warning_line = current_line;
                    }

                    break;
                }

                start_line_flag = false;
                continue;
            } else if (a == '\n') {
                /// end of a line
                start_line_flag = true;
            }

            read_position = read(fds[0], &a, sizeof(a));
        }


        /// file compiled successfully
        if (errors == 0) {

            pid_t pid2 = fork();

            if (pid2 == 0) {
                execlp("rm", "rm", "-f", exec, NULL);
            }

            waitpid(pid2, NULL, 0);
        }

        output(errors, warnings);
        close(fds[0]);
    }

    return 0;
}