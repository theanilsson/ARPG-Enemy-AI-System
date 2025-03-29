#pragma once
#include "../../Dependencies/Wwise/Wwise_Project/GeneratedSoundBanks/Wwise_IDs.h"
#include "../../Dependencies/Wwise/Wwise_SDK/include/AK/SpatialAudio/Common/AkSpatialAudio.h"
#include "../../Dependencies/Wwise/Wwise_SDK/include/AK/SoundEngine/Common/AkTypes.h"
#include <DreamEngine/graphics/AnimationPlayer.h>
#include <DreamEngine/graphics/sprite.h>
#include <DreamEngine/utilities/CountTimer.h>
#include <DreamEngine/math/Collider.h>
#include <DreamEngine/math/Vector3.h>
#include "GameObject.h"
#include "EnemyType.h"
#include "ParticleSystem3D.h"

namespace DreamEngine 
{
	class GraphicsEngine;
	class ModelInstance;
	class AnimatedModelInstance;
	struct Navmesh;
}

class Player;
class EnemyGroup;
class Enemy : public GameObject
{
public:
	// Init stage functions //
	Enemy() = delete;
	Enemy(const EnemyType& anEnemyType);
	void Init() override;
	void SetPlayer(std::shared_ptr<Player> aPlayer);
	void SetNavmesh(std::shared_ptr<DE::Navmesh> aNavmesh, std::shared_ptr<DE::Navmesh> aDetailedNavmesh);
	void SetEnemyGroup(std::shared_ptr<EnemyGroup> anEnemyGroup);
	virtual void SetAnimatedModel() = 0;
	void InitParticleSystems();
	std::vector<ParticleSystem3D*> GetParticleSystems();

	// Update stage functions //
	void Update(float aDeltaTime) override;
	void UpdateSoundEnginePosition();
	virtual void Alert();
	bool GetIsIdleDuringBoss();
	void SetIsIdleDuringBoss(bool anIsActive);
	const bool IsAlive() const;
	const DE::CylinderCollider<float>& GetSelectionCylinder() const;
	std::shared_ptr<DreamEngine::AnimatedModelInstance> GetAnimatedModelInstance() const;
	const eEnemyType GetEnemyType() const;
	const DE::Vector3f GetPositionTwoFramesAgo() const;
	void SetHasCollided();
	void TakeDamageFromCharge();

	// Render stage functions //
	void Render(DreamEngine::GraphicsEngine& aGraphicsEngine) override;
	void RenderToGBuffer(DreamEngine::GraphicsEngine& aGraphicsEngine) override;
	void RenderToGBufferWithoutShadow(DreamEngine::GraphicsEngine& aGraphicsEngine);
	void RenderHealthBar(DreamEngine::GraphicsEngine& aGraphicsEngine);

protected:
	// Update stage functions //
	const int GetMaxHealth() const;
	const int GetCurrentHealth() const;
	void SetCurrentHealth(const int aHealthAmount);
	void TakeDamage(const int& aDamageAmount);
	void Heal(const int& aHealingAmount);
	void HealToMax(); 
	virtual void Respawn();
	virtual void PlayIdleSound() = 0;
	void ChasePlayer();
	void TurnTowards(DE::Vector3f aPos);
	void MoveTowards(DE::Vector3f aPos);
	void MoveOnNavmesh(float aDeltaTime);
	void AddSeparation(float aDeltaTime, DE::Vector3f& anOutNextPosition, Enemy* anEnemy, bool& anOutSeparationAddedFlag);
	void AddPushbackFromCollision(GameObject* anOtherObject);
	virtual void SetCurrentToWalkOrIdleAnimation() {};
	virtual void RunAttackBehavior(float aDistToPlayer) = 0;
	virtual void CountDownCooldowns(float aDeltaTime) = 0;
	virtual void SetAnimationState(float aDeltaTime) = 0;


	// Behavior management variables //
	const EnemyType& myEnemyType;
	std::shared_ptr<Player> myPlayer;
	std::shared_ptr<EnemyGroup> myEnemyGroup;
	bool myIsAlertedByAlly = false;
	bool myIsIdleDuringBoss = false;
	bool myStartedIdleSound = false;

	// Movement variables //
	bool myCollided;
	const float myCountAsInCombatRange = 500.0f;
	const float mySeparationDistance = 50.0f;
	float myMoveSpeedModifier = 1.0f;
	DE::Vector3f myVelocity;
	DE::Vector3f myPositionLastFrame;
	DE::Vector3f myPositionTwoFramesAgo;
	DE::Vector3f mySpawnPos;

	// Pathfinding variables //
	CU::CountdownTimer myPathfindingTimer;
	std::vector<DE::Vector3f> myCurrentPath;
	std::shared_ptr<DreamEngine::Navmesh> myNavmesh;
	std::shared_ptr<DreamEngine::Navmesh> myDetailedNavmesh;
	float myPopPathPointsDistance = 10.0f;

	// Graphics variables //
	int myCurrentAnimationIndex = 0;
	std::shared_ptr<DE::AnimatedModelInstance> myAnimatedModelInstance;
	std::vector<DE::AnimationPlayer> myAnimations;
	DE::CylinderCollider<float> mySelectionCylinder;
	float myVerticalHpBarOffset;
	DreamEngine::Sprite3DInstanceData myHpBarInstance;
	DreamEngine::SpriteSharedData myHpBarData;
	DreamEngine::Vector2f myCurrentHpBarSize;
	DreamEngine::Vector2f myOriginalHpBarSize;
	ParticleSystem3D myBloodSplash;
	ParticleSystem3D myFloatingDamageNumbers;
	struct myFloatingDamageNumberData
	{
		int id;
		int damageAmount;
	};

	// Combat variables //
	CU::CountdownTimer myAttackTimer;
	int myAdditionalDamage = 0;
	bool myIsElite = false;
	CU::CountupTimer myTimeSinceLastTickFromAbilityThree;
	CU::CountdownTimer myTakeDamageFromAbilityThreeTimer;
	CU::CountdownTimer myTakeDamageFromHeavyAttackTimer;
	CU::CountdownTimer myTakeDamageFromAbilityOneTimer;
	CU::CountdownTimer myTakeDamageFromAbilityTwoTimer;
};