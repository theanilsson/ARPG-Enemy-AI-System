#pragma once
#include "Enemy.h"
#include "Observer.h"

namespace DreamEngine
{
	class GraphicsEngine;
	class ModelInstance;
}

enum class eWeakEnemyAnimations
{
	Idle,
	Walk,
	AttackVariationOne,
	AttackVariationTwo,
	Death,
	Spawn,
	DefaultTypeAndCount
};

class Player;
class WeakEnemy : public Enemy, Observer
{
public:
	// Init stage functions //
	WeakEnemy() = delete;
	WeakEnemy(const EnemyType& anEnemyType);
	WeakEnemy(const WeakEnemy& anOtherEnemy) = delete;
	WeakEnemy& operator=(const WeakEnemy& anOtherEnemy) = delete;
	WeakEnemy(WeakEnemy&& anOtherEnemy) = default;
	WeakEnemy& operator=(WeakEnemy&& anOtherEnemy) = default;
	~WeakEnemy();
	void Init() override;
	void SetAnimatedModel() override;

	// Update stage functions //
	void Update(float aDeltaTime) override;
	void Alert() override;
	void OnCollision(GameObject* anObject, eCollisionLayer aCollisionLayer) override;
	void Receive(const Message& aMessage) override;
private:
	void PlayIdleSound() override;
	void ApplyTankBuff();
	void SetCurrentToWalkOrIdleAnimation() override;
	void RunAttackBehavior(float aDistToPlayer) override;
	void CountDownCooldowns(float aDeltaTime) override;
	void SetAnimationState(float aDeltaTime) override;

	
	CU::CountupTimer myAttackSoundDelayTimer;
	bool delayTimerWasDoneLastFrame = true;
	bool myUseAttackVarOne = true;
};