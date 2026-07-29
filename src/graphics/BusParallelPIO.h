#pragma once

#if defined(ARDUINO_ARCH_RP2040)

#include "PRUZEAmini.h"
#include "../third_party/LovyanGFX/src/LovyanGFX.hpp"
#include <hardware/dma.h>
#include <hardware/pio.h>
#include <stdint.h>

namespace PRUZEAmini {

class BusParallelPIO : public lgfx::IBus
{
private:
    static constexpr uint8_t DATA_PIN_COUNT = 8;
    static constexpr size_t REPEAT_BUFFER_SIZE = 256;

    uint8_t dataPinBase = 0;
    uint8_t wrPin = 0;
    uint8_t rdPin = 0;
    uint8_t dcPin = 0;
    uint32_t writeFrequency = 10000000;
    PIO pio = nullptr;
    int stateMachine = -1;
    int dmaChannel = -1;
    uint programOffset = 0;
    bool initialized = false;
    uint8_t repeatBuffer[REPEAT_BUFFER_SIZE]{};
    lgfx::FlipBuffer flipBuffer;

    void setDataMode(bool data);
    void startTransfer(const uint8_t* data, uint32_t length, bool dataMode);
    void writeBlocking(const uint8_t* data, uint32_t length, bool dataMode);

public:
    void configure(const GraphicsILI9341ParallelConfig& config);

    lgfx::bus_type_t busType() const override { return lgfx::bus_type_t::bus_parallel8; }
    uint32_t getClock() const override { return writeFrequency; }

    bool init() override;
    void release() override;
    void beginTransaction() override {}
    void endTransaction() override { wait(); }
    void wait() override;
    bool busy() const override;
    void initDMA() override {}
    void addDMAQueue(const uint8_t* data, uint32_t length) override { writeBytes(data, length, true, true); }
    void execDMAQueue() override { wait(); }
    uint8_t* getDMABuffer(uint32_t length) override { return flipBuffer.getBuffer(length); }
    bool reserveDMABuffer(uint32_t length) override { return flipBuffer.reserve(length); }
    void flush() override { wait(); }
    bool writeCommand(uint32_t data, uint_fast8_t bitLength) override;
    void writeData(uint32_t data, uint_fast8_t bitLength) override;
    void writeDataRepeat(uint32_t data, uint_fast8_t bitLength, uint32_t count) override;
    void writePixels(lgfx::pixelcopy_t* param, uint32_t length) override;
    void writeBytes(const uint8_t* data, uint32_t length, bool dc, bool useDma) override;
    void beginRead() override {}
    void endRead() override {}
    uint32_t readData(uint_fast8_t) override { return 0; }
    bool readBytes(uint8_t*, uint32_t, bool) override { return false; }
    void readPixels(void*, lgfx::pixelcopy_t*, uint32_t) override {}
};

} // namespace PRUZEAmini

#endif
