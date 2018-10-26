///
int internal () {
	return 42;
}

int (*volatile fp) () = internal;

int foo (double d) {
	return fp () + (int)d;
}

int bar (double d) {
	return fp () - (int)d;
}
