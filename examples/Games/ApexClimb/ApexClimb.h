#pragma once
#include <PRUZEAmini.h>

class ApexClimb : public PRUZEAmini::App
{
public:
    const char* getId() const override;

protected:
    void onInit(PRUZEAmini::Storage& storage) override;
    void onUpdate(PRUZEAmini::Input& input, PRUZEAmini::Audio& audio,
                  PRUZEAmini::Storage& storage, float deltaSec) override;
    bool onDraw(PRUZEAmini::Graphics& graphics, bool requestFullRedraw) override;
    void onTerminate(PRUZEAmini::Storage& storage) override;

public:
    struct Point {
        float x;
        float y;
    };

private:
    enum Mode : uint8_t {
        MODE_TITLE,
        MODE_COURSE_SELECT,
        MODE_READY,
        MODE_RUNNING,
        MODE_FINISHING,
        MODE_RESULT,
        MODE_RANKING,
        MODE_PAUSED,
        MODE_TUNING
    };

    enum HandlingState : uint8_t {
        HANDLING_NEUTRAL,
        HANDLING_UNDER,
        HANDLING_DRIFT
    };

    struct DrivingTuning {
        float maxSpeed = 148.0f;
        float displayMaxSpeedKmh = 195.0f;

        float acceleration = 86.0f;
        float brakeForce = 80.0f;
        float brakeLowSpeedScale = 0.32f;
        float brakeFullEffectKmh = 95.0f;
        float slideBrakeMinScale = 0.35f;
        float coastDrag = 22.0f;
        float offroadDrag = 118.0f;

        float steeringResponse = 6.5f;
        float lowSpeedTurnRate = 2.0f;
        float steeringEnableSpeed = 18.0f;
        float digitalFullSteerTime = 0.30f;
        float digitalFullThrottleTime = 0.30f;
        float digitalThrottleReleaseTime = 0.12f;
        float digitalFullBrakeTime = 0.18f;
        float digitalBrakeReleaseTime = 0.26f;

        // Maximum steering demand that the front tires can hold.
        // It falls with speed, creating high-speed understeer.
        float frontGripLowSpeed = 1.10f;
        float frontGripHighSpeed = 0.34f;
        float understeerMinYaw = 0.38f;

        // How quickly velocity aligns with the body while gripping.
        float gripAlign = 4.2f;

        // Drift entry / behavior.
        float driftMinSpeedKmh = 58.0f;
        float brakeDriftSteer = 0.28f;
        float brakeDriftAmount = 0.10f;
        float steerDriftMargin = 0.26f;

        float driftGripAlign = 0.80f;
        float driftYawScale = 1.85f;
        float driftAccelerationScale = 0.90f;
        float driftForwardEfficiency = 0.90f;

        float driftExitSpeedKmh = 25.0f;
        float driftExitSteer = 0.16f;
        float driftExitSlipRad = 0.10f;
        float liftDriftRecovery = 9.0f;
        float liftGripScale = 3.0f;
    };


    struct PresentationTuning {
        float engineGain = 0.70f;
        float tachRise = 2600.0f;
        float tachFall = 1900.0f;

        // Virtual gearbox used by RPM/audio presentation.
        float gear2Kmh = 58.0f;
        float gear3Kmh = 104.0f;
        float gear4Kmh = 150.0f;
        float gear5Kmh = 194.0f;

        // Real vehicle performance tuning kept separate from handling.
        float acceleration = 32.0f;
        float brakeForce = 80.0f;
        float movementScale = 1.45f;

        float topSpeedKmh = 220.0f;
        float highAccelStartKmh = 135.0f;
        float highAccelScale = 0.38f;
    };

    static constexpr int16_t SCREEN_W = 320;
    static constexpr int16_t SCREEN_H = 240;
    static constexpr int16_t VIEW_W = 240;
    static constexpr int16_t VIEW_H = 240;

    static constexpr int16_t WORLD_W = 980;
    static constexpr int16_t WORLD_H = 6700;
    static constexpr float ROAD_HALF_WIDTH = 34.0f;
    static constexpr float SHOULDER_HALF_WIDTH = 41.0f;

