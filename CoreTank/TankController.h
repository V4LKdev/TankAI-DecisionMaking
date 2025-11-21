#pragma once
#ifndef TANKCONTROLLER_H
#define TANKCONTROLLER_H

#include "Tank.h"

class ITankController
{
public:
    virtual ~ITankController() = default;
    virtual void Update(Tank* tank, float elapsedTime) = 0;
};

#endif