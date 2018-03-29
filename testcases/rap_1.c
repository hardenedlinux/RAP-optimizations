
int (*gfp) (int, int);

int f1(int i, int j) {
  return i + j;
}

int test() {
  int (*lfp) (int, int);

  lfp = f1();

  return lfp(1, 2);
}

