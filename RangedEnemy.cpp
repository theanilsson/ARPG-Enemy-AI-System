#include "RangedEnemy.h"
#include "WeakEnemy.h"
#include "TankEnemy.h"
#include "EnemyGroup.h"
#include "Navmesh.h"
#include "ProjectilePool.h"
#include "Player.h"
#include "BreakableObject.h"
#include "MainSingleton.h"
#include "Message.h"
#include "ColliderComponent.h"
#include <DreamEngine\math\Collider.h>
#include <DreamEngine\graphics\ModelFactory.h>
#include <DreamEngine\graphics\ModelInstance.h>
#include <DreamEngine\graphics\AnimatedModelInstance.h>
#include <DreamEngine\windows\settings.h>
#include <DreamEngine\utilities\StringCast.h>

RangedEnemy::RangedEnemy(const EnemyType& anEnemyType) : Enemy(anEnemyType){}

RangedEnemy::~RangedEnemy()
{
	MainSingleton::GetInstance()->GetCollisionManager().DeregisterObject(this, eCollisionLayer::PlayerAttacks);
	MainSingleton::GetInstance()->GetCollisionManager().DeregisterObject(this, eCollisionLayer::Movement);
	MainSingleton::GetInstance()->GetPostMaster().Unsubscribe(eMessageType::PlayerRespawned, this);
	if (myProjectilePool != nullptr)
	{
		delete myProjectilePool;
	}
}

void RangedEnemy::Init()
{
	SetAnimatedModel();
	
	Enemy::Init();
	myVerticalHpBarOffset = 200.f;

	AddComponent<ColliderComponent>();
	auto* colliderCmp = GetComponent<ColliderComponent>();
	colliderCmp->SetCollider(std::make_shared<DE::SphereCollider<float>>(DE::SphereCollider<float>(&myTransform, {0.0f, 50.0f, 0.0f}, 30.0f)));
	MainSingleton::GetInstance()->GetCollisionManager().RegisterObject(this, eCollisionLayer::PlayerAttacks);
	MainSingleton::GetInstance()->GetCollisionManager().RegisterObject(this, eCollisionLayer::Movement);
	MainSingleton::GetInstance()->GetPostMaster().Subscribe(eMessageType::PlayerRespawned, this);

	myProjectilePool = new ProjectilePool(10);
	myAttackWhileRunningAwayTimer = CU::CountdownTimer(4.0f);

	DE::Vector3f selectionOffset(0.0f, 50.0f, 0.0f);
	mySelectionCylinder = DE::CylinderCollider<float>(&myTransform, selectionOffset, 70.0f, 100.0f);

	AK::SoundEngine::RegisterGameObj(myAkId, "RangedEnemy");
	myCurrentAnimationIndex = static_cast<int>(eRangedEnemyAnimations::Spawn);
	myAnimations[myCurrentAnimationIndex].Update(UtilityFunctions::GetRandomFloat(0.0f, 0.5f));
	UpdateSoundEnginePosition();
}

void RangedEnemy::SetAnimatedModel()
{
	DE::ModelFactory& modelFactory = DE::ModelFactory::GetInstance();
	std::wstring resolved_path = DE::Settings::ResolveAssetPathW(L"3D/SK_CH_EnemyClergy.fbx");
	myAnimatedModelInstance = std::make_shared<DE::AnimatedModelInstance>(modelFactory.GetAnimatedModelInstance(resolved_path));

	myAnimations.reserve(4);

	resolved_path = string_cast<std::wstring>((std::string)"3D/A_CH_EnemyClergy_Walk.fbx");
	myAnimations.push_back(modelFactory.GetAnimationPlayer(resolved_path, myAnimatedModelInstance->GetModel()));
	myAnimations.back().SetIsLooping(true);

	resolved_path = string_cast<std::wstring>((std::string)"3D/A_CH_EnemyClergy_LightAttack.fbx");
	myAnimations.push_back(modelFactory.GetAnimationPlayer(resolved_path, myAnimatedModelInstance->GetModel()));
	myAnimations.back().SetIsLooping(false);

	resolved_path = string_cast<std::wstring>((std::string)"3D/A_CH_EnemyClergy_Death.fbx");
	myAnimations.push_back(modelFactory.GetAnimationPlayer(resolved_path, myAnimatedModelInstance->GetModel()));
	myAnimations.back().SetIsLooping(false);

	resolved_path = string_cast<std::wstring>((std::string)"3D/A_CH_EnemyClergy_Spawn.fbx");
	myAnimations.push_back(modelFactory.GetAnimationPlayer(resolved_path, myAnimatedModelInstance->GetModel()));
	myAnimations.back().SetIsLooping(false);
}

