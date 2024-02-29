#include <Arduino.h>
#include <RadioLib.h>

// from src/protocols/Pager.cpp
extern PhysicalLayer* readBitInstance;
extern uint32_t readBitPin;
extern void PagerClientReadBit(void);

class MultiAddressPagerClient : public PagerClient {
public:
  MultiAddressPagerClient(PhysicalLayer* phy) : PagerClient(phy) {}
  int16_t startReceive(uint32_t pin, uint32_t *addresses, uint8_t length);
  int16_t readData(uint8_t *data, size_t *len, uint32_t *addr);

private:
  uint32_t *rxAddresses;
  uint8_t rxAddressesLen;
};
