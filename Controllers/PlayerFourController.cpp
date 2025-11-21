#include "PlayerFourController.h"
#include "Globals.h"

void PlayerFourController::Update(Tank* tank, float elapsedTime)
{
    if (Play::KeyDown(Play::KEY_G))
    {
        tank->Move(TANK_MOVE_SPEED);
    }
    else if (Play::KeyDown(Play::KEY_V))
    {
        tank->Move(-TANK_MOVE_SPEED);
    }
    
    if (Play::KeyDown(Play::KEY_C))
    {
        tank->Rotate(-TANK_ROTATION_SPEED);
    }
    else if (Play::KeyDown(Play::KEY_B))
    {
        tank->Rotate(TANK_ROTATION_SPEED);
    }
    
    bool bShouldShoot = Play::KeyDown(Play::KEY_M);
    tank->Shoot(bShouldShoot, elapsedTime);
}