    static constexpr float REVERSE_SPEED = 38.0f;

    static constexpr uint8_t COURSE_COUNT = 4;
    static constexpr uint8_t COURSE_SELECT_COUNT = COURSE_COUNT + 1;
    static constexpr uint16_t TRACK_SAMPLES = 320;
    static constexpr uint8_t MINIMAP_SAMPLES = 48;
    static constexpr uint32_t RUN_TIMEOUT_MSEC = 120000;
    static constexpr uint32_t DRIFT_SAMPLE_MSEC = 200;
    static constexpr uint32_t DRIFT_DISPLAY_THRESHOLD = 1000;
    static constexpr uint16_t SUSUKI_COUNT = 150;
    static constexpr uint16_t TREE_COUNT_MAX = 82;
    static constexpr uint16_t ROCK_COUNT = 42;

    struct TrackSample
    {
        Point center;
        Point miter;
        float miterScale = 1.0f;
    };

    struct PreparedCourse
    {
        TrackSample samples[TRACK_SAMPLES + 1] = {};
    };

    Mode mode = MODE_TITLE;
    PRUZEAmini::SaveData saveData;
    uint8_t selectedCourse = 0;
    uint8_t currentCourse = 0;
    bool freePractice = false;
    PreparedCourse preparedCourse;

    bool sceneryPrepared = false;
    bool susukiEnabled[SUSUKI_COUNT] = {};
    bool treeEnabled[TREE_COUNT_MAX] = {};
    bool rockEnabled[ROCK_COUNT] = {};
    int16_t miniMapX[MINIMAP_SAMPLES + 1] = {};
    int16_t miniMapY[MINIMAP_SAMPLES + 1] = {};

    PRUZEAmini::Vector2 carPosition;
    PRUZEAmini::Vector2 velocity;
    float carAngle = 0.0f;
    float steering = 0.0f;
    float digitalSteering = 0.0f;
    float throttleInput = 0.0f;
    float brakeInput = 0.0f;
    float slipAngle = 0.0f;

    float engineRpm = 1800.0f;
    float displayRpm = 1800.0f;
    uint8_t virtualGear = 1;
    int8_t engineSoundIndex = -1;
    float engineSoundElapsed = 0.0f;
    uint32_t engineSoundMuteUntilMsec = 0;
    bool drifting = false;
    float driftBlend = 0.0f;
    float frontGripUsage = 0.0f;
    float rearGripFactor = 1.0f;
    HandlingState handlingState = HANDLING_NEUTRAL;
    float progress = 0.0f;
    float previousProgress = 0.0f;

    uint32_t readyStartMsec = 0;
    uint8_t readySignalStep = 0;
    uint32_t runStartMsec = 0;
    uint32_t finishMsec = 0;
    uint32_t finishStopMsec = 0;
    uint32_t finishEffectStartMsec = 0;
    bool finishStopped = false;
    static constexpr uint8_t RANKING_COUNT = 5;
    uint32_t rankings[COURSE_COUNT][RANKING_COUNT] = {};
    uint32_t driftRankings[COURSE_COUNT][RANKING_COUNT] = {};
    uint8_t finishRank = 0;
    uint8_t finishDriftRank = 0;
    uint32_t bestMsec = 0;
    uint32_t currentDriftScore = 0;
    uint32_t bestDriftScore = 0;
    uint32_t confirmedDriftScore = 0;
    uint32_t driftSampleElapsedMsec = 0;
    uint32_t driftScoreFlashStartMsec = 0;
    bool runDisqualified = false;
    uint32_t sectorStartMsec = 0;
    uint32_t sectorTimes[3] = {};
    uint8_t sector = 0;
    bool saveDirty = false;

    DrivingTuning tuning;
    PresentationTuning presentation;
    uint8_t tuningItem = 0;
    uint32_t tuningPauseStartMsec = 0;
    Mode tuningReturnMode = MODE_RUNNING;

