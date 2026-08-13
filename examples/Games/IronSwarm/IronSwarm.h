#pragma once
#include <PRUZEAmini.h>

class IronSwarm : public PRUZEAmini::App
{
public:
    const char* getId() const override { return "iron_swarm"; }

protected:
    void onInit(PRUZEAmini::Storage& storage) override;
    void onUpdate(PRUZEAmini::Input& input, PRUZEAmini::Audio& audio,
                             PRUZEAmini::Storage& storage, float deltaSec) override;
    bool onDraw(PRUZEAmini::Graphics& graphics, bool requestFullRedraw) override;
    void onTerminate(PRUZEAmini::Storage& storage) override;

private:
    enum Mode : uint8_t {
        MODE_TITLE,
        MODE_READY,
        MODE_GO,
        MODE_PLAYING,
        MODE_LEVEL_UP,
        MODE_PAUSED,
        MODE_GAME_OVER,
        MODE_RESULT,
        MODE_RANKING
    };

    enum Upgrade : uint8_t {
        UPGRADE_POWER,
        UPGRADE_RAPID,
        UPGRADE_MULTI,
        UPGRADE_PIERCE,
        UPGRADE_ENGINE,
        UPGRADE_ARMOR,
        UPGRADE_TURRET,
        UPGRADE_COUNT
    };

    enum EnemyType : uint8_t {
        ENEMY_SCOUT,
        ENEMY_GUNNER,
        ENEMY_HEAVY,
        ENEMY_BOSS
    };

    enum MissionType : uint8_t {
        MISSION_NONE,
        MISSION_HOLD_BRIDGE,
        MISSION_BREAKTHROUGH,
        MISSION_DESTROY,
        MISSION_ESCORT,
        MISSION_INTERCEPT,
        MISSION_COUNT
    };

    enum ObstacleType : uint8_t {
        OBSTACLE_ROCK,
        OBSTACLE_RUIN
    };

    struct Enemy {
        PRUZEAmini::Vector2 position;
        float radius;
        float speed;
        float shootTimer;
        int hp;
        EnemyType type;
        bool active;
    };

    struct Bullet {
        PRUZEAmini::Vector2 position;
        PRUZEAmini::Vector2 velocity;
        float life;
        int damage;
        int pierce;
        bool active;
    };

    struct EnemyBullet {
        PRUZEAmini::Vector2 position;
        PRUZEAmini::Vector2 velocity;
        float life;
        uint8_t damage;
        bool active;
    };

    struct ExpOrb {
        PRUZEAmini::Vector2 position;
        uint8_t value;
        bool active;
    };

    struct RepairItem {
        PRUZEAmini::Vector2 position;
        bool active;
    };

    struct Particle {
        PRUZEAmini::Vector2 position;
        PRUZEAmini::Vector2 velocity;
        float life;
        float maxLife;
        uint8_t size;
        PRUZEAmini::Graphics::Color color;
        bool active;
    };

    struct TitleBullet {
        PRUZEAmini::Vector2 position;
        PRUZEAmini::Vector2 velocity;
        float life;
        bool active;
    };

    struct Bomber {
        PRUZEAmini::Vector2 position;
        PRUZEAmini::Vector2 direction;
        float speed;
        float warningTimer;
        float bombTimer;
        float flightTimer;
        bool active;
        bool flying;
        bool enteredView;
    };

    struct Bomb {
        PRUZEAmini::Vector2 position;
        float fuse;
        float explosionTimer;
        bool exploded;
        bool active;
    };

    struct Obstacle {
        float x;
        float y;
        float w;
        float h;
        ObstacleType type;
    };

    struct Mission {
        MissionType type;
        PRUZEAmini::Vector2 targetPosition;
        PRUZEAmini::Vector2 targetPosition2;
        int targetBridge;
        float progress;
        float timeLeft;
        uint8_t stage;
        bool active;
    };

    struct MissionTarget {
        PRUZEAmini::Vector2 position;
        float radius;
        int hp;
        int maxHp;
        bool active;
    };

    struct EscortVehicle {
        PRUZEAmini::Vector2 position;
        float speed;
        bool joined;
        bool active;
    };

    struct SupplyVehicle {
        PRUZEAmini::Vector2 position;
        PRUZEAmini::Vector2 escapeTarget;
        float speed;
        int hp;
        int maxHp;
        bool active;
    };

    static constexpr int SCREEN_W = 320;
    static constexpr int SCREEN_H = 240;

    static constexpr int WORLD_W = 1600;
    static constexpr int WORLD_H = 1200;

    static constexpr float RIVER_X = 760.0f;
    static constexpr float RIVER_W = 80.0f;
    static constexpr float BRIDGE_0_Y = 260.0f;
    static constexpr float BRIDGE_1_Y = 820.0f;
    static constexpr float BRIDGE_H = 80.0f;

