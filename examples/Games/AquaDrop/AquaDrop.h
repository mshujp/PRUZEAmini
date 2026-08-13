#pragma once
#include <PRUZEAmini.h>

class AquaDrop : public PRUZEAmini::App
{
public:
    const char* getId() const override;





protected:
    void onInit(PRUZEAmini::Storage& storage) override;
    void onUpdate(PRUZEAmini::Input& input, PRUZEAmini::Audio& audio, PRUZEAmini::Storage& storage, float deltaSec) override;
    bool onDraw(PRUZEAmini::Graphics& graphics, bool requestFullRedraw) override;
    void onTerminate(PRUZEAmini::Storage& storage) override;

private:
    enum Mode : uint8_t
    {
        MODE_TITLE,
        MODE_STAGE_INTRO,
        MODE_PLAYING,
        MODE_CLEARING,
        MODE_FALLING,
        MODE_STAGE_CLEAR,
        MODE_PAUSED,
        MODE_GAME_OVER,
        MODE_ALL_CLEAR
    };

    static constexpr int16_t SCREEN_W = 320;
    static constexpr int16_t SCREEN_H = 240;
    static constexpr int16_t BOARD_COLS = 6;
    static constexpr int16_t BOARD_ROWS = 12;
    static constexpr int16_t CELL_SIZE = 16;
    static constexpr int16_t BOARD_X = 20;
    static constexpr int16_t BOARD_Y = 24;
    static constexpr int16_t BOARD_W = BOARD_COLS * CELL_SIZE;
    static constexpr int16_t BOARD_H = BOARD_ROWS * CELL_SIZE;
    static constexpr uint8_t EMPTY = 0;
    static constexpr uint8_t MAX_COLORS = 5;

    static constexpr uint32_t STAGE_INTRO_MSEC = 1200;
    static constexpr uint32_t CLEAR_FLASH_MSEC = 260;
    static constexpr uint32_t FALL_SETTLE_MSEC = 130;
    static constexpr uint32_t STAGE_CLEAR_MSEC = 1800;
    static constexpr uint32_t GAME_OVER_MSEC = 2200;
    static constexpr uint32_t ALL_CLEAR_MSEC = 3200;
    static constexpr uint32_t TITLE_BLINK_MSEC = 500;
    static constexpr uint32_t CHAIN_DISPLAY_MSEC = 900;

    static constexpr float SOFT_DROP_MULTIPLIER = 8.0f;
    static constexpr int32_t SCORE_PER_BUBBLE = 10;
    static constexpr int32_t GROUP_BONUS_PER_EXTRA = 20;
    static constexpr int32_t STAGE_CLEAR_BONUS = 1000;

    static constexpr uint8_t STAGE_COUNT = 5;
    static constexpr uint8_t STAGE_COLOR_COUNT[STAGE_COUNT] = {3, 4, 4, 5, 5};
    static constexpr uint16_t STAGE_TARGET[STAGE_COUNT] = {20, 35, 50, 65, 80};
    static constexpr uint8_t STAGE_INITIAL_ROWS[STAGE_COUNT] = {1, 2, 2, 3, 3};
    static constexpr float STAGE_FALL_INTERVAL_SEC[STAGE_COUNT] = {0.90f, 0.78f, 0.68f, 0.58f, 0.48f};

    Mode mode = MODE_TITLE;
    Mode modeBeforePause = MODE_PLAYING;
    uint8_t board[BOARD_ROWS][BOARD_COLS] = {};
    bool clearMask[BOARD_ROWS][BOARD_COLS] = {};

    int16_t pivotX = 2;
    int16_t pivotY = 0;
    uint8_t orientation = 0;
    uint8_t pivotColor = 1;
    uint8_t childColor = 2;
    uint8_t nextPivotColor = 1;
    uint8_t nextChildColor = 2;

    uint8_t stageIndex = 0;
    uint16_t stageClearedCount = 0;
    uint16_t pendingClearCount = 0;
    uint8_t chainCount = 0;
    uint8_t displayedChain = 0;
    int32_t score = 0;
    int32_t highScore = 0;
    bool saveDirty = false;

    float fallAccumulatorSec = 0.0f;
    uint32_t modeStartMsec = 0;
    uint32_t chainDisplayStartMsec = 0;
    uint32_t shakeStartMsec = 0;
    uint16_t shakeDurationMsec = 0;

    PRUZEAmini::SaveData saveData;

    void resetToTitle();
    void startNewGame();
    void prepareStage();
    void fillInitialBoard();
    void spawnPair();
    void generateNextPair();
    uint8_t randomColor() const;

    bool isCellFree(int16_t x, int16_t y) const;
    void getChildPosition(int16_t x, int16_t y, uint8_t dir, int16_t& outX, int16_t& outY) const;
    bool canPlacePair(int16_t x, int16_t y, uint8_t dir) const;
    bool tryMove(int16_t dx, int16_t dy);
    bool tryRotate(int8_t direction);
    void lockPair();

    bool findClearGroups();
    uint16_t markGroup(int16_t startX, int16_t startY, bool visited[BOARD_ROWS][BOARD_COLS]);
    void removeMarkedBubbles();
    bool applyGravity();
    bool isBoardOverflowed() const;

    void enterMode(Mode newMode);
    void finishStage(PRUZEAmini::Audio& audio, PRUZEAmini::Storage& storage);
    void finishGameOver(PRUZEAmini::Storage& storage);
    void updateHighScore();
    void saveIfNeeded(PRUZEAmini::Storage& storage);

    void drawBackground(PRUZEAmini::Graphics& graphics) const;
    void drawTitle(PRUZEAmini::Graphics& graphics, uint32_t now) const;
    void drawGame(PRUZEAmini::Graphics& graphics, uint32_t now) const;
    void drawBoard(PRUZEAmini::Graphics& graphics, uint32_t now) const;
    void drawBubble(PRUZEAmini::Graphics& graphics, int16_t cx, int16_t cy, uint8_t colorIndex, bool flashing) const;
    void drawCurrentPair(PRUZEAmini::Graphics& graphics) const;
    void drawSidePanel(PRUZEAmini::Graphics& graphics, uint32_t now) const;
    void drawOverlay(PRUZEAmini::Graphics& graphics, uint32_t now) const;
    PRUZEAmini::Graphics::Color bubbleColor(uint8_t colorIndex) const;
};