    static constexpr uint8_t SKID_POINT_COUNT = 30;
    struct SkidPoint { PRUZEAmini::Vector2 left, right; bool active = false; };
    SkidPoint skidPoints[SKID_POINT_COUNT];
    uint8_t skidWriteIndex = 0;
    PRUZEAmini::Vector2 lastSkidSample;
    bool hasLastSkidSample = false;
    float skidLife = 0.0f;
    PRUZEAmini::Vector2 skidLeft;
    PRUZEAmini::Vector2 skidRight;
    PRUZEAmini::Vector2 previousSkidLeft;
    PRUZEAmini::Vector2 previousSkidRight;

    void resetToStart();
    void resetPractice();
    void startReady();
    void startRun();
    void updateDriving(PRUZEAmini::Input& input, PRUZEAmini::Audio& audio,
                       PRUZEAmini::Storage& storage, float deltaSec);
    void updateTuning(PRUZEAmini::Input& input);
    void updateEngineAudio(PRUZEAmini::Audio& audio, float deltaSec);
    void updateFinishing(PRUZEAmini::Audio& audio, float deltaSec);
    void resolveGuardrailCollision(float deltaSec);
    float readSteering(const PRUZEAmini::Input& input) const;
    float getDisplaySpeedKmh() const;
    float getFrontGripLimit(float speedRatio) const;
    void finishRun(PRUZEAmini::Audio& audio, PRUZEAmini::Storage& storage);
    void loadRanking(PRUZEAmini::Storage& storage);
    void saveRanking(PRUZEAmini::Storage& storage);
    uint8_t insertRanking(uint32_t msec);
    uint8_t insertDriftRanking(uint32_t score);
    void finishDriftScore(uint32_t now);

    Point getCoursePoint(float t) const;
    Point getCourseTangent(float t) const;
    float getTrackProgress(float x, float y) const;
    float distanceToTrack(float x, float y) const;
    static float distanceToSegmentSquared(float px, float py,
                                          const Point& a, const Point& b);

    void prepareCourse();
    void prepareScenery();
    void drawWorld(PRUZEAmini::Graphics& graphics) const;
    void drawRoad(PRUZEAmini::Graphics& graphics) const;
    void drawScenery(PRUZEAmini::Graphics& graphics) const;
    void drawPracticeWorld(PRUZEAmini::Graphics& graphics) const;
    void drawPracticeHud(PRUZEAmini::Graphics& graphics) const;
    void drawStreetLights(PRUZEAmini::Graphics& graphics) const;
    void drawMiniMap(PRUZEAmini::Graphics& graphics) const;
    void drawDriftScore(PRUZEAmini::Graphics& graphics, uint32_t now) const;
    void drawFinishVenue(PRUZEAmini::Graphics& graphics) const;
    void drawFinishConfetti(PRUZEAmini::Graphics& graphics) const;
    void drawSectorGate(PRUZEAmini::Graphics& graphics, float t,
                        PRUZEAmini::Graphics::Color color) const;
    bool crossedGate(float gateT) const;
    bool reachedFinishZone() const;
    void drawCar(PRUZEAmini::Graphics& graphics) const;
    void drawTelemetry(PRUZEAmini::Graphics& graphics, uint32_t now) const;
    void drawTitle(PRUZEAmini::Graphics& graphics) const;
    void drawCourseSelect(PRUZEAmini::Graphics& graphics) const;
    void drawReady(PRUZEAmini::Graphics& graphics, uint32_t now) const;
    void drawResult(PRUZEAmini::Graphics& graphics) const;
    void drawRanking(PRUZEAmini::Graphics& graphics) const;
    void drawPause(PRUZEAmini::Graphics& graphics) const;
    void drawTuning(PRUZEAmini::Graphics& graphics) const;
    void drawTime(PRUZEAmini::Graphics& graphics, uint32_t msec,
                  int16_t x, int16_t y,
                  PRUZEAmini::Graphics::Color color,
                  PRUZEAmini::Graphics::Font font,
                  PRUZEAmini::Graphics::HorizontalAlign align) const;
};
