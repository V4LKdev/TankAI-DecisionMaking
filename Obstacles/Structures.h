#pragma once

#include "Play.h"

struct Structure
{
    Play::Point2D BottomLeft;
    Play::Point2D Size;
};

extern std::vector<Structure> Structures;

void InitStructures();
void DrawStructures();

bool StructureCollision(const Play::Vector2D& center, float Radius, const Structure& Obstacle);