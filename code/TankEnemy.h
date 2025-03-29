#pragma once
#include "Enemy.h"
#include "Observer.h"

namespace DreamEngine
{
	class GraphicsEngine;
	class ModelInstance;
}

enum class eTankEnemyAnimations
{
	Idle,
	Walk,
	Attack,
	Charge,
	Death,
	Spawn,
	DefaultTypeAndCount
};

class Player;
class RangedEnemy;
class TankEnemy : public Enemy, Observer
{
public:
	// Init stage functions //
	TankEnemy() = delete;
	TankEnemy(const EnemyType& anEnemyType);
	~TankEnemy();
	void Init() override;
	void SetAnimatedModel() override;

	// Update stage functions //
	void Update(float aDeltaTime) override;
	void OnCollision(GameObject* anObject, eCollisionLayer aCollisionLayer) override;
	void Receive(const Message& aMessage) override;
private:
	void Respawn() override;
	void PlayIdleSound() override;
	void PerformProtectionCharge(std::shared_ptr<RangedEnemy> aRangedEnemy);
	void SetCurrentToWalkOrIdleAnimation() override;
	void RunAttackBehavior(float aDistToPlayer) override;
	void CountDownCooldowns(float aDeltaTime) override;
	void SetAnimationState(float aDeltaTime) override;


	CU::CountdownTimer myProtectionChargeTimer;
	DE::Vector3f myChargeTarget;
};