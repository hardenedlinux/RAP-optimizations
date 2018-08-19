#include <stdio.h>

volatile int global = 40;
int (*gpf) (const char *);

static int ignore (int i, double d) {
        return i - d;
}

int zet (int i, double d) {
        return i + d;
}

int main () {
        volatile int i = 2;
        int (*pf) (int, double);
        pf = zet;
        gpf = puts;

        i = pf (i, global);
        i += ignore (i, global);
        pf = ignore;
        i += pf (i, global);
        gpf ("value: ");
        printf ("%d\n", i);

        return i + 2;
}

