#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <dirent.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <stdlib.h>
#define name_size 4096

const char *blank_separator = "   ";
const int protection_size = 11;
const char *const current_directory = ".";
const char *const parent_directory = "..";
const char *const left_down_corner = "\u2514"; /// └
const char *const right_t = "\u251c";          /// ├
const char *const vertical_line = "\u2502";    /// │
const char *const horizontal_line = "\u2500";  /// ─

struct Level
{
    int level;
    bool flag;
};

/// struct to count files and directories
struct Counter
{
    size_t file;
    size_t directory;
};

/// file or directory representation
struct File
{
    struct dirent *file;
    struct stat file_stat;
    char file_name[name_size];
};

typedef struct Level Level;
typedef struct Counter Counter;
typedef struct File File;

/// comparator to sort in lexicographic order
static int comparator(const void *a, const void *b)
{
    const File first = *(const File *)a;
    const File second = *(const File *)b;

    return strcmp(first.file_name, second.file_name);
}

/// print the number of directories and files
void print_bottom(const Counter counter)
{
    char *directory_text = counter.directory == 1 ? "directory" : "directories";
    char *file_text = counter.file == 1 ? "file" : "files";
    printf("\n%lu %s, %lu %s\n", counter.directory, directory_text, counter.file, file_text);
}

/// print the directory each time tree is running on a new directory provided as command line argument
void print_directory_name(const char *dir_name)
{
    printf("%s\n", dir_name);
}

/// function insert file type into the first index of protections
void file_protections_type(const mode_t file_mode, char *protections)
{

    char type = '?';

    if (S_ISREG(file_mode))
        type = '-';
    else if (S_ISDIR(file_mode))
        type = 'd';
    else if (S_ISLNK(file_mode))
        type = 'l';
    else if (S_ISFIFO(file_mode))
        type = 'p';
    else if (S_ISSOCK(file_mode))
        type = 's';
    else if (S_ISCHR(file_mode))
        type = 'c';
    if (S_ISBLK(file_mode))
        type = 'b';

    protections[0] = type;
}

/// function insert file protections permissions into 3 char.
void file_protections_permissions(const mode_t file_mode, const int mask[], const char symbol[], char *protections)
{
    /// read permission
    protections[0] = (mask[0] & file_mode) ? 'r' : '-';
    /// write permission
    protections[1] = (mask[1] & file_mode) ? 'w' : '-';

    ////
    const bool flag = (mask[2] & file_mode);
    /// execute permission for user, group or others
    const bool execute_flag = (mask[3] & file_mode);

    char flag_value = '-';

    if (flag && execute_flag)
        flag_value = symbol[0];
    else if (flag)
        flag_value = symbol[1];
    else if (execute_flag)
        flag_value = 'x';

    protections[2] = flag_value;
}

void file_protections(const struct stat *file_stat, char *line)
{
    char protections[protection_size];
    memset(protections, '\0', protection_size);
    const mode_t file_mode = file_stat->st_mode;
    file_protections_type(file_mode, protections);
    file_protections_permissions(file_mode, (int[]){S_IRUSR, S_IWUSR, S_ISUID, S_IXUSR}, (char[]){'s', 'S'}, protections + strlen(protections));
    file_protections_permissions(file_mode, (int[]){S_IRGRP, S_IWGRP, S_ISGID, S_IXGRP}, (char[]){'s', 'S'}, protections + strlen(protections));
    file_protections_permissions(file_mode, (int[]){S_IROTH, S_IWOTH, S_ISVTX, S_IXOTH}, (char[]){'t', 'T'}, protections + strlen(protections));
    sprintf(line, "%s", protections);
}

void file_owner(const struct stat *file_stat, char *line)
{
    const uid_t user_id = file_stat->st_uid;
    struct passwd *password_pointer = getpwuid(user_id);

    if (password_pointer == NULL)
        sprintf(line, " %i", (int)user_id);
    else
        sprintf(line, " %s", password_pointer->pw_name);
}

void file_group(const struct stat *file_stat, char *line)
{
    const uid_t grp_id = file_stat->st_gid;
    struct group *grp_pointer = getgrgid(grp_id);

    if (grp_pointer == NULL)
        sprintf(line, " %i", (int)grp_id);
    else
        sprintf(line, " %s", grp_pointer->gr_name);
}

