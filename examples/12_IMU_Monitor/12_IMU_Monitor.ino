/*
===============================================================================
 PLAMIOmini Example
 12_IMU_Monitor
===============================================================================

Sensor monitor and simple tilt estimation using an MPU6050.

Controls:
- START : Set the current pitch and roll as the zero reference
- A     : Switch between monitor and spirit-level displays
- B     : Reset recorded maximum values

Notes:
- This is not a precision attitude-estimation example.
- Pitch and roll use a simple complementary filter.
- Yaw is intentionally not estimated because gyro yaw drifts without an
  additional reference such as a magnetometer.

Required library:
- Adafruit MPU6050

Before compiling:
- Replace the required -1 values with pin numbers for your hardware.
- Set IMU_SDA_PIN and IMU_SCL_PIN for the MPU6050.
*/

#include <PLAMIOmini.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <math.h>

using namespace PLAMIOmini;

// =============================================================================
// Hardware configuration
// =============================================================================

GraphicsConfig graphicsConfig = GraphicsILI9341Config{
    .spiHost         = 0,
    .spiWriteFreq    = 60000000,
    .clkPin          = -1,
    .dataPin         = -1,
    .dcPin           = -1,
    .csPin           = -1,
    .resetPin        = -1,
    .backlightPin    = -1,
    .lcdRotate       = 0,
};

InputConfig inputConfig = InputGpioButtonsConfig{
    .buttonMapping = {
        .UP       = -1,
        .DOWN     = -1,
        .LEFT     = -1,
        .RIGHT    = -1,
        .A        = -1,
        .B        = -1,
        .START    = -1,
        .VOL_UP   = -1,
        .VOL_DOWN = -1,
        .MUTE     = -1,
    }
};

AudioConfig audioConfig = AudioStubConfig{};
StorageConfig storageConfig = StorageStubConfig{};

static constexpr int IMU_SDA_PIN = -1;
static constexpr int IMU_SCL_PIN = -1;
static constexpr uint8_t IMU_I2C_ADDRESS = 0x68;

// Set true to preview the sample without an IMU connected.
static constexpr bool IMU_DEMO_MODE = true;

// =============================================================================
// Application
// =============================================================================

class ImuMonitor : public App
{
public:
    const char* getId() const override { return "imu_monitor"; }

private:
    enum ViewMode : uint8_t
    {
        VIEW_MONITOR,
        VIEW_LEVEL
    };

    Adafruit_MPU6050 imu;
    bool imuAvailable = false;
    ViewMode viewMode = VIEW_MONITOR;

    float accX = 0.0f;
    float accY = 0.0f;
    float accZ = 0.0f;
    float gyroX = 0.0f;
    float gyroY = 0.0f;
    float gyroZ = 0.0f;

    float pitch = 0.0f;
    float roll = 0.0f;
    float zeroPitch = 0.0f;
    float zeroRoll = 0.0f;

    float maxAcc = 0.0f;
    float maxGyro = 0.0f;
    float maxTilt = 0.0f;
    bool filterInitialized = false;

    static float maximum(float a, float b)
    {
        return a > b ? a : b;
    }

    static float clamp(float value, float minimum, float maximumValue)
    {
        if (value < minimum) return minimum;
        if (value > maximumValue) return maximumValue;
        return value;
    }

    bool beginI2C()
    {
        if (IMU_SDA_PIN < 0 || IMU_SCL_PIN < 0)
        {
            return false;
        }

#if defined(ARDUINO_ARCH_ESP32)
        return Wire.begin(IMU_SDA_PIN, IMU_SCL_PIN);
#else
        Wire.setSDA(IMU_SDA_PIN);
        Wire.setSCL(IMU_SCL_PIN);
        Wire.begin();
        return true;
#endif
    }

