Set-PSDebug -Trace 2
gcc .\test.c .\RJAllocator.c -o test.exe -g -Wall -Wextra -pedantic -Wswitch-enum -Werror -std=c11
Set-PSDebug -Off