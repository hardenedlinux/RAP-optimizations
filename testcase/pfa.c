int putchar(int);
volatile int a, b;

int foo (double);
int bar (double);
int (*fp [2]) (double);

int main () {
        int c;
        a = 1, b = 2;
        c = a + b;

        fp[0] = foo;
        c += fp[0] (a);
        fp[1] = bar;
        c += fp[1] (b);
        putchar('0' + c);

        return 0;
}

