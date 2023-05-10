#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

int fib(int n) {
  int a = 0, b = 1;
  for (int i=0; i<n; ++i) {
    int next = a + b;
    a = b;
    b = next;
  }
  return b;
}

int ub1() {
    int c = 0;
    for (int i=2147483640; i > 0; ++i) {
      if (i % 2 == i % 3) {
        ++c;
      }
    }
    return c;
}

int ub2(int i) {
    int c = 0;
    for (; i > 0; ++i) {
      if (i % 2 == i % 3) {
        ++c;
      }
    }
    return c;
}

int oob(int x) {
  int a[] = {x,x*2,x+3,x/4};
  if (x == 5) {
    if (a[x] == 123) {
      printf("oob!\n");
    }
    return a[2];
  } else {
    return a[1];
  }
}

bool wtf(bool a, bool b) {
  return a && b;
}

int gg(int a, int b) {
  int c = a + 20;
  return a << b;
}

int main() {
  printf("hello world\n");
  for (int i=0; i<9; ++i) {
    printf("%d ", fib(i));
  }
  printf("%d\n", wtf(true, true));
  printf("shift test %d\n", gg(9999, 15));
  if (rand() % 2) {
    printf("ub1 %d\n", ub1());
  } else {
    printf("ub2 %d\n", ub2(2147483640));
  }
  return 0;
}
