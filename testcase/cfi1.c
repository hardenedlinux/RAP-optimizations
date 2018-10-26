#include <stdio.h>
#include <assert.h>

volatile int g1, g2;

int foo (double);
int bar (double);

int (*gp [2]) (double);

int main () {
	int l1, l2;
        int r1, r2;
        int (*lp [2]) (double);
        l1 = g1 = 1;
       	l2 = g2 = 2;
        r1 = r2 = 10;

        gp[0] = lp[0] = foo;
	
        r1 += gp[0] ((double)g1);
        r2 += lp[0] ((double)l1);
        gp[1] = lp[1] = bar;
        r1 += gp[1] ((double)g2);
        r2 += lp[1] ((double)l2);

        if (r1 == r2)
	  printf ("yes\n");
	else
	  assert (! "error");

        return 0;
}

