#include "PlayerTwoController.h"
#include "Globals.h"

void PlayerTwoController::Update(Tank* tank, float elapsedTime)
{
    if (Play::KeyDown(Play::KEY_W))
    {
        tank->Move(TANK_MOVE_SPEED);
    }
    else if (Play::KeyDown(Play::KEY_S))
    {
        tank->Move(-TANK_MOVE_SPEED);
    }

    if (Play::KeyDown(Play::KEY_A))
    {
        tank->Rotate(-TANK_ROTATION_SPEED);
    }
    else if (Play::KeyDown(Play::KEY_D))
    {
        tank->Rotate(TANK_ROTATION_SPEED);
    }
    
    bool bShouldShoot = Play::KeyDown(Play::KEY_SPACE);
    tank->Shoot(bShouldShoot, elapsedTime);
}
