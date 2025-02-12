#include "Enemy.h"
#include "EnemyGroup.h"
#include "WeakEnemy.h"
#include "RangedEnemy.h"
#include "TankEnemy.h"
#include "Navmesh.h"
#include "Player.h"
#include "MainSingleton.h"
#include "HealthComponent.h"
#include <DreamEngine/graphics/GraphicsEngine.h>
#include <DreamEngine/graphics/SpriteDrawer.h>
#include <DreamEngine/graphics/ModelDrawer.h>
#include <DreamEngine/graphics/ModelInstance.h>
#include <DreamEngine/graphics/AnimatedModelInstance.h>
#include <DreamEngine/graphics/TextureManager.h>
#include <DreamEngine\utilities\UtilityFunctions.h>

Enemy::Enemy(const EnemyType& anEnemyType) : myEnemyType(anEnemyType) {}

void Enemy::Init()
{
	GameObject::Init();
	myTag = eTag::Enemy;

	AddComponent<HealthComponent>();
	auto* healthCmp = GetComponent<HealthComponent>();
	healthCmp->SetMaxHealth(myIsElite ? myEnemyType.maxHealth * 2 : myEnemyType.maxHealth);
	healthCmp->SetCurrentHealth(myIsElite ? myEnemyType.maxHealth * 2 : myEnemyType.maxHealth);

	myTransform.SetPosition(myTransform.GetPosition());
	myAnimatedModelInstance->SetLocation(myTransform.GetPosition());
	mySpawnPos = myTransform.GetPosition();
	myPositionLastFrame = myTransform.GetPosition();
	myPositionTwoFramesAgo = myTransform.GetPosition();

	myAttackTimer = CU::CountdownTimer(myEnemyType.attackCooldown);
	myPathfindingTimer = CU::CountdownTimer(UtilityFunctions::GetRandomFloat(1.0f, 2.0f));
	myTakeDamageFromHeavyAttackTimer = CU::CountdownTimer(0.25f);
	myTakeDamageFromAbilityOneTimer = CU::CountdownTimer(0.25f);
	myTakeDamageFromAbilityTwoTimer = CU::CountdownTimer(0.5f);
	myTimeSinceLastTickFromAbilityThree = CU::CountupTimer(FLT_MAX, 2.0f);

	myFloatingDamageNumbers.SetGameID(myID);
	myFloatingDamageNumbers.SubscribeToObserver();
	myFloatingDamageNumbers.SetParticleSystemAsFloatingDamageNumbers(true);

	myBloodSplash.SetGameID(myID);
	myBloodSplash.SubscribeToObserver();
	myBloodSplash.SetShouldSwapUV(true);
	myBloodSplash.SetWorldScale(50.f);
	myBloodSplash.SetShouldStopAfterSpriteSheet(true);

	auto& engine = *DreamEngine::Engine::GetInstance();
	myHpBarData.myTexture = engine.GetTextureManager().GetTexture(L"/2D/EnemyHP.png");
	myCurrentHpBarSize = {150.f,20.f};
	myOriginalHpBarSize = myCurrentHpBarSize; 
	myCurrentHpBarSize.x = myOriginalHpBarSize.x * (static_cast<float>(GetCurrentHealth()) / static_cast<float>(GetMaxHealth()));
	myHpBarInstance.myTransform.SetPosition(GetTransform()->GetPosition());
}

void Enemy::SetPlayer(std::shared_ptr<Player> aPlayer)
{
	myPlayer = aPlayer;
	AK::SoundEngine::SetListeners(myAkId, &myPlayer->GetAkId(), 1);
}

void Enemy::SetNavmesh(std::shared_ptr<DE::Navmesh> aNavmesh, std::shared_ptr<DE::Navmesh> aDetailedNavmesh)
{
	myNavmesh = aNavmesh;
	myDetailedNavmesh = aDetailedNavmesh;
}

void Enemy::SetEnemyGroup(std::shared_ptr<EnemyGroup> anEnemyGroup)
{
	myEnemyGroup = anEnemyGroup;
}

void Enemy::InitParticleSystems()
{
	myFloatingDamageNumbers.LoadParticleSystem("ParticleSystems/PS_FloatingDamageNumbers.json");
	myBloodSplash.LoadParticleSystem("ParticleSystems/PS_BloodSplash.json");
}

std::vector<ParticleSystem3D*> Enemy::GetParticleSystems()
{
	std::vector<ParticleSystem3D*> allParticleSystems;
	allParticleSystems.push_back(&myFloatingDamageNumbers);
	allParticleSystems.push_back(&myBloodSplash);
	return allParticleSystems;
}

