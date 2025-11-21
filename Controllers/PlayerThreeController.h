#pragma once

#ifndef PLAYERTHREECONTROLLER_H
#define PLAYERTHREECONTROLLER_H

#include "CoreTank/TankController.h"

class PlayerThreeController : public ITankController
{
public:
    void Update(Tank* tank, float elapsedTime) override;
};

#endif