    bool beginImu()
    {
        filterInitialized = false;

        if (!beginI2C() || !imu.begin(IMU_I2C_ADDRESS, &Wire))
        {
            return false;
        }

        imu.setAccelerometerRange(MPU6050_RANGE_8_G);
        imu.setGyroRange(MPU6050_RANGE_500_DEG);
        imu.setFilterBandwidth(MPU6050_BAND_21_HZ);
        return true;
    }

    void resetMaximums()
    {
        maxAcc = 0.0f;
        maxGyro = 0.0f;
        maxTilt = 0.0f;
    }

    void readSensor(float deltaSec)
    {
        sensors_event_t accelEvent;
        sensors_event_t gyroEvent;
        sensors_event_t temperatureEvent;
        imu.getEvent(&accelEvent, &gyroEvent, &temperatureEvent);

        accX = accelEvent.acceleration.x;
        accY = accelEvent.acceleration.y;
        accZ = accelEvent.acceleration.z;

        // Adafruit reports angular velocity in rad/s. Convert to deg/s.
        gyroX = gyroEvent.gyro.x * 57.2957795f;
        gyroY = gyroEvent.gyro.y * 57.2957795f;
        gyroZ = gyroEvent.gyro.z * 57.2957795f;

        const float accelPitch = atan2f(
            -accX,
            sqrtf(accY * accY + accZ * accZ)) * 57.2957795f;

        const float accelRoll = atan2f(accY, accZ) * 57.2957795f;

        if (!filterInitialized || deltaSec <= 0.0f)
        {
            pitch = accelPitch;
            roll = accelRoll;
            filterInitialized = true;
        }
        else
        {
            static constexpr float GYRO_WEIGHT = 0.96f;
            pitch = GYRO_WEIGHT * (pitch + gyroY * deltaSec)
                  + (1.0f - GYRO_WEIGHT) * accelPitch;
            roll = GYRO_WEIGHT * (roll + gyroX * deltaSec)
                 + (1.0f - GYRO_WEIGHT) * accelRoll;
        }

        // Maximum absolute acceleration on any axis. Gravity is included.
        maxAcc = maximum(maxAcc, maximum(fabsf(accX), maximum(fabsf(accY), fabsf(accZ))));
        maxGyro = maximum(maxGyro, maximum(fabsf(gyroX), maximum(fabsf(gyroY), fabsf(gyroZ))));
        maxTilt = maximum(maxTilt, maximum(fabsf(pitch - zeroPitch), fabsf(roll - zeroRoll)));
    }


    void readDemoSensor()
    {
        // Fixed values make screenshots repeatable.
        pitch = -12.4f;
        roll = 3.8f;

        accX = 3.4f;
        accY = -5.2f;
        accZ = 9.1f;

        gyroX = 1.2f;
        gyroY = -0.4f;
        gyroZ = 0.1f;

        filterInitialized = true;

        // Maximum absolute acceleration on any axis. Gravity is included.
        maxAcc = maximum(maxAcc, maximum(fabsf(accX), maximum(fabsf(accY), fabsf(accZ))));
        maxGyro = maximum(maxGyro, maximum(fabsf(gyroX), maximum(fabsf(gyroY), fabsf(gyroZ))));
        maxTilt = maximum(maxTilt, maximum(fabsf(pitch - zeroPitch), fabsf(roll - zeroRoll)));
    }

    void drawBar(
        Graphics& graphics,
        const char* label,
        float value,
        float range,
        int16_t y,
        Graphics::Color color)
    {
        char text[24];
        snprintf(text, sizeof(text), "%s %7.2f", label, value);
        graphics.drawString(text, 12, y, Graphics::WHITE, Graphics::SIZE_13);

        static constexpr int16_t BAR_X = 118;
        static constexpr int16_t BAR_W = 184;
        static constexpr int16_t BAR_H = 12;
        const int16_t center = BAR_X + BAR_W / 2;

        graphics.drawRect(BAR_X, y, BAR_W, BAR_H, Graphics::DARKGRAY);
        graphics.drawLine(center, y, center, y + BAR_H - 1, Graphics::LIGHTGRAY);

        const float normalized = clamp(value / range, -1.0f, 1.0f);
        const int16_t length = static_cast<int16_t>(normalized * (BAR_W / 2 - 2));

        if (length >= 0)
        {
            graphics.fillRect(center + 1, y + 2, length, BAR_H - 4, color);
        }
        else
        {
            graphics.fillRect(center + length, y + 2, -length, BAR_H - 4, color);
        }
    }