void Enemy::Update(float aDeltaTime)
{
	GameObject::Update(aDeltaTime);
	myBloodSplash.SetWorldScale(200.f);
	myPositionTwoFramesAgo = myPositionLastFrame;
	myPositionLastFrame = myTransform.GetPosition();

	auto cam = MainSingleton::GetInstance()->GetActiveCamera(); 

	DreamEngine::Vector3f cameraPos = cam->GetTransform().GetPosition();
	DreamEngine::Vector3f cameraUp = cam->GetTransform().GetMatrix().GetUp();

	DreamEngine::Vector3f pos = GetTransform()->GetPosition();
	pos += cameraUp * myVerticalHpBarOffset + cam->GetTransform().GetMatrix().GetRight() * -1.0f * 75.0f + cam->GetTransform().GetMatrix().GetForward() * -1.0f * 100.0f;

	myHpBarInstance.myColor = myHpBarInstance.myColor;
	DreamEngine::Vector3f forwardDir = (pos - cameraPos).GetNormalized();
	DreamEngine::Vector3f rightDir = forwardDir.Cross(-1.f * cameraUp).GetNormalized();
	DreamEngine::Vector3f upDir = forwardDir.Cross(rightDir).GetNormalized();

	myHpBarInstance.myTransform.SetPosition(pos);
	myHpBarInstance.myTransform.SetForward(forwardDir);
	myHpBarInstance.myTransform.SetRight(myCurrentHpBarSize.x * rightDir); 
	myHpBarInstance.myTransform.SetUp(myCurrentHpBarSize.y *  upDir);

	myFloatingDamageNumbers.SetPos(myTransform.GetPosition() + DE::Vector3f{0,200,0});
	myBloodSplash.SetPos(myTransform.GetPosition() + DE::Vector3f{85,150,-85});
}

void Enemy::UpdateSoundEnginePosition()
{
	auto matrix = myTransform.GetMatrix();
	AkSoundPosition aPos;
	AkVector forwardAK;
	forwardAK.X = matrix.GetForward().GetNormalized().x;
	forwardAK.Y = matrix.GetForward().GetNormalized().y;
	forwardAK.Z = matrix.GetForward().GetNormalized().z;
	AkVector upAK;
	upAK.X = matrix.GetUp().GetNormalized().x;
	upAK.Y = matrix.GetUp().GetNormalized().y;
	upAK.Z = matrix.GetUp().GetNormalized().z;
	aPos.SetOrientation(forwardAK, upAK);
	aPos.SetPosition(myTransform.GetPosition().x / 100.0f, myTransform.GetPosition().y / 100.0f, myTransform.GetPosition().z / 100.0f);
	AK::SoundEngine::SetPosition(myAkId, aPos);
}

void Enemy::Alert()
{
	if (!myIsAlertedByAlly)
	{
		if (myEnemyType.enemyType == eEnemyType::Tank)
		{
			AK::SoundEngine::PostEvent("Play_EnemyTankAggro", myAkId);
		}
		else if (myEnemyType.enemyType == eEnemyType::Ranged)
		{
			AK::SoundEngine::PostEvent("Play_EnemyRangedAggro", myAkId);
		}
	}
	myIsAlertedByAlly = true;
}

bool Enemy::GetIsIdleDuringBoss()
{
	return myIsIdleDuringBoss;
}

void Enemy::SetIsIdleDuringBoss(bool anIsActive)
{
	myIsIdleDuringBoss = anIsActive;
}

const bool Enemy::IsAlive() const
{
	return GetComponent<HealthComponent>()->IsAlive();
}

const DE::CylinderCollider<float>& Enemy::GetSelectionCylinder() const
{
	return mySelectionCylinder;
}

std::shared_ptr<DreamEngine::AnimatedModelInstance> Enemy::GetAnimatedModelInstance() const
{
	return myAnimatedModelInstance;
}

const eEnemyType Enemy::GetEnemyType() const
{
	return myEnemyType.enemyType;
}

const DE::Vector3f Enemy::GetPositionTwoFramesAgo() const
{
	return myPositionTwoFramesAgo;
}

void Enemy::SetHasCollided()
{
	myCollided = true;
}

void Enemy::TakeDamageFromCharge()
{
	if (myTakeDamageFromAbilityTwoTimer.IsDone())
	{
		TakeDamage(15);
		myPlayer->AddResource(5);
		myTakeDamageFromAbilityTwoTimer.Reset();
	}
}

const int Enemy::GetMaxHealth() const
{
	return GetComponent<HealthComponent>()->GetMaxHealth();
}

const int Enemy::GetCurrentHealth() const
{
	return GetComponent<HealthComponent>()->GetCurrentHealth();
}

void Enemy::SetCurrentHealth(const int aHealthAmount)
{
	GetComponent<HealthComponent>()->SetCurrentHealth(aHealthAmount);
}