void RangedEnemy::Update(float aDeltaTime)
{
	if (!myEnemyGroup->IsActive())
	{
		return;
	}

	Enemy::Update(aDeltaTime);

	if (IsAlive() && myPlayer->IsAlive() && myCurrentAnimationIndex != static_cast<int>(eRangedEnemyAnimations::Spawn))
	{
		myVelocity = { 0.0f, 0.0f, 0.0f };

		float sqrDistToPlayer = (myTransform.GetPosition() - myPlayer->GetTransform()->GetPosition()).LengthSqr();
		if (!myStartedIdleSound && myCurrentAnimationIndex == static_cast<int>(eWeakEnemyAnimations::Walk) && myAnimations[myCurrentAnimationIndex].GetFrame() == 15)
		{
			PlayIdleSound();
		}
		if (sqrDistToPlayer < myEnemyGroup->GetDetectionRange() * myEnemyGroup->GetDetectionRange() || myIsAlertedByAlly)
		{
			myEnemyGroup->AlertAll();
			
			if (!myIsAlone)
			{
				CheckIfAlone();
			}

			if (myIsAlone && !myAttackWhileRunningAwayTimer.IsDone() && sqrDistToPlayer < myEnemyType.attackRange * myEnemyType.attackRange && myCurrentAnimationIndex != static_cast<int>(eRangedEnemyAnimations::Ranged))
			{
				RunAway();
			}
			else if (myIsAlone)
			{
				TurnTowards(myPlayer->GetTransform()->GetPosition());
			}
			else if (sqrDistToPlayer > myEnemyType.attackRange * myEnemyType.attackRange)
			{
				ChasePlayer();
			}
			else
			{
				myCurrentPath.clear();
			}
		}

		// Set player as being in combat if close enough
		if (sqrDistToPlayer < myCountAsInCombatRange * myCountAsInCombatRange)
		{
			myPlayer->ResetCombatTimer();
		}

		MoveOnNavmesh(aDeltaTime);
		myAnimatedModelInstance->SetLocation(myTransform.GetPosition());
		UpdateSoundEnginePosition();
		RunAttackBehavior(sqrDistToPlayer);

		if (myPositionLastFrame != myTransform.GetPosition() && myCurrentAnimationIndex == static_cast<int>(eTankEnemyAnimations::Walk) && myAnimations[myCurrentAnimationIndex].GetFrame() == 10)
		{
			AK::SoundEngine::PostEvent("Play_EnemyRangedMovement", myAkId);
		}
		if (myCurrentAnimationIndex == static_cast<int>(eRangedEnemyAnimations::Ranged) && myAnimations[myCurrentAnimationIndex].GetTime() > 1.0f && !myAttackedThisCasting)
		{
			LaunchProjectile();
		}
	}
	else if (myCurrentAnimationIndex == static_cast<int>(eRangedEnemyAnimations::Spawn) && myAnimations[myCurrentAnimationIndex].GetFrame() == 15)
	{
		AK::SoundEngine::PostEvent("Play_WorldGroundCrack", myAkId);
	}

	if (myProjectilePool != nullptr)
	{
		myProjectilePool->Update(aDeltaTime);
	}

	myPositionTwoFramesAgo = myPositionLastFrame;
	myPositionLastFrame = myTransform.GetPosition();
	myCollided = false;
	SetAnimationState(aDeltaTime);
	if (IsAlive())
	{
		CountDownCooldowns(aDeltaTime);
	}
}

