#if defined(ARDUINO_ARCH_RP2040)

#include "BusParallelPIO.h"
#include "BusParallelPIO.pio.h"

#include <algorithm>
#include <hardware/clocks.h>
#include <pico/stdlib.h>

using namespace PRUZEAmini;

void BusParallelPIO::configure(const GraphicsILI9341ParallelConfig& config)
{
    dataPinBase = static_cast<uint8_t>(config.dataPinBase);
    wrPin = static_cast<uint8_t>(config.wrPin);
    rdPin = static_cast<uint8_t>(config.rdPin);
    dcPin = static_cast<uint8_t>(config.dcPin);
    writeFrequency = config.writeFreq;
}

bool BusParallelPIO::init()
{
    if (initialized) return true;

    pio = pio0;
    if (!pio_can_add_program(pio, &pruzea_parallel_program)) pio = pio1;
    if (!pio_can_add_program(pio, &pruzea_parallel_program)) return false;

    stateMachine = pio_claim_unused_sm(pio, false);
    if (stateMachine < 0)
    {
        pio = pio == pio0 ? pio1 : pio0;
        if (!pio_can_add_program(pio, &pruzea_parallel_program)) return false;
        stateMachine = pio_claim_unused_sm(pio, false);
        if (stateMachine < 0) return false;
    }

    dmaChannel = dma_claim_unused_channel(false);
    if (dmaChannel < 0)
    {
        pio_sm_unclaim(pio, static_cast<uint>(stateMachine));
        stateMachine = -1;
        return false;
    }

    programOffset = pio_add_program(pio, &pruzea_parallel_program);
    for (uint pin = dataPinBase; pin < dataPinBase + DATA_PIN_COUNT; ++pin) pio_gpio_init(pio, pin);
    pio_gpio_init(pio, wrPin);

    pio_sm_config config = pruzea_parallel_program_get_default_config(programOffset);
    sm_config_set_out_pins(&config, dataPinBase, DATA_PIN_COUNT);
    sm_config_set_sideset_pins(&config, wrPin);
    sm_config_set_out_shift(&config, true, true, 8);
    sm_config_set_fifo_join(&config, PIO_FIFO_JOIN_TX);
    const float clockDivider =
        static_cast<float>(clock_get_hz(clk_sys)) /
        static_cast<float>(writeFrequency * 2);
    sm_config_set_clkdiv(&config, clockDivider);

    pio_sm_set_consecutive_pindirs(
        pio,
        static_cast<uint>(stateMachine),
        dataPinBase,
        DATA_PIN_COUNT,
        true);
    pio_sm_set_consecutive_pindirs(
        pio,
        static_cast<uint>(stateMachine),
        wrPin,
        1,
        true);
    pio_sm_init(pio, static_cast<uint>(stateMachine), programOffset, &config);
    pio_sm_set_pins_with_mask(
        pio,
        static_cast<uint>(stateMachine),
        1u << wrPin,
        1u << wrPin);
    pio_sm_set_enabled(pio, static_cast<uint>(stateMachine), true);

    gpio_init(dcPin);
    gpio_set_dir(dcPin, GPIO_OUT);
    gpio_put(dcPin, 1);

    gpio_init(rdPin);
    gpio_set_dir(rdPin, GPIO_OUT);
    gpio_put(rdPin, 1);

    initialized = true;
    return true;
}

void BusParallelPIO::release()
{
    if (!initialized) return;

    wait();
    dma_channel_abort(static_cast<uint>(dmaChannel));
    dma_channel_unclaim(static_cast<uint>(dmaChannel));
    pio_sm_set_enabled(pio, static_cast<uint>(stateMachine), false);
    pio_sm_unclaim(pio, static_cast<uint>(stateMachine));
    pio_remove_program(pio, &pruzea_parallel_program, programOffset);

    dmaChannel = -1;
    stateMachine = -1;
    pio = nullptr;
    initialized = false;
}

void BusParallelPIO::wait()
{
    if (!initialized || dmaChannel < 0) return;

    dma_channel_wait_for_finish_blocking(static_cast<uint>(dmaChannel));
    while (!pio_sm_is_tx_fifo_empty(pio, static_cast<uint>(stateMachine))) tight_loop_contents();

    const uint32_t stallMask = 1u << (PIO_FDEBUG_TXSTALL_LSB + stateMachine);
    pio->fdebug = stallMask;
    while ((pio->fdebug & stallMask) == 0) tight_loop_contents();
    pio->fdebug = stallMask;
}

