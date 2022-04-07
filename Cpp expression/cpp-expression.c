#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

const char *program = "#include <iostream>\nint main(int argc, char** argv) {\n  std::cout << %s << std::endl;\n  return 0;\n}";

int main(int argc, char **argv)
{
    pid_t process_id;
    const size_t size = 4096;
    char expression[size];
    memset(expression, '\0', sizeof(expression));
    fgets(expression, size, stdin);

    for (size_t i = 0; i < size; ++i)
    {
        if (expression[i] == '\n')
        {
            expression[i] = '\0';
            break;
        }
    }

    /// create directory
    if ((process_id = fork()) == 0)
    {
        execlp("mkdir", "mkdir", "teyalite", NULL);
    }

    waitpid(process_id, NULL, 0);

    /// create expression.cpp
    if ((process_id = fork()) == 0)
    {

        int file_descriptor = open("./teyalite/expression.cpp", O_CREAT | O_WRONLY | O_APPEND, 0777);

        if (file_descriptor == -1)
        {
            printf("couldn't open or create a file\n");
            return 1;
        }

        dprintf(file_descriptor, program, expression);
        close(file_descriptor);
        return 0;
    }

    waitpid(process_id, NULL, 0);

    /// compile program
    if ((process_id = fork()) == 0)
    {
        execlp("g++", "g++", "./teyalite/expression.cpp", "-o", "./teyalite/expression", NULL);
    }

    waitpid(process_id, NULL, 0);

    /// execute python program
    if ((process_id = fork()) == 0)
    {
        execlp("./teyalite/expression", "./teyalite/expression", NULL);
    }

    waitpid(process_id, NULL, 0);

    /// delete directory
    if ((process_id = fork()) == 0)
    {
        execlp("rm", "rm", "-r", "./teyalite", NULL);
    }

    waitpid(process_id, NULL, 0);

    return 0;
}