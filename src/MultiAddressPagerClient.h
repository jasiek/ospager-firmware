#include <RadioLib.h>

class MultiAddressPagerClient : public PagerClient {
 public:
  int16_t startReceive(uint32_t pin, uint32_t *addresses, uint8_t length);
  int16_t readData(uint8_t* data, size_t* len, uint32_t* addr = NULL);

 private:
  uint32_t *rxAddresses;
  uint8_t rxAddressesLen;
};