/// check if the symbolic link points to directory or file then return the counter
Counter process_lnk(char *file_name, int file_descriptor)
{
    Counter counter;
    counter.file = 0;
    counter.directory = 0;
    struct stat file_stat;

    fstatat(file_descriptor, file_name, &file_stat, AT_SYMLINK_NOFOLLOW);

    if (S_ISLNK(file_stat.st_mode))
    {

        char pointer_file[name_size];
        memset(pointer_file, '\0', name_size);

        readlink(file_name, pointer_file, name_size);

        Counter counter_local = process_lnk(pointer_file, file_descriptor);
        counter.file += counter_local.file;
        counter.directory += counter_local.directory;
    }
    else if (S_ISDIR(file_stat.st_mode))
    {
        counter.directory += 1;
    }
    else
    {
        counter.file += 1;
    }

    return counter;
}

/// count number of file in the given directory
size_t count_files(const char *dir_name, bool all_files_flag)
{

    size_t count = 0;
    DIR *directory_descriptor = opendir(dir_name);

    if (directory_descriptor != NULL)
    {

        struct dirent *file;

        while ((file = readdir(directory_descriptor)) != NULL)
        {

            bool all_files_condition;

            if (all_files_flag)
                all_files_condition = (strcmp(file->d_name, current_directory) != 0 && strcmp(file->d_name, parent_directory) != 0);
            else
                all_files_condition = file->d_name[0] != '.';

            if (all_files_condition)
                ++count;
        }
    }

    closedir(directory_descriptor);
    return count;
}

Counter tree(const char *dir_name, Level level, bool all_files_flag, bool file_owner_flag, bool file_group_flag,
             bool file_size_flag, bool file_protections_flag, char *separator)
{

    Counter counter;
    counter.file = 0;
    counter.directory = 0;

    if (level.flag && level.level < 1)
        return counter;

    --level.level;

    DIR *directory_descriptor = opendir(dir_name);

    const size_t number_of_files = count_files(dir_name, all_files_flag);

    File files[number_of_files];

    if (directory_descriptor != NULL)
    {

        size_t current_file_counter = 0;

        File file;

        while ((file.file = readdir(directory_descriptor)) != NULL)
        {

            strcpy(file.file_name, dir_name);
            strcat(file.file_name, "/");
            strcat(file.file_name, file.file->d_name);

            bool all_files_condition;

            if (all_files_flag)
                all_files_condition = (strcmp(file.file->d_name, current_directory) != 0 &&
                                       strcmp(file.file->d_name, parent_directory) != 0);
            else
                all_files_condition = file.file->d_name[0] != '.';

            if (lstat(file.file_name, &file.file_stat) != -1 && all_files_condition)
            {
                files[current_file_counter] = file;
                ++current_file_counter;
            }
        }
    }
    else
    {
        printf("NULL\n");
    }

    qsort(files, number_of_files, sizeof(File), comparator);

    /// processing each file in the lexicographic order
    for (int index = 0; index < number_of_files; ++index)
    {

        char sub_separator[name_size];
        char line[name_size];

        memset(sub_separator, '\0', name_size);
        memset(line, '\0', name_size);

        strcat(sub_separator, separator);
        strcat(sub_separator, (index == number_of_files - 1) ? " " : vertical_line);
        strcat(sub_separator, blank_separator);

        sprintf(line, "%s%s", separator, (index == number_of_files - 1) ? left_down_corner : right_t);
        sprintf(line + strlen(line), "%s%s ", horizontal_line, horizontal_line);

        //// at least one of the flags to open bracket is true
        const bool opening_bracket = file_size_flag || file_owner_flag || file_group_flag || file_protections_flag;
        /// opening the bracket if opening is true.
        if (opening_bracket)
            sprintf(line + strlen(line), "%c", '[');

        /// adding permissions if required
        if (file_protections_flag)
            file_protections(&files[index].file_stat, line + strlen(line));

        /// adding user if required
        if (file_owner_flag)
            file_owner(&files[index].file_stat, line + strlen(line));

        /// adding group if required
        if (file_group_flag)
            file_group(&files[index].file_stat, line + strlen(line));

        /// add file size to line.
        if (file_size_flag)
            sprintf(line + strlen(line), " %ld", (size_t)files[index].file_stat.st_size);

        /// closing the bracket if it has been opened.
        if (opening_bracket)
            sprintf(line + strlen(line), "%c ", ']');

        if (S_ISDIR(files[index].file_stat.st_mode))
        {

            printf("%s %s\n", line, files[index].file->d_name);

            Counter counter_local = tree(files[index].file_name, level, all_files_flag, file_owner_flag, file_group_flag,
                                         file_size_flag, file_protections_flag, sub_separator);

            counter.file += counter_local.file;
            counter.directory += counter_local.directory + 1;
        }
        else if (S_ISLNK(files[index].file_stat.st_mode))
        {
            char pointer_file[name_size];
            memset(pointer_file, '\0', name_size);

            readlink(files[index].file_name, pointer_file, name_size);

            int file_descriptor = open(dir_name, O_RDONLY | O_DIRECTORY);

            printf("%s %s -> %s\n", line, files[index].file->d_name, pointer_file);

            Counter counter_local = process_lnk(pointer_file, file_descriptor);

            close(file_descriptor);

            counter.file += counter_local.file;
            counter.directory += counter_local.directory;
        }
        else
        {

            printf("%s %s\n", line, files[index].file->d_name);

            counter.file += 1;
        }
    }

    closedir(directory_descriptor);

    return counter;
}