void Enemy::TakeDamage(const int& aDamageAmount)
{
	if (IsAlive())
	{
		if (myEnemyType.enemyType == eEnemyType::Weak)
		{
			AK::SoundEngine::PostEvent("Play_EnemyPopcornHurt", myAkId);
		}
		else if (myEnemyType.enemyType == eEnemyType::Tank)
		{
			AK::SoundEngine::PostEvent("Play_EnemyTankHurt", myAkId);
		}
		else
		{
			AK::SoundEngine::PostEvent("Play_EnemyRangedHurt", myAkId);
		}
		Message message;

		myFloatingDamageNumberData data;
		data.id = myID;
		data.damageAmount = aDamageAmount;
		message.messageData = &data;
		message.messageType = eMessageType::SpawnParticle;
		MainSingleton::GetInstance()->GetPostMaster().TriggerMessage(message);
		SetCurrentHealth(UtilityFunctions::Max(GetCurrentHealth() - aDamageAmount, 0));
		myCurrentHpBarSize.x = myOriginalHpBarSize.x * (static_cast<float>(GetCurrentHealth()) / static_cast<float>(GetMaxHealth()));
		if (!IsAlive())
		{
			if (myEnemyType.enemyType == eEnemyType::Weak)
			{
				AK::SoundEngine::PostEvent("Play_EnemyPopcornDeath", myAkId);
			}
			else if (myEnemyType.enemyType == eEnemyType::Tank)
			{
				AK::SoundEngine::PostEvent("Play_EnemyTankDeath", myAkId);
			}
			else
			{
				AK::SoundEngine::PostEvent("Play_EnemyRangedDeath", myAkId);
			}
		}
	}
}

void Enemy::Heal(const int& aHealingAmount)
{
	SetCurrentHealth(UtilityFunctions::Min(GetCurrentHealth() + aHealingAmount, GetMaxHealth()));
}

void Enemy::HealToMax()
{
	SetCurrentHealth(GetMaxHealth());
}

void Enemy::Respawn()
{
	HealToMax();
	myTransform.SetPosition(mySpawnPos);
	myAnimatedModelInstance->SetLocation(myTransform.GetPosition());
	myCurrentAnimationIndex = static_cast<int>(eWeakEnemyAnimations::Spawn);
	myAnimations[myCurrentAnimationIndex].Stop();
	myAnimations[myCurrentAnimationIndex].Update(UtilityFunctions::GetRandomFloat(0.0f, 0.5f));
	myAnimations[static_cast<int>(eWeakEnemyAnimations::Death)].Stop();
	myIsAlertedByAlly = false;
	myEnemyGroup->ResetAlert();
	myPathfindingTimer.Zeroize();
	AK::SoundEngine::StopAll(myAkId);
	myStartedIdleSound = false;
	myCurrentHpBarSize = myOriginalHpBarSize;
	if (myEnemyGroup->IsBubbleGroup())
	{
		myEnemyGroup->SetActive(false);
	}
}

void Enemy::ChasePlayer()
{
	if (myCurrentPath.empty() || myPathfindingTimer.IsDone())
	{
		auto nextPath = DE::Pathfinding::FindShortestPath(myTransform.GetPosition(), myPlayer->GetPosition(), myNavmesh);
		if (nextPath.size() > 0)
		{
			myCurrentPath = nextPath;
			myPathfindingTimer.Reset();
		}
	}

	if (!myCurrentPath.empty() && (DE::Vector3f(myTransform.GetPosition().x, 0.0f, myTransform.GetPosition().z) - DE::Vector3f(myCurrentPath.back().x, 0.0f, myCurrentPath.back().z)).LengthSqr() < myPopPathPointsDistance * myPopPathPointsDistance)
	{
		myCurrentPath.pop_back();
	}

	if (!myCurrentPath.empty())
	{
		TurnTowards(myCurrentPath.back());
		if (myCollided == false)
		{
			MoveTowards(myCurrentPath.back());
		}
	}
}

void Enemy::TurnTowards(DE::Vector3f aPos)
{
	DE::Rotator dirToPos = aPos - myTransform.GetPosition();
	auto calculatedYawRotation = UtilityFunctions::CalculateYRotationBetweenDirections(myTransform.GetMatrix().GetForward().x, myTransform.GetMatrix().GetForward().z, dirToPos.GetNormalized().x, dirToPos.GetNormalized().z);
	myTransform.AddRotation({0.0f, calculatedYawRotation, 0.0f });
	myAnimatedModelInstance->SetRotation(myTransform.GetRotation());
}

void Enemy::MoveTowards(DE::Vector3f aPos)
{
	auto dirToPos = aPos - myTransform.GetPosition();
	dirToPos.y = 0;
	myVelocity = dirToPos.GetNormalized() * (myEnemyType.baseSpeed * myMoveSpeedModifier);
}

