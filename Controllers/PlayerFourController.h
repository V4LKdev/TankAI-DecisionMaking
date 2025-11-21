#pragma once

#ifndef PLAYERFOURCONTROLLER_H
#define PLAYERFOURCONTROLLER_H

#include "CoreTank/TankController.h"

class PlayerFourController : public ITankController
{
public:
    void Update(Tank* tank, float elapsedTime) override;
};

#endif