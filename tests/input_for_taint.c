#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void test1() {
  char input[100];
  int index;
  printf("Hello\n");
  printf("Hello\n");
  printf("Hello\n");
  printf("Hello\n");
  scanf("%s", input);
  scanf("%d", &index);
  printf("%s", input);
  printf("%c", input[index]);
  printf("Hello\n");
}

void __sink0(void *ptr) {
  printf("Performing operation in __sink0 on memory at address: %p\n", ptr);

  char *data = (char *)ptr;
  printf("Data at memory address in __sink0: %s\n", data);
}

void __sink1(void *ptr, int a) {
  printf("Performing operation in __sink1 on memory at address: %p with "
         "integer %d\n",
         ptr, a);

  int *data = (int *)ptr;
  printf("Data at memory address in __sink1: %d\n", *data);
  printf("Modified data in __sink1: %d\n", *data + a);
}

void __sink2(void *ptr, int a, int b) {
  printf("Performing operation in __sink2 on memory at address: %p with "
         "integers %d and %d\n",
         ptr, a, b);

  int *data = (int *)ptr;
  printf("Data at memory address in __sink2: %d\n", *data);
  printf("Modified data in __sink2: %d\n", *data + a + b);
}

int main(int argc, char *argv[]) {
  // use get user input
  printf("Hello\n");
  char input[100];
  scanf("%s", input);
  printf("Hello\n");
  printf("Hello\n");

  char buffer[100];
  int index;

  // Taint source functions
  printf("Enter a string: ");
  fgets(buffer, sizeof(buffer), stdin);

  printf("Enter an integer: ");
  scanf("%d", &index);

  // Sink functions
  void *mem = malloc(256);
  strcpy(mem, buffer);
  __sink0(mem);

  void *mem_int = malloc(sizeof(int));
  memcpy(mem_int, &index, sizeof(int));
  __sink1(mem_int, 5);

  void *mem_int2 = malloc(sizeof(int));
  memcpy(mem_int2, &index, sizeof(int));
  __sink2(mem_int2, 3, 7);

  free(mem);
  free(mem_int);
  free(mem_int2);

  return EXIT_SUCCESS;
}
