#include <stdint.h>
#include <stdio.h>

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

int main() {
  uint64_t a = UINT64_MAX - 10;
  uint32_t b = 5;
  uint32_t c = 2;
  uint64_t d = UINT64_MAX / 2;

  uint64_t result = calculate(a, b, c, d);
  printf("Result: %llu\n", result);

  return 0;
}
