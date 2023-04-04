//=============================================================================
// FILE:
//      input_for_hello.c
//
// DESCRIPTION:
//      Sample input file for HelloWorld and InjectFuncCall
//
// License: MIT
//=============================================================================
int foo(int a) {
  return a * 2;
}

int bar(int a, int b) {
  return (a + foo(b) * 2);
}

int baz(unsigned int a, unsigned int b) {
  unsigned int c = a / b - a % b;
  signed int aa = a, bb = b;
  int cc = a / b - a % b;
  return c + cc;
}

int fez(int a, int b, int c) {
  return (a + bar(a, b) * 2 + c * 3);
}

int test(int a, int b, int c) {
	return (a + b * 2 + c * 3);
}

int main(int argc, char *argv[]) {
  int a = 123;
  int ret = 0;

  ret += foo(a);
  ret += bar(a, ret);
  ret += baz(123, 4);
  ret += fez(a, ret, 123);
	ret += test(a, ret, 123);

  return ret;
}
