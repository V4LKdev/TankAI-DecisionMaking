#pragma once

#ifndef TANK_H
#define TANK_H

#include "Play.h"
#include <functional>

class Tank
{
public:
	using DamageCB = std::function<void(int)>;
	using DeathCB = std::function<void()>;
	using RespawnCB = std::function<void()>;

	void Move(float Amount);
	void Rotate(float Angle);
	void Shoot(const bool bShouldShoot, const float ElapsedTime);
	void Update(float ElapsedTime);
	void DrawChargeIndicator() const;

	// Getters
	Play::Vector2D GetPosition() const { return Position; }
	float GetRotation() const { return Rotation; }
	int GetID() const { return TankID; }
	float GetRadius() const;
	Play::Vector2D GetSize() const;
	int GetHealth() const { return Health; }
	bool IsAlive() const { return bAlive; }

    void TakeDamage(int Amount);
    void Respawn(const Play::Vector2D& SpawnPos);

	void SetOnDamage(const DamageCB& cb) { onDamage_ = cb; }
	void SetOnDeath(const DeathCB& cb) { onDeath_ = cb; }
	void SetOnRespawn(const RespawnCB& cb) { onRespawn_ = cb; }

	static std::vector<Tank*>& GetAllTanks() { return TankList; }
	static int CreateTank(Play::Vector2D StartPosition, int Id);

private:
	Tank(Play::Vector2D StartPos, int Id);
	
	Play::Vector2D Position;
	float Rotation;
	int TankID;
    int SpriteId;
	bool bCharging = false;
	float CurrentChargeTime = 0.0f;
    float TankScale = 0.5f;

    int MaxHealth = 3;
    int Health = MaxHealth;
    bool bAlive = true;

    float RespawnDelay = 3.0f;
    float RespawnTimer = 0.0f;

	// Bullet variables
	float MaxChargeTime = 2.0f;
	float BaseSpeed = 350.0f;
	float ChargeSpeedMulti = 50.0f;
	float BaseDistance = 100.0f;
	float ChargeDistanceMulti = 150.0f;
	float ChargeTimeNormMulti = 3.0f;

	void Draw() const;

	// Callbacks
	DamageCB onDamage_{};
	DeathCB onDeath_{};
	RespawnCB onRespawn_{};

	// Static list of all Tanks
	static std::vector<Tank*> TankList;
};

#endif
