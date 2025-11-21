#pragma once
#ifndef BULLET_H
#define BULLET_H

#include "Play.h"
#include <vector>

class Tank;

class Bullet
{
public:
	void Update(float ElapsedTime);
	void Draw() const;
	bool IsAlive() const;

	Play::Vector2D GetPosition() const { return Pos; }
	float GetRadius() const { return Radius; }
	int GetOwnerId() const { return OwnerId; }

	static void CreateBullet(const Play::Vector2D& StartPos, const Play::Vector2D& Velocity, const float MaxDistance, const int OwnerId);
	static std::vector<Bullet*>& GetAllBullets() { return BulletList; }

private:
	Bullet(const Play::Vector2D& StartPos, const Play::Vector2D& Velocity, float MaxDistance, int OwnerId);
	
	Play::Vector2D Pos;
	Play::Vector2D Velocity;
	float DistanceTraveled = 0.0f;
	float MaxDistance;
	float Radius = 4.0f;
	int OwnerId;
    int SpriteId;

    bool bHasLeftOuterWall = false;

	bool CheckTankCollision(const Tank* Tank) const;
	bool CheckBulletCollision(const Bullet* Other) const;

	// Static vector of all Bullets
	static std::vector<Bullet*> BulletList;
};

#endif
