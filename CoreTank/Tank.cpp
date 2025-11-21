#include "Tank.h"
#include <cmath>

#include "Globals.h"
#include "CoreBullet/Bullet.h"
#include "Obstacles/Structures.h"

std::vector<Tank*> Tank::TankList;

Tank::Tank( const Play::Vector2D StartPos, int Id)
	: Position(StartPos), Rotation(0.0f), TankID(Id)
{
    std::string spriteName = "Tank" + std::to_string(TankID + 1);
    SpriteId = Play::Graphics::GetSpriteId(spriteName.c_str());
}

void Tank::Move( const float Amount)
{
    const float Radius = GetRadius();
    Play::Vector2D NewPos = Position;

    const float dx = std::cos(Rotation) * Amount;
    const float dy = std::sin(Rotation) * Amount;

    // Check x - coordinate movement
    Play::Vector2D xCoordTest = NewPos;
    xCoordTest.x += dx;
    bool xCoordBlocked = false;
    
    for (const auto& Obstacle : Structures)
    {
        // Skip Outer Wall
        if (&Obstacle == &Structures.back()) { continue; }
        
        if (StructureCollision({ xCoordTest.x, NewPos.y }, Radius, Obstacle))
        {
            xCoordBlocked = true;
            break;
        }
    }

    // Check other tanks
    if (!xCoordBlocked)
    {
        for (const auto* OtherTank : TankList)
        {
            if (OtherTank->GetID() == TankID || !OtherTank->IsAlive()) { continue; }
            
            const Play::Vector2D TankOffset = { xCoordTest.x - OtherTank->GetPosition().x, NewPos.y - OtherTank->GetPosition().y };
            float CombinedRadius = Radius + OtherTank->GetRadius();
            
            if ((TankOffset.x*TankOffset.x + TankOffset.y*TankOffset.y) <= CombinedRadius*CombinedRadius)
            {
                xCoordBlocked = true;
                break;
            }
        }
    }
    
    if (!xCoordBlocked)
    {
        NewPos.x += dx;
    }

    // Check y - coordinate movement
    Play::Vector2D yCoordTest = NewPos;
    yCoordTest.y += dy;
    bool yCoordBlocked = false;
    
    for (const auto& Obstacle : Structures)
    {
        // Skip Outer Wall
        if (&Obstacle == &Structures.back()) { continue; }
        
        if (StructureCollision({ NewPos.x, yCoordTest.y }, Radius, Obstacle))
        {
            yCoordBlocked = true;
            break;
        }
    }

    if (!yCoordBlocked)
    {
        for (const auto& OtherTank : TankList)
        {
            if (OtherTank->GetID() == TankID || !OtherTank->IsAlive()) { continue; }
            
            const Play::Vector2D TankOffset = { NewPos.x - OtherTank->GetPosition().x, yCoordTest.y - OtherTank->GetPosition().y };
            float CombinedRadius = Radius + OtherTank->GetRadius();
            
            if ((TankOffset.x*TankOffset.x + TankOffset.y*TankOffset.y) <= CombinedRadius*CombinedRadius)
            {
                yCoordBlocked = true;
                break;
            }
        }
    }
    
    
    if (!yCoordBlocked)
    {
        NewPos.y += dy;
    }
    
    // Clamp Movement to Outer Wall
    const auto& OuterWall = Structures.back();
    NewPos.x = std::clamp(NewPos.x, OuterWall.BottomLeft.x + Radius, OuterWall.BottomLeft.x + OuterWall.Size.x - Radius);
    NewPos.y = std::clamp(NewPos.y, OuterWall.BottomLeft.y + Radius, OuterWall.BottomLeft.y + OuterWall.Size.y - Radius);

    Position = NewPos;
}

void Tank::Rotate( const float Angle)
{
	Rotation -= Angle;
}

void Tank::Shoot( bool bShouldShoot, const float ElapsedTime)
{
    if(bShouldShoot)
    {
        bCharging = true;
        CurrentChargeTime += ElapsedTime;
        CurrentChargeTime = std::min(CurrentChargeTime, MaxChargeTime);
    }
	else if (bCharging)
	{
		bCharging = false;
		const float Speed = BaseSpeed + CurrentChargeTime * ChargeSpeedMulti;
		float MaxDistance = BaseDistance + CurrentChargeTime * ChargeDistanceMulti;
		Play::Vector2D Velocity = { std::cos(Rotation) * Speed, std::sin(Rotation) * Speed };
		
        Bullet::CreateBullet(Position, Velocity, MaxDistance, TankID);

		CurrentChargeTime = 0.0f;
	}
}

void Tank::Update( const float ElapsedTime)
{
    if (!bAlive)
    {
        RespawnTimer -= ElapsedTime;
        
        if (RespawnTimer <= 0.0f)
        {

            Respawn(SpawnPositions[TankID]);
        }
        return;
    }

	// Draw charge indicator
	DrawChargeIndicator();
	Draw();
}

void Tank::DrawChargeIndicator() const
{
	if (!bCharging) return;

	float ChargeTimeNorm = CurrentChargeTime / MaxChargeTime;
	float Scale = std::min(ChargeTimeNorm * ChargeTimeNormMulti, 1.0f);
	float IndicatorLength = BaseDistance * Scale;

    Play::Vector2D Size = GetSize();
    float BarrelOffset = Size.y * 0.5f; 

	Play::Vector2D StartPos = {
		Position.x + std::cos(Rotation) * BarrelOffset,
		Position.y + std::sin(Rotation) * BarrelOffset
	};
	Play::Vector2D EndPos = { StartPos.x + std::cos(Rotation) * IndicatorLength,
							  StartPos.y + std::sin(Rotation) * IndicatorLength };
	Play::DrawLine(StartPos, EndPos, Play::cMagenta);
}

float Tank::GetRadius() const
{
    const auto Size = Play::Graphics::GetSpriteSize(SpriteId);
    return std::max(Size.x, Size.y) * 0.5f * TankScale;
}

Play::Vector2D Tank::GetSize() const
{
    auto Size = Play::Graphics::GetSpriteSize(SpriteId);
    return { Size.x * TankScale, Size.y * TankScale };
}

void Tank::TakeDamage(int Amount)
{
    if (!bAlive) return;

    Health -= Amount;
    if (onDamage_) onDamage_(Amount);

    if (Health <= 0)
    {
        bAlive = false;
        RespawnTimer = RespawnDelay;
        if (onDeath_) onDeath_();
    }
}

void Tank::Respawn(const Play::Vector2D& SpawnPos)
{
    Position = SpawnPos;
    Health = MaxHealth;
    bAlive = true;
    if (onRespawn_) onRespawn_();
}

int Tank::CreateTank(Play::Vector2D StartPosition, int Id)
{
    Tank* tank = new Tank(StartPosition, Id);
    TankList.push_back(tank);

    return Id;
}


void Tank::Draw() const
{
	// Draw tank body
    static const float RotationOffsets[] = {
        -Play::PLAY_PI / 2.0f,
        -Play::PLAY_PI / 2.0f,
        -Play::PLAY_PI / 2.0f,
        -Play::PLAY_PI / 2.0f
    };

    float RotationOffset = 0.0f;
    
    if (TankID >= 0 && TankID < 4)
    {
        RotationOffset = RotationOffsets[TankID];
    }

    // Draw tank sprite with rotation
    Play::DrawSpriteRotated(SpriteId, Position, 0, Rotation + RotationOffset, TankScale);

	// Draw charge indicator
	DrawChargeIndicator();
}
