#include "Bullet.h"
#include <cmath>

#include "CoreTank/Tank.h"
#include "Obstacles/Structures.h"

std::vector<Bullet*> Bullet::BulletList;

Bullet::Bullet(const Play::Vector2D& StartPos, const Play::Vector2D& Velocity, const float MaxDistance, const int OwnerId)
    : Pos(StartPos), Velocity(Velocity), MaxDistance(MaxDistance), OwnerId(OwnerId)
{
    std::string spriteName = "Bullet" + std::to_string(OwnerId + 1);
    SpriteId = Play::Graphics::GetSpriteId(spriteName.c_str());

    BulletList.push_back(this);
}

void Bullet::Update( const float ElapsedTime)
{
    // Move bullet
    Pos.x += Velocity.x * ElapsedTime;
    Pos.y += Velocity.y * ElapsedTime;
    const float Dist = std::sqrt(Velocity.x*Velocity.x + Velocity.y*Velocity.y) * ElapsedTime;
    DistanceTraveled += Dist;

    // Bullet-to-tank collision
    for (auto* CurrTank : Tank::GetAllTanks())
    {
        if (CurrTank->GetID() != OwnerId && CheckTankCollision(CurrTank))
        {
            CurrTank->TakeDamage(1);
            
            // Destroy Bullet
            DistanceTraveled = MaxDistance;
        }
    }

    // Bullet-to-bullet collision
    for (auto& Bullet : Bullet::BulletList)
    {
        if (Bullet != this && CheckBulletCollision(Bullet))
        {
            // Destroy Bullet
            DistanceTraveled = MaxDistance;
            Bullet->DistanceTraveled = Bullet->MaxDistance;
        }
    }

    // Bullet-to-Obstacle Collision
    const Structure& OuterWall = Structures.back();
    
    // Bullet left Outer Wall
    if (!StructureCollision(Pos, Radius, OuterWall))
    {
        bHasLeftOuterWall = true;
    }

    // Check Collision
    for (const auto& Obstacle : Structures)
    {
        // Skip Outer Wall unless marked
        if (&Obstacle == &Structures.back() && !bHasLeftOuterWall) { continue; }

        if (StructureCollision(Pos, Radius, Obstacle) || bHasLeftOuterWall)
        {
            // Destroy Bullet
            DistanceTraveled = MaxDistance;
            return;
        }
    }
}

void Bullet::Draw() const
{
    const float Rotation = std::atan2(Velocity.y, Velocity.x);
    const float BulletScale = 0.2f;
    
    Play::DrawSpriteRotated(SpriteId, Pos, 0, Rotation, BulletScale);
}
bool Bullet::IsAlive() const
{
    return DistanceTraveled < MaxDistance;
}

void Bullet::CreateBullet(const Play::Vector2D& StartPos, const Play::Vector2D& Velocity, const float MaxDistance, const int OwnerId)
{
    Bullet* NewBullet = new Bullet(StartPos, Velocity, MaxDistance, OwnerId);
    BulletList.push_back(NewBullet);
}

bool Bullet::CheckTankCollision(const Tank* Tank) const
{
    const Play::Vector2D Diff = { Pos.x - Tank->GetPosition().x, Pos.y - Tank->GetPosition().y };
    const float DistSquared = Diff.x*Diff.x + Diff.y*Diff.y;
    float CollisionRadius = Radius + Tank->GetRadius();
    return DistSquared <= (CollisionRadius * CollisionRadius);
}

bool Bullet::CheckBulletCollision(const Bullet* Other) const
{
    const Play::Vector2D Diff = { Pos.x - Other->Pos.x, Pos.y - Other->Pos.y };
    const float DistSquared = Diff.x*Diff.x + Diff.y*Diff.y;
    const float CollisionDist = Radius + Other->Radius;
    return DistSquared <= (CollisionDist*CollisionDist);
}