void RangedEnemy::OnCollision(GameObject* anObject, eCollisionLayer aCollisionLayer)
{
	if (myIsIdleDuringBoss) return;

	if (aCollisionLayer == eCollisionLayer::Movement)
	{
		if (anObject->GetTag() == eTag::AbilityShrine)
		{
			AddPushbackFromCollision(anObject);
		}
		else if (anObject->GetTag() == eTag::BreakableObject && static_cast<BreakableObject*>(anObject)->IsActive() == true)
		{
			AddPushbackFromCollision(anObject);
		}
	}
	else if (aCollisionLayer == eCollisionLayer::PlayerAttacks)
	{
		if (IsAlive() && IsActive())
		{
			Attack* attack = static_cast<Attack*>(anObject);
			switch (attack->GetAttackType())
			{
			case eAttackType::PlayerLightAttack:
			{
				AK::SoundEngine::PostEvent("Play_PlayerSwordAttackLightHit", myAkId);
				TakeDamage(attack->GetDamage());
				myPlayer->AddResource(5); // TODO adjust resource generated
				break;
			}
			case eAttackType::PlayerAbility1:
			{
				if (myTakeDamageFromAbilityOneTimer.IsDone())
				{
					TakeDamage(attack->GetDamage());
					myTakeDamageFromAbilityOneTimer.Reset();
				}
				break;
			}
			case eAttackType::PlayerHeavyAttack:
			{
				if (myTakeDamageFromHeavyAttackTimer.IsDone())
				{
					AK::SoundEngine::PostEvent("Play_PlayerSwordAttackHeavyHit", myAkId);
					TakeDamage(attack->GetDamage());
					myTakeDamageFromHeavyAttackTimer.Reset();
				}
				break;
			}
			case eAttackType::PlayerAbility3:
			{
				if (myTimeSinceLastTickFromAbilityThree.GetCurrentValue() > myTakeDamageFromAbilityThreeTimer.GetResetValue() * 2.0f)
				{
					TakeDamage(attack->GetDamage());
					myTakeDamageFromAbilityThreeTimer.Reset();
					myTimeSinceLastTickFromAbilityThree.Reset();
				}
				else if (myTakeDamageFromAbilityThreeTimer.IsDone())
				{
					if (attack->GetDamage() <= 25)
					{
						TakeDamage(5);
					}
					else
					{
						TakeDamage(10);
					}
					myTakeDamageFromAbilityThreeTimer.Reset();
					myTimeSinceLastTickFromAbilityThree.Reset();
				}
				break;
			}
			}
		}
	}
}

void RangedEnemy::Receive(const Message& aMessage)
{
	switch (aMessage.messageType)
	{
		case eMessageType::PlayerRespawned:
		{
			Respawn();
			break;
		}
	}
}

void RangedEnemy::Respawn()
{
	HealToMax();
	myTransform.SetPosition(mySpawnPos);
	myAnimatedModelInstance->SetLocation(mySpawnPos);
	myCurrentAnimationIndex = static_cast<int>(eRangedEnemyAnimations::Spawn);
	myAnimations[myCurrentAnimationIndex].Stop();
	myAnimations[myCurrentAnimationIndex].Update(UtilityFunctions::GetRandomFloat(0.0f, 0.5f));
	myAnimations[static_cast<int>(eRangedEnemyAnimations::Death)].Stop();
	myIsAlertedByAlly = false;
	myEnemyGroup->ResetAlert();
	myIsAlone = false;
	myPathfindingTimer.Zeroize();
	myAttackTimer.Zeroize();
	myAttackWhileRunningAwayTimer.Reset();
	AK::SoundEngine::StopAll(myAkId);
	myStartedIdleSound = false;
	myCurrentHpBarSize = myOriginalHpBarSize;
	if (myEnemyGroup->IsBubbleGroup())
	{
		myEnemyGroup->SetActive(false);
	}
}

void RangedEnemy::PlayIdleSound()
{
	AK::SoundEngine::PostEvent("Play_EnemyRangedIdle", myAkId);
	if (myIsAlertedByAlly)
	{
		AK::SoundEngine::StopAll(myAkId);
	}
	myStartedIdleSound = true;
}

void RangedEnemy::CheckIfAlone()
{
	bool isAlone = true;
	for (auto weakEnemy : myEnemyGroup->GetWeakEnemies())
	{
		if (weakEnemy->IsAlive())
		{
			isAlone = false;
		}
	}
	for (auto tankEnemy : myEnemyGroup->GetTankEnemies())
	{
		if (tankEnemy->IsAlive())
		{
			isAlone = false;
		}
	}
	for (auto rangedEnemy : myEnemyGroup->GetRangedEnemies())
	{
		if (rangedEnemy->GetID() != myID && rangedEnemy->IsAlive())
		{
			isAlone = false;
		}
	}
	if (isAlone) 
	{
		myAttackWhileRunningAwayTimer.Reset();
		myMoveSpeedModifier = 1.5f;
	}
	myIsAlone = isAlone;
}

