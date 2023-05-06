// #include <cstdint>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

uint64_t wow = 234234324234;
uint64_t wow2;
uint64_t wow3;
char string_test[] = "asdfasdf";
uint64_t* uninit_array = NULL;
uint64_t init_array[10] = {1,2,3,4,5,6,7};


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
uint64_t testCastZExt(uint32_t a) {
    return (uint64_t)a;
}

uint64_t testCastSExt(int32_t a) {
    return (int64_t)a;
}

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
void updateWow3(uint64_t value) {
  wow3 = value;
}

// test ReturnInst
uint64_t testReturnInst(uint64_t value) {
  return value * 2;
}


int main() {
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
  return 0;
}
