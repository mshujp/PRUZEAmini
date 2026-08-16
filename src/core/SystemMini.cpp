#include "../audio/AudioBase.h"
#include "../graphics/GraphicsBase.h"
#include "../input/InputBase.h"
#include "../storage/StorageBase.h"
#include "../util/Platform.h"
#include "../util/ScreenShot.h"

#include "../graphics/GraphicsILI9341.h"
#include "../graphics/GraphicsSSD1306.h"
#include "../graphics/GraphicsLGFXContext.h"
#include "../input/InputGpioButtons.h"
#include "../input/InputPS.h"
#include "../input/InputSnes.h"
#include "../input/InputStub.h"
#include "../input/InputTouch.h"
#include "../audio/AudioI2S.h"
#include "../audio/AudioPWM.h"
#include "../audio/AudioStub.h"
#include "../storage/StorageEEPROM.h"
#include "../storage/StorageSD.h"
#include "../storage/StorageStub.h"

#include <Arduino.h>
#include <atomic>

#if defined(ARDUINO_ARCH_RP2040)
#include <pico/multicore.h>
#elif defined(ARDUINO_ARCH_ESP32)
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#endif


namespace PRUZEAmini {

class SystemMini
{
public:
    SystemMini(GraphicsBase& graphics, InputBase& input, StorageBase& storage, AudioBase& audio, App& app);
    SystemMini(const SystemMini&) = delete;
    SystemMini& operator=(const SystemMini&) = delete;
    void setScreenShotContext(GraphicsILI9341* gi, StorageSD* sd);

    bool start();

private:
    GraphicsBase& graphics;
    InputBase& input;
    StorageBase& storage;
    AudioBase& audio;
    App& app;

    GraphicsILI9341* graphicsILI9341 = nullptr;
    StorageSD* storageSD = nullptr;

    std::atomic<bool> audioReady{false};
    std::atomic<bool> audioAvailable{false};
    uint8_t volumeSteps = 0;
    uint32_t lastFrameMsec = 0;
    bool requestFullRedraw = true;

    bool randomSeedInitialized = false;

