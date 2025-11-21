#define PLAY_IMPLEMENTATION
#define PLAY_USING_GAMEOBJECT_MANAGER

#include "AI/Controllers/DebugAIController.h"
#include "AI/Controllers/FSM/FSMController.h"
#include "AI/Controllers/BT/BehaviorTreeController.h"
#include "AI/Gateway/AIServiceGateway.h"
#include "AISubsystem.h"
#include "CoreBullet/Bullet.h"
#include "CoreTank/Tank.h"
#include "Globals.h"
#include "Obstacles/Structures.h"
#include "Play.h"
#include "Services/Motion/Motion.h"
#include "Services/Pathfinding/Pathfinding.h"

// -----------------------------------------------------------------------------
// AI Controller Spawn Toggles
// -----------------------------------------------------------------------------
constexpr bool ENABLE_AI_DEBUG = false;  // Debug overlay oriented controller
constexpr bool ENABLE_AI_FSM   = true;  // Finite State Machine controller
constexpr bool ENABLE_AI_BT    = true;  // Behavior Tree controller

enum class ControllerKind { Debug, FSM, BT };

// Tank spawn positions
std::vector<Play::Vector2D> SpawnPositions = {
    {375, 100},
    {895, 625},
    {895, 100},
    {375, 625}
};

std::unique_ptr<AI::AISubsystem> g_ai;

// The entry point for a PlayBuffer program
void MainGameEntry( PLAY_IGNORE_COMMAND_LINE ) {
    Play::CreateManager(DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_SCALE);
    Play::CentreAllSpriteOrigins();
    Play::LoadBackground("Data/Backgrounds/TankBackground.png");

    // Build list of controllers to spawn based on toggles
    std::vector<ControllerKind> controllers;
    controllers.reserve(3);
    if constexpr (ENABLE_AI_DEBUG) controllers.push_back(ControllerKind::Debug);
    if constexpr (ENABLE_AI_FSM)   controllers.push_back(ControllerKind::FSM);
    if constexpr (ENABLE_AI_BT)    controllers.push_back(ControllerKind::BT);

    const int NUM_PLAYERS = static_cast<int>(controllers.size());

    // Safety: nothing enabled -> early out
    if (NUM_PLAYERS == 0) {
        return;
    }

    // Initialize structures
    InitStructures();

    g_ai = std::make_unique<AI::AISubsystem>();

    // Configure and build initial pathfinding graph (playable area = outer wall)
    if (!Structures.empty())
    {
        const auto& [BottomLeft, Size] = Structures.back();

        Pathfinding::PathfindingConfig params;
        params.playableOrigin = BottomLeft;
        params.playableSize   = Size;
        params.tankRadius     = 32.0f;
        params.safetyMargin   = 6.0f;
        params.outerInset     = params.tankRadius + params.safetyMargin;
        params.turnRadius     = 28.0f;

        g_ai->pathfinder().SetConfig(params);
        g_ai->pathfinder().Rebuild();
    }

    // Create Tanks
    for (int i = 0; i < NUM_PLAYERS; ++i)
    {
        Tank::CreateTank(SpawnPositions[i], i);
    }

    // Load Tank & Bullet SpriteSheets (indexed by tank id)
    for (int i = 0; i < NUM_PLAYERS; ++i)
    {
        std::string TankImageName   = "Tank"   + std::to_string(i + 1);
        std::string BulletImageName = "Bullet" + std::to_string(i + 1);

        Play::Graphics::LoadSpriteSheet("Data/Sprites/", TankImageName);
        Play::Graphics::LoadSpriteSheet("Data/Sprites/", BulletImageName);
    }

    // Attach per-tank services + controllers
    for (int i = 0; i < NUM_PLAYERS; ++i) {
        Tank* t = Tank::GetAllTanks()[i];
        const auto id = static_cast<AI::TankId>(i);
        g_ai->addTank(id, t, nullptr); // create per-tank services + gateway

        switch (controllers[i]) {
            case ControllerKind::Debug: {
                auto ctrl = std::make_unique<AI::DebugAIController>(g_ai->gateway(id));
                g_ai->setController(id, std::move(ctrl));
                break;
            }
            case ControllerKind::FSM: {
                auto ctrl = std::make_unique<AI::FSMController>(g_ai->gateway(id));
                g_ai->setController(id, std::move(ctrl));
                break;
            }
            case ControllerKind::BT: {
                auto ctrl = std::make_unique<AI::BT::BehaviorTreeController>(g_ai->gateway(id));
                g_ai->setController(id, std::move(ctrl));
                break;
            }
        }
    }
}

// Called by PlayBuffer every frame (60 times a second!)
bool MainGameUpdate( float elapsedTime )
{
    Play::DrawBackground();

    if (g_ai) {
        // Update AI subsystem (sensing + controllers)
        g_ai->tick(elapsedTime);
    }

    // Update all tanks (physics, draw)
    for (auto* T : Tank::GetAllTanks())
    {
        T->Update(elapsedTime);
    }

    // Update bullets
    for (auto* B : Bullet::GetAllBullets())
    {
        B->Update(elapsedTime);
        if (B->IsAlive()) B->Draw();
    }
    // Remove dead bullets
    Bullet::GetAllBullets().erase(
        std::ranges::remove_if(Bullet::GetAllBullets(),
                   [](const Bullet* b){ return !b->IsAlive(); }).begin(),
        Bullet::GetAllBullets().end()
    );

    if (g_ai) g_ai->renderDebugOverlay();

    Play::PresentDrawingBuffer();
    return Play::KeyDown( KEY_ESCAPE );
}

// Gets called once when the player quits the game 
int MainGameExit( void )
{
    Play::DestroyManager();
    return PLAY_OK;
}
