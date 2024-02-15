#include "MultiAddressPagerClient.h"


int16_t PagerClient::startReceive(uint32_t pin, uint32_t *addresses, uint8_t length) {
  // save the variables
  readBitPin = pin;
  rxAddresses = addresses;
  rxAddressesLen = length;

  // set the carrier frequency
  int16_t state = phyLayer->setFrequency(baseFreq);
  RADIOLIB_ASSERT(state);

  // set bitrate
  state = phyLayer->setBitRate(dataRate);
  RADIOLIB_ASSERT(state);

  // set frequency deviation to 4.5 khz
  state = phyLayer->setFrequencyDeviation((float)shiftFreqHz / 1000.0f);
  RADIOLIB_ASSERT(state);

  // now set up the direct mode reception
  Module* mod = phyLayer->getMod();
  mod->hal->pinMode(pin, mod->hal->GpioModeInput);

  // set direct sync word to the frame sync word
  // the logic here is inverted, because modules like SX1278
  // assume high frequency to be logic 1, which is opposite to POCSAG
  if(!inv) {
    phyLayer->setDirectSyncWord(~RADIOLIB_PAGER_FRAME_SYNC_CODE_WORD, 32);
  } else {
    phyLayer->setDirectSyncWord(RADIOLIB_PAGER_FRAME_SYNC_CODE_WORD, 32);
  }

  phyLayer->setDirectAction(PagerClientReadBit);
  phyLayer->receiveDirect();

  return(state);
}

int16_t PagerClient::readData(uint8_t* data, size_t* len) {
  // find the correct address
  bool match = false;
  uint8_t framePos = 0;
  uint8_t symbolLength = 0;
  while(!match && phyLayer->available()) {
    uint32_t cw = read();
    framePos++;

    // check if it's the idle code word
    if(cw == RADIOLIB_PAGER_IDLE_CODE_WORD) {
      continue;
    }

    // check if it's the sync word
    if(cw == RADIOLIB_PAGER_FRAME_SYNC_CODE_WORD) {
      framePos = 0;
      continue;
    }

    // not an idle code word, check if it's an address word
    if(cw & (RADIOLIB_PAGER_MESSAGE_CODE_WORD << (RADIOLIB_PAGER_CODE_WORD_LEN - 1))) {
      // this is pretty weird, it seems to be a message code word without address
      continue;
    }

    // should be an address code word, extract the address
    uint32_t addr_found = ((cw & RADIOLIB_PAGER_ADDRESS_BITS_MASK) >> (RADIOLIB_PAGER_ADDRESS_POS - 3)) | (framePos/2);
    for (int i = 0; i < rxAddressesLen; i++) {
      if (rxAddresses[i] == addr_found) {
        match = true;
        if (addr) {
          // output matched address
          *addr = addr_found;
        }

        // determine the encoding from the function bits
        if((cw & RADIOLIB_PAGER_FUNCTION_BITS_MASK) >> RADIOLIB_PAGER_FUNC_BITS_POS == RADIOLIB_PAGER_FUNC_BITS_NUMERIC) {
          symbolLength = 4;
        } else {
          symbolLength = 7;
        }
      }
    }
  }

  if(!match) {
    // address not found
    return(RADIOLIB_ERR_ADDRESS_NOT_FOUND);
  }

  // we have the address, start pulling out the message
  bool complete = false;
  size_t decodedBytes = 0;
  uint32_t prevCw = 0;
  bool overflow = false;
  int8_t ovfBits = 0;
  while(!complete && phyLayer->available()) {
    uint32_t cw = read();

    // check if it's the idle code word
    if(cw == RADIOLIB_PAGER_IDLE_CODE_WORD) {
      complete = true;
      break;
    }

    // skip the sync words
    if(cw == RADIOLIB_PAGER_FRAME_SYNC_CODE_WORD) {
      continue;
    }

    // check overflow from previous code word
    uint8_t bitPos = RADIOLIB_PAGER_CODE_WORD_LEN - 1 - symbolLength;
    if(overflow) {
      overflow = false;

      // this is a bit convoluted - first, build masks for both previous and current code word
      uint8_t currPos = RADIOLIB_PAGER_CODE_WORD_LEN - 1 - symbolLength + ovfBits;
      uint8_t prevPos = RADIOLIB_PAGER_MESSAGE_END_POS;
      uint32_t prevMask = (0x7FUL << prevPos) & ~((uint32_t)0x7FUL << (RADIOLIB_PAGER_MESSAGE_END_POS + ovfBits));
      uint32_t currMask = (0x7FUL << currPos) & ~((uint32_t)1 << (RADIOLIB_PAGER_CODE_WORD_LEN - 1));

      // next, get the two parts of the message symbol and stick them together
      uint8_t prevSymbol = (prevCw & prevMask) >> prevPos;
      uint8_t currSymbol = (cw & currMask) >> currPos;
      uint32_t symbol = prevSymbol << (symbolLength - ovfBits) | currSymbol;

      // finally, we can flip the bits
      symbol = Module::reflect((uint8_t)symbol, 8);
      symbol >>= (8 - symbolLength);

      // decode BCD and we're done
      if(symbolLength == 4) {
        symbol = decodeBCD(symbol);
      }
      data[decodedBytes++] = symbol;

      // adjust the bit position of the next message symbol
      bitPos += ovfBits;
      bitPos -= symbolLength;
    }

    // get the message symbols based on the encoding type
    while(bitPos >= RADIOLIB_PAGER_MESSAGE_END_POS) {
      // get the message symbol from the code word and reverse bits
      uint32_t symbol = (cw & (0x7FUL << bitPos)) >> bitPos;
      symbol = Module::reflect((uint8_t)symbol, 8);
      symbol >>= (8 - symbolLength);

      // decode BCD if needed
      if(symbolLength == 4) {
        symbol = decodeBCD(symbol);
      }
      data[decodedBytes++] = symbol;

      // now calculate if the next symbol is overflowing to the following code word
      int8_t remBits = bitPos - RADIOLIB_PAGER_MESSAGE_END_POS;
      if(remBits < symbolLength) {
        // overflow!
        prevCw = cw;
        overflow = true;
        ovfBits = remBits;
      }
      bitPos -= symbolLength;
    }

  }

  // save the number of decoded bytes
  *len = decodedBytes;
  return(RADIOLIB_ERR_NONE);
}
