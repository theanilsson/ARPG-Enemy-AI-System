#pragma once
#include "Enemy.h"
#include "Observer.h"

namespace DreamEngine
{
	class GraphicsEngine;
	class ModelInstance;
}

enum class eRangedEnemyAnimations
{
	Walk,
	Ranged,
	Death,
	Spawn,
	DefaultTypeAndCount
};

class Player;
class ProjectilePool;
class RangedEnemy : public Enemy, Observer
{
public:
	// Init stage functions //
	RangedEnemy() = delete;
	RangedEnemy(const EnemyType& anEnemyType);
	RangedEnemy(const RangedEnemy& anOtherEnemy) = delete;
	RangedEnemy& operator=(const RangedEnemy& anOtherEnemy) = delete;
	RangedEnemy(RangedEnemy&& anOtherEnemy) = default;
	RangedEnemy& operator=(RangedEnemy&& anOtherEnemy) = default;
	~RangedEnemy();
	void Init() override;
	void SetAnimatedModel() override;

	// Update stage functions //
	void Update(float aDeltaTime) override;
	void OnCollision(GameObject* anObject, eCollisionLayer aCollisionLayer) override;
	void Receive(const Message& aMessage) override;

	// Render stage functions //
	void RenderProjectileColliders();
	void RenderProjectiles(DreamEngine::GraphicsEngine& aGraphicsEngine);
	
private:
	// Update stage functions //
	void Respawn() override;
	void PlayIdleSound() override;
	void CheckIfAlone();
	void RunAway();
	void LaunchProjectile();
	void RunAttackBehavior(float aDistToPlayer) override;
	void CountDownCooldowns(float aDeltaTime) override;
	void SetAnimationState(float aDeltaTime) override;


	ProjectilePool* myProjectilePool = nullptr;
	CU::CountdownTimer myAttackWhileRunningAwayTimer;
	bool myIsAlone = false;
	bool myAttackedThisCasting = false;
};