    static constexpr int MAX_ENEMIES = 18;
    static constexpr int MAX_BULLETS = 48;
    static constexpr int MAX_ENEMY_BULLETS = 36;
    static constexpr int MAX_ORBS = 32;
    static constexpr int MAX_REPAIRS = 4;
    static constexpr int MAX_PARTICLES = 40;
    static constexpr int MAX_TITLE_BULLETS = 3;
    static constexpr int MAX_BOMBS = 10;
    static constexpr int RANKING_COUNT = 10;
    static constexpr int OBSTACLE_COUNT = 10;

    static constexpr float PLAYER_RADIUS = 9.0f;
    static constexpr float ENEMY_RADIUS = 7.0f;
    static constexpr float BULLET_RADIUS = 2.0f;

    static const Obstacle OBSTACLES[OBSTACLE_COUNT];

    Mode mode = MODE_TITLE;

    PRUZEAmini::Vector2 player;
    PRUZEAmini::Vector2 lastMoveDirection = PRUZEAmini::Vector2(1.0f, 0.0f);

    float bodyAngle = 0.0f;
    float turretAngle = 0.0f;
    float turretTurnSpeed = 2.8f;

    int hp = 5;
    int maxHp = 5;
    int level = 1;
    int experience = 0;
    int nextLevelExp = 8;
    uint32_t score = 0;
    uint32_t kills = 0;

    uint32_t rankings[RANKING_COUNT] = {};
    int lastRank = -1;
    bool rankingFromTitle = false;
    bool runFinalized = false;

    float moveSpeed = 72.0f;
    int bulletDamage = 1;
    float fireInterval = 0.48f;
    int multiShot = 1;
    int bulletPierce = 0;

    float fireTimer = 0.0f;
    float spawnTimer = 0.0f;
    float gameTime = 0.0f;
    float invincibleTimer = 0.0f;
    float hitFlashTimer = 0.0f;
    float bomberEventTimer = 18.0f;
    float bossTimer = 70.0f;
    float missionTimer = 22.0f;
    float missionNoticeTimer = 0.0f;
    float stateTimer = 0.0f;

    float cameraShakeTimer = 0.0f;
    float cameraShakeDuration = 0.0f;
    float cameraShakeStrength = 0.0f;

    PRUZEAmini::Vector2 destroyBlastPosition;
    float destroyBlastTimer = 0.0f;

    float titleTankX = 132.0f;
    float titleTankSpeed = 20.0f;
    float titleTurretAngle = 0.0f;
    float titleFireTimer = 0.25f;
    float titleAnimTime = 0.0f;

    bool specialTargetHeld = false;

    MissionType lastMissionType = MISSION_NONE;

    int selectedUpgrade = 0;
    Upgrade upgradeChoices[3] = {
        UPGRADE_POWER, UPGRADE_RAPID, UPGRADE_ENGINE
    };

    Enemy enemies[MAX_ENEMIES];
    Bullet bullets[MAX_BULLETS];
    EnemyBullet enemyBullets[MAX_ENEMY_BULLETS];
    ExpOrb orbs[MAX_ORBS];
    RepairItem repairs[MAX_REPAIRS];
    Particle particles[MAX_PARTICLES];
    TitleBullet titleBullets[MAX_TITLE_BULLETS];
    Bomb bombs[MAX_BOMBS];
    Bomber bomber{};

    Mission mission{MISSION_NONE, PRUZEAmini::Vector2(), PRUZEAmini::Vector2(), 0, 0.0f, 0.0f, 0, false};
    MissionTarget missionTarget{};
    EscortVehicle escortVehicle{};
    SupplyVehicle supplyVehicle{};

    void resetGame();
    void clearObjects();
    void finalizeRun(PRUZEAmini::Storage& storage);

    void updateTitle(float deltaSec);
    void updatePlayer(PRUZEAmini::Input& input, float deltaSec);
    void resolveTerrainCollision();
    void resolveActorTerrain(PRUZEAmini::Vector2& position, float radius);
    bool isActorPositionBlocked(const PRUZEAmini::Vector2& position, float radius) const;
    bool isBulletBlocked(const PRUZEAmini::Vector2& position) const;
    bool isWorldPointVisible(const PRUZEAmini::Vector2& position, float margin = 0.0f) const;

    void updateEnemies(PRUZEAmini::Audio& audio, float deltaSec);
    void updateEnemyShooting(float deltaSec);
    void updateBullets(PRUZEAmini::Audio& audio, float deltaSec);
    void updateEnemyBullets(PRUZEAmini::Audio& audio, float deltaSec);
    void updateOrbs(PRUZEAmini::Audio& audio, float deltaSec);
    void updateRepairs(PRUZEAmini::Audio& audio);
    void updateParticles(float deltaSec);
    void updateSpawning(float deltaSec);
    void updateAutoFire(PRUZEAmini::Audio& audio, float deltaSec);

