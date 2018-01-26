#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

void IoOut8(uint16_t addr, uint8_t value);
void IoOut16(uint16_t addr, uint16_t value);
void IoOut32(uint16_t addr, uint32_t value);
uint8_t IoIn8(uint16_t addr);
uint16_t IoIn16(uint16_t addr);
uint32_t IoIn32(uint16_t addr);

/** @brief CompareExchange do "LOCK CMPXCHG".
 *
 * If *mem == expected, then *mem = value; return expected;
 * Otherwise return *mem;
 *
 * @param mem  Memory position to be tested.
 * @param expected  The value expected to be the same as *mem.
 * @param value  The value to be written to *mem.
 *
 * @return Value previously written at mem.
 */
uint64_t CompareExchange(uint64_t* mem, uint64_t expected, uint64_t value);

void LFencedWrite(uint64_t* mem, uint64_t value);

void AsmInthandler00();
void AsmInthandler01();
void AsmInthandler03();
void AsmInthandler04();
void AsmInthandler05();
void AsmInthandler06();
void AsmInthandler07();
void AsmInthandler08();
void AsmInthandler0a();
void AsmInthandler0b();
void AsmInthandler0c();
void AsmInthandler0d();
void AsmInthandler0e();
void AsmInthandler21();

#ifdef __cplusplus
}
#endif
