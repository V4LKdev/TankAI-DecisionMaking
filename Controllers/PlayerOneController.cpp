#include "PlayerOneController.h"
#include "Globals.h"

void PlayerOneController::Update(Tank* tank, float elapsedTime)
{
    if (Play::KeyDown(Play::KEY_UP))
    {
        tank->Move(TANK_MOVE_SPEED);
    }
    else if (Play::KeyDown(Play::KEY_DOWN))
    {
        tank->Move(-TANK_MOVE_SPEED);
    }

    if (Play::KeyDown(Play::KEY_LEFT))
    {
        tank->Rotate(-TANK_ROTATION_SPEED);
    }
    else if (Play::KeyDown(Play::KEY_RIGHT))
    {
        tank->Rotate(TANK_ROTATION_SPEED);
    }

    bool bShouldShoot = Play::KeyDown(Play::KEY_ENTER);
    tank->Shoot(bShouldShoot, elapsedTime);
}