void Enemy::MoveOnNavmesh(float aDeltaTime)
{
	DE::Vector3f nextPosition = myTransform.GetPosition() + myVelocity * aDeltaTime;
	
	bool addedSeparation = false;
	for (auto weakEnemy : myEnemyGroup->GetWeakEnemies())
	{
		if(myID == weakEnemy->GetID() || addedSeparation)
		{
			continue;
		}
		AddSeparation(aDeltaTime, nextPosition, weakEnemy.get(), addedSeparation);
	}
	for (auto rangedEnemy : myEnemyGroup->GetRangedEnemies())
	{
		if(myID == rangedEnemy->GetID() || addedSeparation)
		{
			continue;
		}
		AddSeparation(aDeltaTime, nextPosition, rangedEnemy.get(), addedSeparation);
	}
	for (auto tankEnemy : myEnemyGroup->GetTankEnemies())
	{
		if(myID == tankEnemy->GetID() || addedSeparation)
		{
			continue;
		}
		AddSeparation(aDeltaTime, nextPosition, tankEnemy.get(), addedSeparation);
	}

	int nodeIndex = DE::Pathfinding::GetNodeIndexFromPointDetailed(nextPosition, myDetailedNavmesh);
	if (nodeIndex > -1)
	{
		nextPosition.y = DE::Pathfinding::GetYPosFromPointDetailed(nextPosition, nodeIndex, myDetailedNavmesh);
	}
	else
	{
		nextPosition = DE::Pathfinding::GetClosestPointOnNavmesh(nextPosition, myNavmesh);
	}
	if (MainSingleton::GetInstance()->GetCollisionManager().CheckIfPosTooCloseToBubbleFence(nextPosition)) 
	{
		nextPosition = myPositionLastFrame;
	}

	myPositionLastFrame = myTransform.GetPosition();
	myTransform.SetPosition(nextPosition);
	UpdateSoundEnginePosition();
}

void Enemy::AddSeparation(float aDeltaTime, DE::Vector3f& anOutNextPosition, Enemy* anEnemy, bool& anOutSeparationAddedFlag)
{
	if ((anOutNextPosition - anEnemy->GetTransform()->GetPosition()).LengthSqr() < mySeparationDistance * mySeparationDistance)
	{
		DE::Vector3f dirToNext = (anOutNextPosition - myTransform.GetPosition()).GetNormalized();
		if (DE::Pathfinding::IsToTheLeftOfLine(myTransform.GetPosition(), anOutNextPosition, anEnemy->GetTransform()->GetPosition()))
		{
			anOutNextPosition -= myVelocity * aDeltaTime;
			anOutNextPosition += DE::Vector3f(dirToNext.y, -dirToNext.x) * (myEnemyType.baseSpeed * myMoveSpeedModifier) * aDeltaTime;
		}
		else
		{
			anOutNextPosition -= myVelocity * aDeltaTime;
			anOutNextPosition += DE::Vector3f(-dirToNext.y, dirToNext.x) * (myEnemyType.baseSpeed * myMoveSpeedModifier) * aDeltaTime;
		}
		anOutSeparationAddedFlag = true;
	}
}

void Enemy::AddPushbackFromCollision(GameObject* anOtherObject)
{
	DE::Vector3f dirAwayFromCollision = myTransform.GetPosition() - anOtherObject->GetTransform()->GetPosition();
	DE::Vector3f pushedAwayPos = myTransform.GetPosition() + (dirAwayFromCollision * 0.01f);
	pushedAwayPos.y = myTransform.GetPosition().y;
	myTransform.SetPosition(pushedAwayPos);
	myCollided = true;
}

void Enemy::Render(DreamEngine::GraphicsEngine& aGraphicsEngine)
{
	if (myEnemyGroup->IsActive() && IsAlive())
	{
		aGraphicsEngine.GetModelDrawer().DrawPbr(*myAnimatedModelInstance.get());
	}
}

void Enemy::RenderToGBuffer(DreamEngine::GraphicsEngine& aGraphicsEngine)
{
	if (myEnemyGroup->IsActive())
	{
		aGraphicsEngine.GetModelDrawer().DrawGBCalcAnimated(*myAnimatedModelInstance.get());
	}
}

void Enemy::RenderToGBufferWithoutShadow(DreamEngine::GraphicsEngine& aGraphicsEngine)
{
	if (myIsAlertedByAlly == true)
	{
		aGraphicsEngine.GetSpriteDrawer().DrawToGBuffer(myHpBarData, myHpBarInstance);
	}
}

void Enemy::RenderHealthBar(DreamEngine::GraphicsEngine& aGraphicsEngine)
{
	if (myIsAlertedByAlly == true)
	{
		aGraphicsEngine.GetSpriteDrawer().Draw(myHpBarData, myHpBarInstance);
	}
}