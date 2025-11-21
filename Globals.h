#pragma once

#include "CoreBullet/Bullet.h"
#include <vector>

static int DISPLAY_WIDTH = 1280;
static int DISPLAY_HEIGHT = 719;
static int DISPLAY_SCALE = 1;

// Motion control constants
static constexpr float TANK_MOVE_SPEED = 2.0f;
static constexpr float TANK_ROTATION_SPEED = 0.05f;

extern std::vector<Play::Vector2D> SpawnPositions;