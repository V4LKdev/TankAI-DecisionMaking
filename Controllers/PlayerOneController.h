#pragma once

#ifndef PLAYERONECONTROLLER_H
#define PLAYERONECONTROLLER_H

#include "CoreTank/TankController.h"

class PlayerOneController : public ITankController
{
public:
    void Update(Tank* tank, float elapsedTime) override;
};

#endif