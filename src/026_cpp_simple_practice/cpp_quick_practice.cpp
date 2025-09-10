#include <iostream>
#include <stdio.h>

using namespace std;





int main(int argc, const char **argv)
{
    int a = 1, b = 3, c = 2;

    while (a<b<c) {
        ++a;
        --b;
        --c;
    }
    
    printf("a=%d, b=%d, c=%d\n", a, b, c);
    return 0;
}