    void spawnEnemy();
    void spawnBoss(PRUZEAmini::Audio& audio);
    void spawnEnemyBullet(const PRUZEAmini::Vector2& position, float angle, float speed, uint8_t damage);
    void spawnBullet(float angle);
    void spawnOrb(const PRUZEAmini::Vector2& position, uint8_t value);
    void spawnMissionReward(const PRUZEAmini::Vector2& position);
    void spawnRepair(const PRUZEAmini::Vector2& position);
    void spawnDeathParticles(const PRUZEAmini::Vector2& position,
                             PRUZEAmini::Graphics::Color color, int count);
    void triggerCameraShake(float strength, float duration);
    void triggerDestroyExplosion(const PRUZEAmini::Vector2& position);
    int findNearestEnemy() const;
    bool getSpecialTarget(PRUZEAmini::Vector2& outPosition) const;
    bool hasBoss() const;

    PRUZEAmini::Vector2 getEnemyMoveTarget(const Enemy& enemy) const;
    PRUZEAmini::Vector2 getBridgeCenter(int bridgeIndex) const;
    PRUZEAmini::Vector2 getMoveTargetAcrossRiver(const PRUZEAmini::Vector2& from,
                                             const PRUZEAmini::Vector2& destination) const;

    void updateBomber(PRUZEAmini::Audio& audio, float deltaSec);
    void startBomberRun(PRUZEAmini::Audio& audio);
    void dropBomb();
    void updateBombs(PRUZEAmini::Audio& audio, float deltaSec);
    void explodeBomb(Bomb& bomb, PRUZEAmini::Audio& audio);

    void updateMission(PRUZEAmini::Audio& audio, float deltaSec);
    void startMission(PRUZEAmini::Audio& audio);
    void failMission(PRUZEAmini::Audio& audio);
    void completeMission(const PRUZEAmini::Vector2& rewardPosition,
                         PRUZEAmini::Audio& audio, bool dropRepair);
    void clearMissionObjects();
    void startHoldBridgeMission();
    void startBreakthroughMission();
    void startDestroyMission();
    void startEscortMission();
    void startInterceptMission();
    void updateEscortVehicle(float deltaSec);
    void updateSupplyVehicle(float deltaSec);
    bool handleMissionBulletHit(Bullet& bullet, PRUZEAmini::Audio& audio);
    const char* getMissionName() const;

    void damagePlayer(int damage, PRUZEAmini::Audio& audio);

    void gainExperience(int amount, PRUZEAmini::Audio& audio);
    void beginLevelUp(PRUZEAmini::Audio& audio);
    void generateUpgradeChoices();
    void applyUpgrade(Upgrade upgrade, PRUZEAmini::Audio& audio);
    const char* getUpgradeName(Upgrade upgrade) const;
    const char* getUpgradeDescription(Upgrade upgrade) const;
    const char* getUpgradeValue(Upgrade upgrade) const;

    void loadRanking(PRUZEAmini::Storage& storage);
    void saveRanking(PRUZEAmini::Storage& storage);
    int insertRanking(uint32_t value);

    void drawWorld(PRUZEAmini::Graphics& graphics) const;
    void drawParticles(PRUZEAmini::Graphics& graphics) const;
    void drawTerrain(PRUZEAmini::Graphics& graphics) const;
    void drawObstacles(PRUZEAmini::Graphics& graphics) const;
    void drawTank(PRUZEAmini::Graphics& graphics) const;
    void drawEnemy(PRUZEAmini::Graphics& graphics, const Enemy& enemy) const;
    void drawMissionObjects(PRUZEAmini::Graphics& graphics) const;
    void drawBomber(PRUZEAmini::Graphics& graphics) const;
    void drawBombs(PRUZEAmini::Graphics& graphics) const;
    void drawHud(PRUZEAmini::Graphics& graphics) const;
    void drawRadar(PRUZEAmini::Graphics& graphics) const;
    void drawMission(PRUZEAmini::Graphics& graphics) const;
    void drawTitle(PRUZEAmini::Graphics& graphics) const;
    void drawReady(PRUZEAmini::Graphics& graphics) const;
    void drawLevelUp(PRUZEAmini::Graphics& graphics) const;
    void drawPause(PRUZEAmini::Graphics& graphics) const;
    void drawGameOver(PRUZEAmini::Graphics& graphics) const;
    void drawResult(PRUZEAmini::Graphics& graphics) const;
    void drawRanking(PRUZEAmini::Graphics& graphics) const;

    static int16_t sx(float value);
    static int16_t sy(float value);
};