void RangedEnemy::RunAway()
{
	DE::Vector3f runAwayPos = myTransform.GetPosition() + DE::Vector3f(myTransform.GetPosition() - myPlayer->GetPosition()).GetNormalized();
	TurnTowards(runAwayPos);
	if (myCollided == false)
	{
		MoveTowards(runAwayPos);
	}
}

void RangedEnemy::LaunchProjectile()
{
	myAttackedThisCasting = true;

	DE::Vector3f dirToPlayer = DE::Vector3f(DE::Vector3f(myPlayer->GetTransform()->GetPosition().x, myPlayer->GetTransform()->GetPosition().y, myPlayer->GetTransform()->GetPosition().z) - myTransform.GetPosition());
	myProjectilePool->GetProjectile(DE::Vector3f(myTransform.GetPosition().x, myTransform.GetPosition().y + 50.0f, myTransform.GetPosition().z), dirToPlayer.GetNormalized(), myPlayer->GetTransform());
	AK::SoundEngine::PostEvent("Play_EnemyRangedAttack", myAkId);
}

void RangedEnemy::RunAttackBehavior(float aSqrDistToPlayer)
{
	if ((aSqrDistToPlayer <= myEnemyType.attackRange * myEnemyType.attackRange && myAttackTimer.IsDone()) || (myIsAlone && myAttackWhileRunningAwayTimer.IsDone() && aSqrDistToPlayer <= myEnemyGroup->GetDetectionRange() * myEnemyGroup->GetDetectionRange()))
	{
		TurnTowards(myPlayer->GetPosition());
		myAttackTimer.Reset();
		myAttackWhileRunningAwayTimer.Reset();
		myCurrentAnimationIndex = static_cast<int>(eRangedEnemyAnimations::Ranged);
		myAttackedThisCasting = false;
		myAnimations[myCurrentAnimationIndex].Stop();
	}
}

void RangedEnemy::CountDownCooldowns(float aDeltaTime)
{
	myAttackTimer.Update(aDeltaTime);
	myTakeDamageFromAbilityThreeTimer.Update(aDeltaTime);
	myTakeDamageFromHeavyAttackTimer.Update(aDeltaTime);
	myTakeDamageFromAbilityOneTimer.Update(aDeltaTime);
	myTakeDamageFromAbilityTwoTimer.Update(aDeltaTime);
	myTimeSinceLastTickFromAbilityThree.Update(aDeltaTime);
	myPathfindingTimer.Update(aDeltaTime);
	if (myIsAlone) 
	{
		myAttackWhileRunningAwayTimer.Update(aDeltaTime);
	}
}

void RangedEnemy::SetAnimationState(float aDeltaTime)
{
	if (!IsAlive())
	{
		myCurrentAnimationIndex = static_cast<int>(eRangedEnemyAnimations::Death);
		myAnimations[myCurrentAnimationIndex].Play();
	}
	else if (myCurrentAnimationIndex == static_cast<int>(eRangedEnemyAnimations::Spawn) && myAnimations[static_cast<int>(eRangedEnemyAnimations::Spawn)].GetState() != DE::eAnimationState::Finished)
	{
		myAnimations[myCurrentAnimationIndex].Play();
	}
	else if (myCurrentAnimationIndex == static_cast<int>(eRangedEnemyAnimations::Ranged) && myAnimations[static_cast<int>(eRangedEnemyAnimations::Ranged)].GetState() != DE::eAnimationState::Finished)
	{
		myAnimations[myCurrentAnimationIndex].Play();
	}
	else
	{
		if (myCurrentAnimationIndex != static_cast<int>(eRangedEnemyAnimations::Walk))
		{
			myCurrentAnimationIndex = static_cast<int>(eRangedEnemyAnimations::Walk);
			myAnimations[myCurrentAnimationIndex].Update(UtilityFunctions::GetRandomFloat(0.0f, 0.5f));
		}
		myAnimations[myCurrentAnimationIndex].Play();
	}

	myAnimations[myCurrentAnimationIndex].Update(aDeltaTime);
	myAnimatedModelInstance->SetPose(myAnimations[myCurrentAnimationIndex]);
}

void RangedEnemy::RenderProjectileColliders()
{
	myProjectilePool->RenderColliders();
}

void RangedEnemy::RenderProjectiles(DreamEngine::GraphicsEngine& aGraphicsEngine)
{
	if (myProjectilePool != nullptr)
	{
		myProjectilePool->Render(aGraphicsEngine);
	}
}