bool BusParallelPIO::busy() const
{
    if (!initialized || dmaChannel < 0) return false;
    return dma_channel_is_busy(static_cast<uint>(dmaChannel)) ||
        !pio_sm_is_tx_fifo_empty(pio, static_cast<uint>(stateMachine));
}

void BusParallelPIO::setDataMode(bool data)
{
    gpio_put(dcPin, data ? 1 : 0);
}

void BusParallelPIO::startTransfer(const uint8_t* data, uint32_t length, bool dataMode)
{
    if (!initialized || data == nullptr || length == 0) return;

    wait();
    setDataMode(dataMode);

    dma_channel_config config = dma_channel_get_default_config(static_cast<uint>(dmaChannel));
    channel_config_set_transfer_data_size(&config, DMA_SIZE_8);
    channel_config_set_read_increment(&config, true);
    channel_config_set_write_increment(&config, false);
    channel_config_set_dreq(&config, pio_get_dreq(pio, static_cast<uint>(stateMachine), true));
    dma_channel_configure(
        static_cast<uint>(dmaChannel),
        &config,
        &pio->txf[stateMachine],
        data,
        length,
        true);
}

void BusParallelPIO::writeBlocking(const uint8_t* data, uint32_t length, bool dataMode)
{
    if (!initialized || data == nullptr || length == 0) return;

    wait();
    setDataMode(dataMode);
    for (uint32_t i = 0; i < length; ++i)
    {
        pio_sm_put_blocking(pio, static_cast<uint>(stateMachine), data[i]);
    }
    wait();
}

bool BusParallelPIO::writeCommand(uint32_t data, uint_fast8_t bitLength)
{
    const uint32_t byteCount = bitLength >> 3;
    if (byteCount == 0 || byteCount > 4) return initialized;

    uint8_t bytes[4]{};
    for (uint32_t i = 0; i < byteCount; ++i) bytes[i] = static_cast<uint8_t>(data >> (i * 8));

    writeBlocking(bytes, byteCount, false);
    return initialized;
}

void BusParallelPIO::writeData(uint32_t data, uint_fast8_t bitLength)
{
    const uint32_t byteCount = bitLength >> 3;
    if (byteCount == 0 || byteCount > 4) return;

    uint8_t bytes[4]{};
    for (uint32_t i = 0; i < byteCount; ++i) bytes[i] = static_cast<uint8_t>(data >> (i * 8));

    writeBlocking(bytes, byteCount, true);
}

void BusParallelPIO::writeDataRepeat(uint32_t data, uint_fast8_t bitLength, uint32_t count)
{
    const uint32_t byteCount = bitLength >> 3;
    if (byteCount == 0 || byteCount > 4 || count == 0) return;

    const uint32_t repetitions = REPEAT_BUFFER_SIZE / byteCount;
    if (repetitions == 0) return;

    for (uint32_t i = 0; i < repetitions; ++i)
    {
        uint8_t* output = &repeatBuffer[i * byteCount];
        for (uint32_t byte = 0; byte < byteCount; ++byte)
        {
            output[byte] = static_cast<uint8_t>(data >> (byte * 8));
        }
    }

    while (count > 0)
    {
        const uint32_t chunk = std::min(count, repetitions);
        startTransfer(repeatBuffer, chunk * byteCount, true);
        wait();
        count -= chunk;
    }
}

void BusParallelPIO::writePixels(lgfx::pixelcopy_t* param, uint32_t length)
{
    if (param == nullptr || length == 0) return;

    const uint8_t destinationBytes = param->dst_bits >> 3;
    if (destinationBytes == 0) return;

    constexpr uint32_t BUFFER_PIXELS = 256;

    while (length > 0)
    {
        const uint32_t chunk = std::min(length, BUFFER_PIXELS);

        // flipBuffer is reused for every chunk.  Ensure the previous DMA
        // transfer has completed before fp_copy() overwrites the buffer.
        wait();

        uint8_t* buffer = flipBuffer.getBuffer(chunk * destinationBytes);
        if (buffer == nullptr) return;

        param->fp_copy(buffer, 0, chunk, param);
        startTransfer(buffer, chunk * destinationBytes, true);
        wait();

        length -= chunk;
    }
}

void BusParallelPIO::writeBytes(const uint8_t* data, uint32_t length, bool dc, bool useDma)
{
    if (useDma)
    {
        startTransfer(data, length, dc);
    }
    else
    {
        writeBlocking(data, length, dc);
    }
}


#endif