int main(int argc, char *argv[])
{
    /// error flag
    // bool throw_error = false;
    Counter counter;
    counter.file = 0;
    counter.directory = 0;
    /// initialisation of level flag(-L <level>)
    Level level;
    level.level = 0;
    level.flag = false;
    /// initialisation of all file flag(-a)
    bool all_files_flag = false;
    /// initialisation of file owner or user id flag(-u)
    bool file_owner_flag = false;
    /// initialisation of file group owner or group id flag(-g)
    bool file_group_flag = false;
    /// initialisation of file size flag(-s)
    bool file_size_flag = false;
    /// initialisation of file protections flag(-p)
    bool file_protections_flag = false;

    int number_of_values = 0;
    int index = 1;

    while (index < argc)
    {
        // printf("index %d max %d\n", index, argc);
        if (argv[index][0] == '-')
        {
            /// it's a flag

            int level_depends = 0;

            for (int i = 1; i < strlen(argv[index]); ++i)
            {

                if (argv[index][i] == 'L')
                {
                    level.flag = true;
                    level.level = atoi(argv[index + 1]);
                    level_depends = 1;
                }
                else if (argv[index][i] == 'u')
                {
                    file_owner_flag = true;
                }
                else if (argv[index][i] == 's')
                {
                    file_size_flag = true;
                }
                else if (argv[index][i] == 'p')
                {
                    file_protections_flag = true;
                }
                else if (argv[index][i] == 'g')
                {
                    file_group_flag = true;
                }
                else if (argv[index][i] == 'a')
                {
                    all_files_flag = true;
                }
            }

            index += 1 + level_depends;
        }
        else
        {
            /// it's a value
            index += 1;
            number_of_values += 1;
        }
    }

    if (number_of_values == 0)
    {
        char *directory_name = ".";
        print_directory_name(directory_name);
        Counter counter = tree(directory_name, level, all_files_flag, file_owner_flag, file_group_flag,
                               file_size_flag, file_protections_flag, "");
        print_bottom(counter);
    }

    // char directories[number_of_values][name_size];

    index = 1;
    number_of_values = 0;

    while (index < argc)
    {
        if (argv[index][0] == '-')
        {
            /// it's a flag
            int level_depends = 0;
            for (int i = 1; i < strlen(argv[index]); ++i)
                if (argv[index][i] == 'L')
                    level_depends = 1;

            index += 1 + level_depends;
        }
        else
        {
            /// it's a value

            print_directory_name(argv[index]);
            Counter counter_local = tree(argv[index], level, all_files_flag, file_owner_flag, file_group_flag,
                                         file_size_flag, file_protections_flag, "");

            counter.file += counter_local.file;
            counter.directory += counter_local.directory;
            ++index;
        }
    }

    print_bottom(counter);

    return 0;
}