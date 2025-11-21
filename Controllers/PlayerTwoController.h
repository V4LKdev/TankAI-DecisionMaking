#pragma once

#ifndef PLAYERTWOCONTROLLER_H
#define PLAYERTWOCONTROLLER_H

#include "CoreTank/TankController.h"

class PlayerTwoController : public ITankController
{
public:
    void Update(Tank* tank, float elapsedTime) override;
};

#endif