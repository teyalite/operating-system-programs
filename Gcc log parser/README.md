# Task
You have to write a program in the `main.c` file, which receives a name of a `.c` source code file and
outputs the amount **unique rows (lines)**, containing warnings and errors in this file. You should use the `gcc`
compiler. Your program should **try** to compile the provided file and parse the output of the `gcc` compiler
to calculate the needed information. Your program should **not** create any additional files. That is, if
the `gcc` compiles the code, you should delete the compiled file, so it won't appear in the folder!
You are not allowed to modify any files, other than the `main.c`.

##### Example:
File `main.c`:
```c
#include <stdio.h>

int main () {
    printf("%s %s", 123, 44); /// Two warnings on this line
    /// No warnings and errors here
    int a b c d; /// Multiple errors on this line
    int; /// One warning here
}
```
If you try to compile this file with the `gcc`, you'll see the compilation log (provided through the `stderr` stream):
```
main.c:2:5: warning: implicit declaration of function ‘printf’ [-Wimplicit-function-declaration]
    2 |     printf("%s %s", 123, 44); /// Two warnings on this line
      |     ^~~~~~
main.c:2:5: warning: incompatible implicit declaration of built-in function ‘printf’
main.c:1:1: note: include ‘’ or provide a declaration of ‘printf’
  +++ |+#include <stdio.h>
    1 | int main () {
main.c:2:14: warning: format ‘%s’ expects argument of type ‘char *’, but argument 2 has type ‘int’ [-Wformat=]
    2 |     printf("%s %s", 123, 44); /// Two warnings on this line
      |             ~^      ~~~
      |              |      |
      |              char * int
      |             %d
main.c:2:17: warning: format ‘%s’ expects argument of type ‘char *’, but argument 3 has type ‘int’ [-Wformat=]
    2 |     printf("%s %s", 123, 44); /// Two warnings on this line
      |                ~^        ~~
      |                 |        |
      |                 char *   int
      |                %d
main.c:4:11: error: expected ‘=’, ‘,’, ‘;’, ‘asm’ or ‘__attribute__’ before ‘b’
    4 |     int a b c d; ///
      |           ^
main.c:4:11: error: unknown type name ‘b’
main.c:4:15: error: expected ‘=’, ‘,’, ‘;’, ‘asm’ or ‘__attribute__’ before ‘d’
    4 |     int a b c d; ///
      |               ^
main.c:5:5: warning: useless type name in empty declaration
    5 |     int; /// One warning here
      |     ^~~
```
