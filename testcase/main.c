#include <stdio.h>

volatile int global = 40;
//int (*gpf) (const char *);
static int direct (int i) {
        return i + 1;
}

static int ignore (int i) {
        return i + 1;
}

int zet (int i) {
        return i + 2;
}

int main () {
        volatile int i = 2;
        int (*pf) (int);
        pf = zet;
        //gpf = puts;

        i = pf (global);
        i += direct (global);
        pf = ignore;
        i += pf (global);
        //gpf ("value: ");
        printf ("%d\n", i);

        return 0;
}

