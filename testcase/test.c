#include <stdio.h>

int var = 1;
int *pi = &var;

int main () {
        int var = 42;
        printf ("%d\n", *pi);
        
        pi = &var;
        printf ("%d\n", *pi);
        
        if (var)
        {
                int var = 100;
                pi = &var;
                printf ("%d\n", *pi);
        }
}

