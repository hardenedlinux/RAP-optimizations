///
#include <stdio.h>

volatile int vi = 0;

int internal () {
	return 42;
}

int (*volatile fp) () = internal;

int foo (double d) {
	vi += fp ();
	printf ("%d\n", vi);
	return fp () + (int)d;
}

int bar (double d) {
	vi += fp ();
	printf ("%d\n", vi);
	return fp () - (int)d;
}