    void drawMonitor(Graphics& graphics)
    {
        char text[40];
        const float shownPitch = pitch - zeroPitch;
        const float shownRoll = roll - zeroRoll;

        snprintf(text, sizeof(text), "PITCH %7.1f deg", shownPitch);
        graphics.drawString(text, 12, 45, Graphics::CYAN, Graphics::SIZE_13);
        snprintf(text, sizeof(text), "ROLL  %7.1f deg", shownRoll);
        graphics.drawString(text, 174, 45, Graphics::CYAN, Graphics::SIZE_13);

        drawBar(graphics, "ACC X", accX, 19.62f, 76, Graphics::GREEN);
        drawBar(graphics, "ACC Y", accY, 19.62f, 96, Graphics::GREEN);
        drawBar(graphics, "ACC Z", accZ, 19.62f, 116, Graphics::GREEN);

        drawBar(graphics, "GYR X", gyroX, 250.0f, 146, Graphics::ORANGE);
        drawBar(graphics, "GYR Y", gyroY, 250.0f, 166, Graphics::ORANGE);
        drawBar(graphics, "GYR Z", gyroZ, 250.0f, 186, Graphics::ORANGE);

        snprintf(text, sizeof(text), "MAX AXIS ACC %.1f  GYRO %.1f  TILT %.1f",
                 maxAcc, maxGyro, maxTilt);
        graphics.drawString(text, 160, 211, Graphics::YELLOW, Graphics::SIZE_10,
                            Graphics::HorizontalAlign::CENTER,
                            Graphics::VerticalAlign::MIDDLE);
    }

    void drawLevel(Graphics& graphics)
    {
        const float shownPitch = pitch - zeroPitch;
        const float shownRoll = roll - zeroRoll;
        const int16_t cx = 160;
        const int16_t cy = 121;
        const int16_t radius = 72;

        graphics.drawCircle(cx, cy, radius, Graphics::LIGHTGRAY);
        graphics.drawCircle(cx, cy, radius / 2, Graphics::DARKGRAY);
        graphics.drawLine(cx - radius, cy, cx + radius, cy, Graphics::DARKGRAY);
        graphics.drawLine(cx, cy - radius, cx, cy + radius, Graphics::DARKGRAY);

        const int16_t bubbleX = cx + static_cast<int16_t>(clamp(shownRoll, -30.0f, 30.0f) / 30.0f * 60.0f);

        // Reverse the sign if the sensor is mounted in the opposite orientation.
        const int16_t bubbleY = cy + static_cast<int16_t>(clamp(shownPitch, -30.0f, 30.0f) / 30.0f * 60.0f);

        graphics.fillCircle(bubbleX, bubbleY, 10,
            (fabsf(shownPitch) < 2.0f && fabsf(shownRoll) < 2.0f)
                ? Graphics::GREEN : Graphics::YELLOW);
        graphics.drawCircle(bubbleX, bubbleY, 10, Graphics::WHITE);

        char text[32];
        snprintf(text, sizeof(text), "PITCH %6.1f", shownPitch);
        graphics.drawString(text, 12, 54, Graphics::CYAN, Graphics::SIZE_13);
        snprintf(text, sizeof(text), "ROLL %6.1f", shownRoll);
        graphics.drawString(text, 308, 54, Graphics::CYAN, Graphics::SIZE_13,
                            Graphics::HorizontalAlign::RIGHT,
                            Graphics::VerticalAlign::TOP);

        graphics.drawString("SIMPLE TILT LEVEL", 160, 204, Graphics::LIGHTGRAY,
                            Graphics::SIZE_13,
                            Graphics::HorizontalAlign::CENTER,
                            Graphics::VerticalAlign::MIDDLE);
    }

protected:
    void onInit(Storage& storage) override
    {
        (void)storage;
        if (IMU_DEMO_MODE)
        {
            imuAvailable = true;
            filterInitialized = true;
        }
        else
        {
            imuAvailable = beginImu();
        }

        resetMaximums();
        dirty = true;
    }

