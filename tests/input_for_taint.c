#include <stdbool.h>
#include <stdint.h>
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

void perform_sink_operations(void *mem, int index) {
  __sink0(mem);

  void *mem_int = malloc(sizeof(int));
  memcpy(mem_int, &index, sizeof(int));
  __sink1(mem_int, 5);

  void *mem_int2 = malloc(sizeof(int));
  memcpy(mem_int2, &index, sizeof(int));
  __sink2(mem_int2, 3, 7);

  free(mem_int);
  free(mem_int2);
}

void perform_sink_operations_extended(void *mem, int index, int x, int y) {
  __sink0(mem);

  void *mem_int = malloc(sizeof(int));
  memcpy(mem_int, &index, sizeof(int));
  __sink1(mem_int, x);

  void *mem_int2 = malloc(sizeof(int));
  memcpy(mem_int2, &index, sizeof(int));
  __sink2(mem_int2, x, y);

  free(mem_int);
  free(mem_int2);
}

void test_case_basic() {
  char buffer[100];
  int index;

  printf("Enter a string: ");
  fgets(buffer, sizeof(buffer), stdin);

  printf("Enter an integer: ");
  scanf("%d", &index);
  getchar(); // Consume newline character

  void *mem = malloc(256);
  strcpy(mem, buffer);
  perform_sink_operations(mem, index);

  free(mem);
  printf("\n");
}

void test_case_extended() {
  char buffer[100];
  int index, x, y;

  printf("Enter a string: ");
  fgets(buffer, sizeof(buffer), stdin);

  printf("Enter an integer: ");
  scanf("%d", &index);
  getchar(); // Consume newline character

  printf("Enter two more integers: ");
  scanf("%d %d", &x, &y);
  getchar(); // Consume newline character

  void *mem = malloc(256);
  strcpy(mem, buffer);
  perform_sink_operations_extended(mem, index, x, y);

  free(mem);
  printf("\n");
}

uint64_t wow = 234234324234;
uint64_t wow2;
uint64_t wow3;
char string_test[] = "asdfasdf";
uint64_t *uninit_array = NULL;
uint64_t init_array[10] = {1, 2, 3, 4, 5, 6, 7};

uint64_t add(uint64_t a, uint32_t b) { return a + b; }

uint64_t subtract(uint64_t a, uint32_t b) { return a - b; }

uint64_t multiply(uint32_t a, uint64_t b) { return a * b; }

uint64_t divide(uint64_t a, uint32_t b) { return a / b; }

uint64_t xor (uint64_t a, uint32_t b) { return a ^ b; }

    uint64_t calculate(uint64_t a, uint32_t b, uint32_t c, uint64_t d) {
  uint64_t sum = add(a, b);
  uint64_t difference = subtract(sum, c);
  uint64_t product = multiply(c, d);
  uint64_t quotient = divide(product, b);
  return xor(quotient, difference);
}
// test SelectInst
uint64_t selectInst(uint64_t a, uint64_t b, bool condition) {
  return condition ? a : b;
}

// test CastInst (Trunc)
uint64_t castInst(uint64_t a) {
  uint32_t truncated = (uint32_t)a;
  return (uint64_t)truncated;
}
uint64_t testCastZExt(uint32_t a) { return (uint64_t)a; }

uint64_t testCastSExt(int32_t a) { return (int64_t)a; }

// test PHINode
uint64_t testPHINode(uint64_t a, uint64_t b, uint64_t c) {
  uint64_t result;
  if (a < b) {
    result = a * c;
  } else {
    result = b * c;
  }
  return result;
}

// test CallInst
void updateWow3(uint64_t value) { wow3 = value; }

// test ReturnInst
uint64_t testReturnInst(uint64_t value) { return value * 2; }

void range() {

  uint64_t a = UINT64_MAX - 10;
  uint32_t b = 5;
  uint32_t c = 2;
  uint64_t d = UINT64_MAX / 2;
  uint64_t callInstValue = 10;

  uint64_t result = calculate(a, b, c, d);
  printf("Result: %jd\n", result);
  uint64_t selectInstResult = selectInst(a, b, true);
  printf("selectInst Result: %jd\n", selectInstResult);
  uint64_t castTruncResult = castInst(a);
  printf("Truncate castInst Result: %jd\n", castTruncResult);
  uint64_t castZExtResult = testCastZExt(b);
  printf("ZExt castInst Result: %jd\n", castZExtResult);
  uint64_t castSExtResult = testCastSExt(b);
  printf("SExt castInst Result: %jd\n", castSExtResult);
  uint64_t phiNodeResult = testPHINode(a, b, c);
  printf("PHINode Result: %jd\n", phiNodeResult);
  updateWow3(callInstValue);
  printf("CallInst - wow3 updated to: %jd\n", wow3);

  // Test StoreInst
  wow2 = wow * 2;
  printf("StoreInst - wow2 assigned: %jd\n", wow2);

  // Test ReturnInst
  uint64_t returnInstValue = testReturnInst(callInstValue);
  printf("ReturnInst - returned value: %jd\n", returnInstValue);
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

  int basic_test_cases, extended_test_cases;

  printf("Enter the number of basic test cases: ");
  scanf("%d", &basic_test_cases);
  getchar(); // Consume newline character

  for (int i = 0; i < basic_test_cases; i++) {
    printf("Basic Test case %d\n", i + 1);
    test_case_basic();
  }

  printf("Enter the number of extended test cases: ");
  scanf("%d", &extended_test_cases);
  getchar(); // Consume newline character

  for (int i = 0; i < extended_test_cases; i++) {
    printf("Extended Test case %d\n", i + 1);
    test_case_extended();
  }

  range();

  return EXIT_SUCCESS;
}
