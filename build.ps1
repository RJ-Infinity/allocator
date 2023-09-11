Set-PSDebug -Trace 2
gcc .\RJAllocator.c -o allocerTest.exe -g -Wall -Wextra -pedantic -Wswitch-enum -Werror -std=c11
Set-PSDebug -Off