    void onUpdate(Input& input, Audio& audio, Storage& storage, float deltaSec) override
    {
        (void)audio;
        (void)storage;

        if (!imuAvailable)
        {
            if (!IMU_DEMO_MODE && input.justPressed(Input::A))
            {
                imuAvailable = beginImu();
                resetMaximums();
                dirty = true;
            }
            return;
        }

        if (IMU_DEMO_MODE)
        {
            readDemoSensor();
        }
        else
        {
            readSensor(deltaSec);
        }

        if (input.justPressed(Input::START))
        {
            zeroPitch = pitch;
            zeroRoll = roll;
            resetMaximums();
        }

        if (input.justPressed(Input::A))
        {
            viewMode = viewMode == VIEW_MONITOR ? VIEW_LEVEL : VIEW_MONITOR;
        }

        if (input.justPressed(Input::B))
        {
            resetMaximums();
        }

        dirty = true;
    }

    bool onDraw(Graphics& graphics, bool requestFullRedraw) override
    {
        if (!requestFullRedraw && !dirty)
        {
            return false;
        }

        graphics.fillScreen(Graphics::BLACK);
        graphics.fillRect(0, 0, 320, 34, Graphics::BLUE);
        graphics.drawString("IMU MONITOR", 12, 17, Graphics::WHITE,
                            Graphics::SIZE_22B,
                            Graphics::HorizontalAlign::LEFT,
                            Graphics::VerticalAlign::MIDDLE);

        graphics.drawString(IMU_DEMO_MODE ? "DEMO" : (viewMode == VIEW_MONITOR ? "MONITOR" : "LEVEL"),
                            308, 17, Graphics::YELLOW, Graphics::SIZE_13,
                            Graphics::HorizontalAlign::RIGHT,
                            Graphics::VerticalAlign::MIDDLE);

        if (!imuAvailable)
        {
            graphics.drawString("MPU6050 NOT FOUND", 160, 102, Graphics::RED,
                                Graphics::SIZE_25B,
                                Graphics::HorizontalAlign::CENTER,
                                Graphics::VerticalAlign::MIDDLE);
            graphics.drawString("CHECK SDA / SCL / ADDRESS", 160, 137,
                                Graphics::LIGHTGRAY, Graphics::SIZE_13,
                                Graphics::HorizontalAlign::CENTER,
                                Graphics::VerticalAlign::MIDDLE);
            graphics.drawString("A: RETRY", 160, 167, Graphics::YELLOW,
                                Graphics::SIZE_13,
                                Graphics::HorizontalAlign::CENTER,
                                Graphics::VerticalAlign::MIDDLE);
        }
        else if (viewMode == VIEW_MONITOR)
        {
            drawMonitor(graphics);
        }
        else
        {
            drawLevel(graphics);
        }

        graphics.drawString(
                            imuAvailable
                                ? "START: ZERO   A: VIEW   B: RESET MAX"
                                : "A: RETRY IMU INITIALIZATION",
                            160, 220, Graphics::WHITE, Graphics::SIZE_10,
                            Graphics::HorizontalAlign::CENTER,
                            Graphics::VerticalAlign::MIDDLE);

        dirty = false;
        return true;
    }

    void onTerminate(Storage& storage) override
    {
        (void)storage;
    }
};

ImuMonitor app;

void setup()
{
    PLAMIOmini::start(graphicsConfig, inputConfig, audioConfig, storageConfig, app);
}

void loop()
{
}