    bool initialize();
    bool launchAudioWorker();
    void audioWorker();
    void runFrame();
    void waitFor30Fps();
    void updateSystem();
    void drawOSD();
    void drawVolume();
    void saveVolume(uint8_t volume);
    uint8_t loadVolume();

#if defined(ARDUINO_ARCH_RP2040)
    static void picoAudioEntry();
#elif defined(ARDUINO_ARCH_ESP32)
    static void espAudioEntry(void* context);
#endif
};

namespace {

constexpr const char* SYSTEM_ID = "_system";
constexpr const char* CONFIG_FILE = "config.ini";

#if defined(ARDUINO_ARCH_RP2040)
SystemMini* picoSystem = nullptr;
alignas(8) uint32_t audioWorkerStack[2048 / sizeof(uint32_t)];
#endif

} // namespace

SystemMini::SystemMini(GraphicsBase& _graphics, InputBase& _input, StorageBase& _storage, AudioBase& _audio, App& _app)
    : graphics(_graphics), input(_input), storage(_storage), audio(_audio), app(_app)
{
}

void SystemMini::setScreenShotContext(GraphicsILI9341* gi, StorageSD* sd) 
{
    graphicsILI9341 = gi;
    storageSD = sd;
}

void initRandom()
{
    uint32_t seed = micros();
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    randomSeed(seed);
}

bool SystemMini::initialize()
{
    if (!graphics.begin() || !input.begin()) return false;

    storage.begin();
    volumeSteps = audio.getVolumeSteps();
    audio.setVolumeLevel(loadVolume());
    app.init(storage);
    requestFullRedraw = true;
    initRandom();
    return true;
}

bool SystemMini::start()
{
    if (!initialize()) return false;

    if (audio.runsAsAudioWorker()) launchAudioWorker();
    lastFrameMsec = 0;
    for (;;)
    {
        runFrame();
        waitFor30Fps();
    }
    return true;
}

bool SystemMini::launchAudioWorker()
{
    audioReady.store(false);
    audioAvailable.store(false);

#if defined(ARDUINO_ARCH_RP2040)
    picoSystem = this;
    multicore_launch_core1_with_stack(&SystemMini::picoAudioEntry, audioWorkerStack, sizeof(audioWorkerStack));
#elif defined(ARDUINO_ARCH_ESP32)
    BaseType_t result;
#if CONFIG_FREERTOS_UNICORE
    result = xTaskCreate(&SystemMini::espAudioEntry, "pruzea_audio", 4096, this, 2, nullptr);
#else
    result = xTaskCreatePinnedToCore(&SystemMini::espAudioEntry, "pruzea_audio", 4096, this, 2, nullptr, 0);
#endif
    if (result != pdPASS) return false;
#else
    return false;
#endif

    const uint32_t startMsec = millis();
    while (!audioReady.load() && !Platform::elapsed(millis(), startMsec, 500)) delay(1);
    return audioAvailable.load();
}

void SystemMini::audioWorker()
{
#if defined(ARDUINO_ARCH_RP2040)
    Platform::initializeManualCoreFlashLockout();
#endif
    audioAvailable.store(audio.begin());
    audioReady.store(true);
    if (audioAvailable.load())
    {
        audio.playSE(&Audio::SE::NO_8, 1.0f);
        for (;;)
        {
            audio.update();
            delay(1);
        }
    }
}

#if defined(ARDUINO_ARCH_RP2040)
void SystemMini::picoAudioEntry()
{
    if (picoSystem) picoSystem->audioWorker();
    for (;;) delay(1000);
}
#elif defined(ARDUINO_ARCH_ESP32)
void SystemMini::espAudioEntry(void* context)
{
    static_cast<SystemMini*>(context)->audioWorker();
    vTaskDelete(nullptr);
}
#endif

void SystemMini::runFrame()
{
    const uint32_t now = millis();
    uint32_t deltaMsec = lastFrameMsec ? now - lastFrameMsec : 0;
    if (deltaMsec > 100) deltaMsec = 100;

    const float delta = float(deltaMsec) / 1000.0f;
    lastFrameMsec = now;
    input.update();
    if (!randomSeedInitialized && input.hasInput())
    {
        initRandom();
        randomSeedInitialized = true;
    }
    updateSystem();
    app.update(input, audio, storage, delta);

    const bool drew = app.draw(graphics, requestFullRedraw);
    if (drew)
    {
        graphics.suspendCamera();

        drawOSD();
        while(graphics.push())
        {
            app.draw(graphics, true);
            drawOSD();
        }
        requestFullRedraw = false;

        graphics.resumeCamera();
    }
}

void SystemMini::waitFor30Fps()
{
    static uint32_t next = 0;
    if (next == 0 || millis() - next > 100) next = millis();

    next += 33;
    while (static_cast<int32_t>(millis() - next) < 0) delay(1);
}

void SystemMini::updateSystem()
{
    if (input.justPressed(Input::MUTE))
    {
        audio.toggleMute();
        requestFullRedraw = true;
        return;
    }
    if (input.justPressed(Input::VOL_UP) && input.pressed(Input::VOL_DOWN))
    {
        if (graphicsILI9341 != nullptr && storageSD != nullptr)
        {
            ScreenShot ss;
            ss.save(*graphicsILI9341, *storageSD);
        }
    }

    if (input.justPressed(Input::VOL_UP) || input.justPressed(Input::VOL_DOWN))
    {
        const int8_t before = audio.getVolumeLevel();
        if (input.justPressed(Input::VOL_DOWN)) audio.downVolume();
        else audio.upVolume();

        if (before != audio.getVolumeLevel())
        {
            saveVolume(audio.getVolumeLevel());
            audio.playSE(&Audio::SE::NO_1, 1.0f);
            requestFullRedraw = true;
        }
    }
}

void SystemMini::drawVolume()
{
    if (audio.getVolumeSteps() == 0) return;

    const char* text = nullptr;
    const uint8_t volume = audio.getVolumeLevel();

    if (audio.isMuted()) text = "VOL: MUTE";
    else if (volumeSteps == 2) text = volume == 1 ? "VOL: ON" : "VOL: ?";
    else if (volumeSteps > 2)
    {
        switch (volume)
        {
        case 1:
            text = "VOL: - _ _";
            break;
        case 2:
            text = "VOL: - = _";
            break;
        case 3:
            text = "VOL: - = #";
            break;
        default:
            text = "VOL: ?";
            break;
        }
    }

    if (text)
    {
        const int16_t x = 13 + graphics.getViewportX();
        const int16_t y = 225 + graphics.getViewportY();
        graphics.drawString(text, x + 1, y, Graphics::BLACK, Graphics::SIZE_10);
        graphics.drawString(text, x, y + 1, Graphics::BLACK, Graphics::SIZE_10);
        graphics.drawString(text, x, y, Graphics::WHITE, Graphics::SIZE_10);
    }
}

void SystemMini::drawOSD()
{
    graphics.suspendClipRect();
    drawVolume();
    graphics.resumeClipRect();
}

void SystemMini::saveVolume(uint8_t volume)
{
    if (!storage.isAvailable()) return;

    SaveData data;
    data.load(storage, SYSTEM_ID, CONFIG_FILE);
    data.setUInt32("volume", volume);
    data.save(storage, SYSTEM_ID, CONFIG_FILE);
}

uint8_t SystemMini::loadVolume()
{
    if (!storage.isAvailable()) return 1;

    SaveData data;
    data.load(storage, SYSTEM_ID, CONFIG_FILE);
    return static_cast<uint8_t>(data.getUInt32("volume", 1));
}

void start(const GraphicsConfig& graphicsConfig, const InputConfig& inputConfig, const AudioConfig& audioConfig, const StorageConfig& storageConfig, App& app)
{
    GraphicsBase* graphics = nullptr;
    InputBase* inputDriver = nullptr;
    StorageBase* storageDriver = nullptr;
    AudioBase* audioDriver = nullptr;
    GraphicsILI9341* gi = nullptr;
    StorageSD* sd = nullptr;
    static GraphicsLGFXContext lgfxContext;

    lgfxContext.clearTouch();
    if (std::holds_alternative<InputTouchConfig<InputStubConfig>>(inputConfig))
    {
        lgfxContext.configureTouch(std::get<InputTouchConfig<InputStubConfig>>(inputConfig).touch);
    }
    else if (std::holds_alternative<InputTouchConfig<InputGpioButtonsConfig>>(inputConfig))
    {
        lgfxContext.configureTouch(std::get<InputTouchConfig<InputGpioButtonsConfig>>(inputConfig).touch);
    }
    else if (std::holds_alternative<InputTouchConfig<InputSnesConfig>>(inputConfig))
    {
        lgfxContext.configureTouch(std::get<InputTouchConfig<InputSnesConfig>>(inputConfig).touch);
    }
    else if (std::holds_alternative<InputTouchConfig<InputPSConfig>>(inputConfig))
    {
        lgfxContext.configureTouch(std::get<InputTouchConfig<InputPSConfig>>(inputConfig).touch);
    }

    if (std::holds_alternative<GraphicsILI9341Config>(graphicsConfig))
    {
        static GraphicsILI9341 instance(std::get<GraphicsILI9341Config>(graphicsConfig), lgfxContext);
        graphics = &instance;
        gi = &instance;
    }
    else if (std::holds_alternative<GraphicsILI9341ParallelConfig>(graphicsConfig))
    {
        static GraphicsILI9341 instance(std::get<GraphicsILI9341ParallelConfig>(graphicsConfig), lgfxContext);
        graphics = &instance;
        gi = &instance;
    }
    else if (std::holds_alternative<GraphicsSSD1306Config>(graphicsConfig))
    {
        static GraphicsSSD1306 instance(std::get<GraphicsSSD1306Config>(graphicsConfig));
        graphics = &instance;
    }

    if (std::holds_alternative<InputGpioButtonsConfig>(inputConfig))
    {
        static InputGpioButtons instance(std::get<InputGpioButtonsConfig>(inputConfig));
        inputDriver = &instance;
    }
    else if (std::holds_alternative<InputSnesConfig>(inputConfig))
    {
        static InputSnes instance(std::get<InputSnesConfig>(inputConfig));
        inputDriver = &instance;
    }
    else if (std::holds_alternative<InputPSConfig>(inputConfig))
    {
        static InputPS instance(std::get<InputPSConfig>(inputConfig));
        inputDriver = &instance;
    }
    else if (std::holds_alternative<InputTouchConfig<InputStubConfig>>(inputConfig))
    {
        static InputTouch<InputStub> instance(std::get<InputTouchConfig<InputStubConfig>>(inputConfig), lgfxContext);
        inputDriver = &instance;
    }
    else if (std::holds_alternative<InputTouchConfig<InputGpioButtonsConfig>>(inputConfig))
    {
        static InputTouch<InputGpioButtons> instance(std::get<InputTouchConfig<InputGpioButtonsConfig>>(inputConfig), lgfxContext);
        inputDriver = &instance;
    }
    else if (std::holds_alternative<InputTouchConfig<InputSnesConfig>>(inputConfig))
    {
        static InputTouch<InputSnes> instance(std::get<InputTouchConfig<InputSnesConfig>>(inputConfig), lgfxContext);
        inputDriver = &instance;
    }
    else if (std::holds_alternative<InputTouchConfig<InputPSConfig>>(inputConfig))
    {
        static InputTouch<InputPS> instance(std::get<InputTouchConfig<InputPSConfig>>(inputConfig), lgfxContext);
        inputDriver = &instance;
    }

    if (std::holds_alternative<StorageEEPROMConfig>(storageConfig))
    {
        static StorageEEPROM instance(std::get<StorageEEPROMConfig>(storageConfig), app.getId());
        storageDriver = &instance;
    }
    else if (std::holds_alternative<StorageSDConfig>(storageConfig))
    {
        static StorageSD instance(std::get<StorageSDConfig>(storageConfig));
        storageDriver = &instance;
        sd = &instance;
    }
    else if (std::holds_alternative<StorageStubConfig>(storageConfig))
    {
        static StorageStub instance;
        storageDriver = &instance;
    }

    if (std::holds_alternative<AudioPWMConfig>(audioConfig))
    {
        static AudioPWM instance(std::get<AudioPWMConfig>(audioConfig));
        audioDriver = &instance;
    }
    else if (std::holds_alternative<AudioI2SConfig>(audioConfig))
    {
        static AudioI2S instance(std::get<AudioI2SConfig>(audioConfig));
        audioDriver = &instance;
    }
    else if (std::holds_alternative<AudioStubConfig>(audioConfig))
    {
        static AudioStub instance;
        audioDriver = &instance;
    }

    if (!graphics || !inputDriver || !storageDriver || !audioDriver) return;

    static SystemMini systemMini(*graphics, *inputDriver, *storageDriver, *audioDriver, app);
    if (gi != nullptr && sd != nullptr)
    {
        systemMini.setScreenShotContext(gi, sd);
    }
    systemMini.start();
}

} // namespace PRUZEAmini
