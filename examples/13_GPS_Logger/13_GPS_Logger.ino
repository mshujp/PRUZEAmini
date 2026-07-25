/*
===============================================================================
 PLAMIOmini Example
 13_GPS_Logger
===============================================================================

GPS monitor and CSV logger using TinyGPSPlus.

Controls:
- START : Start / stop recording
- A     : Save a waypoint marker
- B     : Switch display screen

CSV log format:
  time_msec,latitude,longitude,altitude_m,speed_kmh,satellites

Waypoint markers are saved separately to gps_markers.csv.

Required library:
- TinyGPSPlus

Before compiling:
- Replace the required -1 values with pin numbers for your hardware.
- Connect GPS module TX to GPS_RX_PIN.
- GPS_TX_PIN is optional and may remain -1.

Existing log files are replaced when the example starts.
*/

#include <PLAMIOmini.h>
#include <TinyGPSPlus.h>
#include <math.h>

using namespace PLAMIOmini;

// =============================================================================
// Hardware configuration
// =============================================================================

GraphicsConfig graphicsConfig = GraphicsILI9341Config{
    .spiHost         = 1,
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
    .gpioButtonPins = {
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

StorageConfig storageConfig = StorageSDConfig{
    .spiHost = 0,
    .misoPin  = -1,
    .sckPin   = -1,
    .mosiPin  = -1,
    .csPin    = -1,
    .baudRate = 12000000,
};

static constexpr int GPS_RX_PIN = -1;
static constexpr int GPS_TX_PIN = -1;
static constexpr uint32_t GPS_BAUD_RATE = 9600;

// Set true to preview the sample without a GPS module connected.
static constexpr bool GPS_DEMO_MODE = true;
static constexpr double DEMO_BASE_LATITUDE = 33.589800;
static constexpr double DEMO_BASE_LONGITUDE = 130.420700;

// =============================================================================
// Application
// =============================================================================

class GpsLogger : public App
{
public:
    const char* getId() const override { return APP_ID; }

private:
    static constexpr const char* APP_ID = "gps_logger";
    static constexpr const char* LOG_FILE = "gps_log.csv";
    static constexpr const char* MARKER_FILE = "gps_markers.csv";
    static constexpr const char* CSV_HEADER =
        "time_msec,latitude,longitude,altitude_m,speed_kmh,satellites";
    static constexpr const char* MARKER_HEADER =
        "marker,time_msec,latitude,longitude,altitude_m,speed_kmh,satellites";
    static constexpr uint32_t RECORD_INTERVAL_MSEC = 1000;
    static constexpr uint32_t FIX_MAX_AGE_MSEC = 3000;

    enum Screen : uint8_t
    {
        SCREEN_POSITION,
        SCREEN_STATUS
    };

    TinyGPSPlus gps;
    Screen screen = SCREEN_POSITION;

    bool gpsSerialAvailable = false;
    bool storageAvailable = false;
    bool logReady = false;
    bool markerFileReady = false;
    bool recording = false;

    uint32_t logCount = 0;
    uint32_t markerCount = 0;
    uint32_t lastRecordMsec = 0;
    uint32_t receivedCharacters = 0;

    double demoLatitude = DEMO_BASE_LATITUDE;
    double demoLongitude = DEMO_BASE_LONGITUDE;
    double demoAltitude = 8.4;
    double demoSpeedKmh = 4.2;
    unsigned long demoSatellites = 9;

    bool hasPreviousDistancePoint = false;
    double previousLatitude = 0.0;
    double previousLongitude = 0.0;
    double distanceMeters = 0.0;

    const char* statusText = "WAITING FOR GPS";
    Graphics::Color statusColor = Graphics::YELLOW;

    double currentLatitude()
    {
        return GPS_DEMO_MODE ? demoLatitude : gps.location.lat();
    }

    double currentLongitude()
    {
        return GPS_DEMO_MODE ? demoLongitude : gps.location.lng();
    }

    double currentAltitude()
    {
        return GPS_DEMO_MODE ? demoAltitude
                             : (gps.altitude.isValid() ? gps.altitude.meters() : 0.0);
    }

    double currentSpeedKmh()
    {
        return GPS_DEMO_MODE ? demoSpeedKmh
                             : (gps.speed.isValid() ? gps.speed.kmph() : 0.0);
    }

    unsigned long currentSatellites()
    {
        return GPS_DEMO_MODE ? demoSatellites
                             : (gps.satellites.isValid() ? gps.satellites.value() : 0ul);
    }

    void updateDemoPosition()
    {
        const float timeSec = Platform::getMsec() * 0.001f;
        // A small walking loop around Hakata Station for display and logging tests.
        demoLatitude = DEMO_BASE_LATITUDE + sinf(timeSec * 0.08f) * 0.000060;
        demoLongitude = DEMO_BASE_LONGITUDE + cosf(timeSec * 0.08f) * 0.000075;
        demoAltitude = 8.4 + sinf(timeSec * 0.12f) * 0.6;
        demoSpeedKmh = 4.2 + sinf(timeSec * 0.18f) * 0.7;
        receivedCharacters += 6;
    }

    bool beginGpsSerial()
    {
        if (GPS_DEMO_MODE)
        {
            return true;
        }

        if (GPS_RX_PIN < 0)
        {
            return false;
        }

#if defined(ARDUINO_ARCH_ESP32)
        Serial1.begin(GPS_BAUD_RATE, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
#else
        Serial1.setRX(GPS_RX_PIN);
        if (GPS_TX_PIN >= 0)
        {
            Serial1.setTX(GPS_TX_PIN);
        }
        Serial1.begin(GPS_BAUD_RATE);
#endif
        return true;
    }

    void receiveGps()
    {
        if (GPS_DEMO_MODE)
        {
            updateDemoPosition();
            return;
        }

        if (!gpsSerialAvailable)
        {
            return;
        }

        while (Serial1.available() > 0)
        {
            gps.encode(static_cast<char>(Serial1.read()));
            ++receivedCharacters;
        }
    }

    bool hasFix() const
    {
        return GPS_DEMO_MODE || (gps.location.isValid() && gps.location.age() < FIX_MAX_AGE_MSEC);
    }

    const char* getFixText() const
    {
        if (GPS_DEMO_MODE)
        {
            return "DEMO FIX";
        }
        if (!gpsSerialAvailable)
        {
            return "NO SERIAL";
        }
        if (receivedCharacters == 0)
        {
            return "NO DATA";
        }
        if (!gps.location.isValid())
        {
            return "SEARCHING";
        }
        if (gps.location.age() >= FIX_MAX_AGE_MSEC)
        {
            return "STALE";
        }
        return "FIX";
    }

    Graphics::Color getFixColor() const
    {
        return hasFix() ? Graphics::GREEN : Graphics::YELLOW;
    }

    bool createLogFiles(Storage& storage)
    {
        recording = false;
        logCount = 0;
        markerCount = 0;
        distanceMeters = 0.0;
        hasPreviousDistancePoint = false;

        if (!storageAvailable)
        {
            logReady = false;
            markerFileReady = false;
            statusText = "STORAGE UNAVAILABLE";
            statusColor = Graphics::RED;
            return false;
        }

        logReady = storage.writeUserFile(APP_ID, LOG_FILE, CSV_HEADER, false);
        markerFileReady = storage.writeUserFile(APP_ID, MARKER_FILE, MARKER_HEADER, false);

        if (logReady && markerFileReady)
        {
            statusText = "LOG FILES READY";
            statusColor = Graphics::CYAN;
            return true;
        }

        statusText = "LOG CREATE FAILED";
        statusColor = Graphics::RED;
        return false;
    }

    bool formatPositionLine(char* line, size_t lineSize, const char* prefix = nullptr)
    {
        if (!hasFix())
        {
            return false;
        }

        const uint32_t now = Platform::getMsec();
        const double altitude = currentAltitude();
        const double speed = currentSpeedKmh();
        const unsigned long satellites = currentSatellites();

        int length;
        if (prefix != nullptr)
        {
            length = snprintf(
                line, lineSize,
                "%s,%lu,%.6f,%.6f,%.1f,%.1f,%lu",
                prefix,
                static_cast<unsigned long>(now),
                currentLatitude(),
                currentLongitude(),
                altitude,
                speed,
                satellites);
        }
        else
        {
            length = snprintf(
                line, lineSize,
                "%lu,%.6f,%.6f,%.1f,%.1f,%lu",
                static_cast<unsigned long>(now),
                currentLatitude(),
                currentLongitude(),
                altitude,
                speed,
                satellites);
        }

        return length > 0 && static_cast<size_t>(length) < lineSize;
    }

    bool appendCurrentPosition(Storage& storage)
    {
        char line[160];
        if (!formatPositionLine(line, sizeof(line)))
        {
            return false;
        }
        return storage.writeUserFile(APP_ID, LOG_FILE, line, true);
    }

    bool appendMarker(Storage& storage)
    {
        if (!markerFileReady || !hasFix())
        {
            return false;
        }

        char markerName[16];
        snprintf(markerName, sizeof(markerName), "M%03lu",
                 static_cast<unsigned long>(markerCount + 1));

        char line[176];
        if (!formatPositionLine(line, sizeof(line), markerName))
        {
            return false;
        }

        if (!storage.writeUserFile(APP_ID, MARKER_FILE, line, true))
        {
            return false;
        }

        ++markerCount;
        return true;
    }

    void addDistancePoint()
    {
        const double latitude = currentLatitude();
        const double longitude = currentLongitude();

        if (hasPreviousDistancePoint)
        {
            distanceMeters += TinyGPSPlus::distanceBetween(
                previousLatitude,
                previousLongitude,
                latitude,
                longitude);
        }

        previousLatitude = latitude;
        previousLongitude = longitude;
        hasPreviousDistancePoint = true;
    }

    void updateRecording(Storage& storage)
    {
        if (!recording || !logReady || !hasFix())
        {
            return;
        }

        const uint32_t now = Platform::getMsec();
        if (!Platform::elapsed(now, lastRecordMsec, RECORD_INTERVAL_MSEC))
        {
            return;
        }

        lastRecordMsec = now;
        if (appendCurrentPosition(storage))
        {
            addDistancePoint();
            ++logCount;
            statusText = "LOG APPENDED";
            statusColor = Graphics::GREEN;
        }
        else
        {
            recording = false;
            statusText = "APPEND FAILED";
            statusColor = Graphics::RED;
        }
    }

    void drawValue(Graphics& graphics, const char* label, const char* value,
                   int16_t y, Graphics::Color color = Graphics::WHITE)
    {
        graphics.drawString(label, 12, y, Graphics::LIGHTGRAY, Graphics::SIZE_13);
        graphics.drawString(value, 308, y, color, Graphics::SIZE_18,
                            Graphics::HorizontalAlign::RIGHT,
                            Graphics::VerticalAlign::TOP);
    }

    void drawPositionScreen(Graphics& graphics)
    {
        char text[64];

        if (hasFix())
        {
            snprintf(text, sizeof(text), "%.6f", currentLatitude());
            drawValue(graphics, "LATITUDE", text, 45, Graphics::CYAN);

            snprintf(text, sizeof(text), "%.6f", currentLongitude());
            drawValue(graphics, "LONGITUDE", text, 71, Graphics::CYAN);

            snprintf(text, sizeof(text), "%lu",
                     currentSatellites());
            drawValue(graphics, "SATELLITES", text, 97,
                      currentSatellites() >= 4
                          ? Graphics::GREEN : Graphics::YELLOW);

            snprintf(text, sizeof(text), "%.1f m",
                     currentAltitude());
            drawValue(graphics, "ALTITUDE", text, 123);

            snprintf(text, sizeof(text), "%.1f km/h",
                     currentSpeedKmh());
            drawValue(graphics, "SPEED", text, 149);

            if (distanceMeters < 1000.0)
            {
                snprintf(text, sizeof(text), "%.1f m", distanceMeters);
            }
            else
            {
                snprintf(text, sizeof(text), "%.3f km", distanceMeters / 1000.0);
            }
            drawValue(graphics, "DISTANCE", text, 175, Graphics::GREEN);
        }
        else
        {
            graphics.drawString("SEARCHING FOR GPS FIX", 160, 92,
                                Graphics::YELLOW, Graphics::SIZE_22B,
                                Graphics::HorizontalAlign::CENTER,
                                Graphics::VerticalAlign::MIDDLE);
            snprintf(text, sizeof(text), "RX: %lu chars",
                     static_cast<unsigned long>(receivedCharacters));
            graphics.drawString(text, 160, 125, Graphics::LIGHTGRAY,
                                Graphics::SIZE_13,
                                Graphics::HorizontalAlign::CENTER,
                                Graphics::VerticalAlign::MIDDLE);
        }
    }

    void drawStatusScreen(Graphics& graphics)
    {
        char text[64];

        drawValue(graphics, "GPS FIX", getFixText(), 50, getFixColor());
        drawValue(graphics, "RECORDING", recording ? "RECORDING" : "STOPPED", 80,
                  recording ? Graphics::GREEN : Graphics::YELLOW);

        snprintf(text, sizeof(text), "%lu", static_cast<unsigned long>(logCount));
        drawValue(graphics, "LOG COUNT", text, 110);

        snprintf(text, sizeof(text), "%lu", static_cast<unsigned long>(markerCount));
        drawValue(graphics, "MARKERS", text, 140);

        snprintf(text, sizeof(text), "%lu", static_cast<unsigned long>(receivedCharacters));
        drawValue(graphics, "RX CHARACTERS", text, 170);
    }

protected:
    void onInit(Storage& storage) override
    {
        storageAvailable = storage.isAvailable();
        gpsSerialAvailable = beginGpsSerial();
        receivedCharacters = 0;
        lastRecordMsec = Platform::getMsec();

        if (!gpsSerialAvailable)
        {
            statusText = "GPS RX PIN NOT SET";
            statusColor = Graphics::RED;
        }
        else
        {
            createLogFiles(storage);
        }
        dirty = true;
    }

    void onUpdate(Input& input, Audio& audio, Storage& storage, float deltaSec) override
    {
        (void)audio;
        (void)deltaSec;

        receiveGps();
        updateRecording(storage);

        if (input.justPressed(Input::START))
        {
            if (!gpsSerialAvailable)
            {
                statusText = "GPS RX PIN NOT SET";
                statusColor = Graphics::RED;
            }
            else if (!logReady)
            {
                statusText = "LOG FILE NOT READY";
                statusColor = Graphics::RED;
            }
            else
            {
                recording = !recording;
                lastRecordMsec = Platform::getMsec() - RECORD_INTERVAL_MSEC;
                statusText = recording ? "RECORDING STARTED" : "RECORDING STOPPED";
                statusColor = recording ? Graphics::GREEN : Graphics::YELLOW;
            }
        }

        if (input.justPressed(Input::A))
        {
            if (!hasFix())
            {
                statusText = "MARKER NEEDS GPS FIX";
                statusColor = Graphics::YELLOW;
            }
            else if (appendMarker(storage))
            {
                statusText = "MARKER SAVED";
                statusColor = Graphics::CYAN;
            }
            else
            {
                statusText = "MARKER SAVE FAILED";
                statusColor = Graphics::RED;
            }
        }

        if (input.justPressed(Input::B))
        {
            screen = screen == SCREEN_POSITION ? SCREEN_STATUS : SCREEN_POSITION;
            statusText = screen == SCREEN_POSITION ? "POSITION SCREEN" : "STATUS SCREEN";
            statusColor = Graphics::CYAN;
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
        graphics.drawString("GPS LOGGER", 12, 17, Graphics::WHITE,
                            Graphics::SIZE_22B,
                            Graphics::HorizontalAlign::LEFT,
                            Graphics::VerticalAlign::MIDDLE);

        char header[48];
        snprintf(header, sizeof(header), "%s%s  LOG:%lu",
                 GPS_DEMO_MODE ? "DEMO " : "",
                 recording ? "REC" : "STOP",
                 static_cast<unsigned long>(logCount));
        graphics.drawString(header, 308, 17,
                            recording ? Graphics::GREEN : Graphics::YELLOW,
                            Graphics::SIZE_13,
                            Graphics::HorizontalAlign::RIGHT,
                            Graphics::VerticalAlign::MIDDLE);

        if (screen == SCREEN_POSITION)
        {
            drawPositionScreen(graphics);
        }
        else
        {
            drawStatusScreen(graphics);
        }

        graphics.drawString(statusText, 160, 207, statusColor,
                            Graphics::SIZE_13,
                            Graphics::HorizontalAlign::CENTER,
                            Graphics::VerticalAlign::MIDDLE);

        graphics.drawString("START: REC  A: MARKER  B: SCREEN",
                            160, 220, Graphics::WHITE, Graphics::SIZE_10,
                            Graphics::HorizontalAlign::CENTER,
                            Graphics::VerticalAlign::MIDDLE);

        dirty = false;
        return true;
    }

    void onTerminate(Storage& storage) override
    {
        (void)storage;
        if (gpsSerialAvailable && !GPS_DEMO_MODE)
        {
            Serial1.end();
        }
    }
};

GpsLogger app;

void setup()
{
    PLAMIOmini::start(graphicsConfig, inputConfig, audioConfig, storageConfig, app);
}

void loop()
{
}
