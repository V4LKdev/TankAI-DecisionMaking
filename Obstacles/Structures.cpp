#include "Structures.h"

std::vector<Structure> Structures;

void InitStructures()
{
    // Corner Boxes
    Structures.push_back({ {405, 135}, {65, 65} });
    Structures.push_back({ {795, 135}, {65, 65} });
    Structures.push_back({ {795, 525}, {65, 65} });
    Structures.push_back({ {405, 525}, {65, 65} });

    // Edge Boxes
    Structures.push_back({ {405, 300}, {65, 125} });
    Structures.push_back({ {795, 300}, {65, 125} });
    Structures.push_back({ {570, 135}, {125, 65} });
    Structures.push_back({ {570, 525}, {125, 65} });

    // Center Box
    Structures.push_back({ {570, 300}, {125, 125} });

    // Outer Wall Box
    Structures.push_back({ {325, 50}, {615, 625} });
}

void DrawStructures()
{
    for (const auto& Obstacle : Structures)
    {
        const Play::Point2D TopRight = {Obstacle.BottomLeft.x + Obstacle.Size.x, Obstacle.BottomLeft.y + Obstacle.Size.y};
        Play::DrawRect(Obstacle.BottomLeft, TopRight, Play::cBlack);
    }
}

bool StructureCollision(const Play::Vector2D& center, float radius, const Structure& Obstacle)
{
    float closestX = std::clamp(center.x, Obstacle.BottomLeft.x, Obstacle.BottomLeft.x + Obstacle.Size.x);
    float closestY = std::clamp(center.y, Obstacle.BottomLeft.y, Obstacle.BottomLeft.y + Obstacle.Size.y);

    float dx = center.x - closestX;
    float dy = center.y - closestY;

    return (dx*dx + dy*dy) <= (radius * radius);
}
