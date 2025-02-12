#include "TankEnemy.h"
#include "RangedEnemy.h"
#include "EnemyGroup.h"
#include "Navmesh.h"
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

TankEnemy::TankEnemy(const EnemyType& anEnemyType) : Enemy(anEnemyType) {}

TankEnemy::~TankEnemy()
{
	MainSingleton::GetInstance()->GetCollisionManager().DeregisterObject(this, eCollisionLayer::PlayerAttacks);
	MainSingleton::GetInstance()->GetCollisionManager().DeregisterObject(this, eCollisionLayer::Movement);
	MainSingleton::GetInstance()->GetPostMaster().Unsubscribe(eMessageType::PlayerRespawned, this);
}

void TankEnemy::Init()
{
	SetAnimatedModel();

	Enemy::Init();
	myVerticalHpBarOffset = 250.f;

	AddComponent<ColliderComponent>();
	auto* colliderCmp = GetComponent<ColliderComponent>();
	colliderCmp->SetCollider(std::make_shared<DE::SphereCollider<float>>(DE::SphereCollider<float>(&myTransform, { 0.0f, 50.0f, 0.0f }, 50.0f)));
	MainSingleton::GetInstance()->GetCollisionManager().RegisterObject(this, eCollisionLayer::PlayerAttacks);
	MainSingleton::GetInstance()->GetCollisionManager().RegisterObject(this, eCollisionLayer::Movement);
	MainSingleton::GetInstance()->GetPostMaster().Subscribe(eMessageType::PlayerRespawned, this);

	myChargeTarget.x = -50000.0f;
	myProtectionChargeTimer = CU::CountdownTimer(5.0f, 5.0f);
	DE::Vector3f selectionOffset(0.0f, 100.0f, 0.0f);
	mySelectionCylinder = DE::CylinderCollider<float>(&myTransform, selectionOffset, 80.0f, 150.0f);

	AK::SoundEngine::RegisterGameObj(myAkId, "TankEnemy");
	myCurrentAnimationIndex = static_cast<int>(eTankEnemyAnimations::Spawn);
	myAnimations[myCurrentAnimationIndex].Update(UtilityFunctions::GetRandomFloat(0.0f, 0.5f));
	UpdateSoundEnginePosition();
}

void TankEnemy::SetAnimatedModel()
{
	DE::ModelFactory& modelFactory = DE::ModelFactory::GetInstance();
	std::wstring resolved_path = DE::Settings::ResolveAssetPathW(L"3D/SK_CH_EnemyCrusader.fbx");
	myAnimatedModelInstance = std::make_shared<DE::AnimatedModelInstance>(modelFactory.GetAnimatedModelInstance(resolved_path));

	myAnimations.reserve(6);
	resolved_path = string_cast<std::wstring>((std::string)"3D/A_CH_EnemyCrusader_Idle.fbx");
	myAnimations.push_back(modelFactory.GetAnimationPlayer(resolved_path, myAnimatedModelInstance->GetModel()));
	myAnimations.back().SetIsLooping(true);

	resolved_path = string_cast<std::wstring>((std::string)"3D/A_CH_EnemyCrusader_WalkCycle.fbx");
	myAnimations.push_back(modelFactory.GetAnimationPlayer(resolved_path, myAnimatedModelInstance->GetModel()));
	myAnimations.back().SetIsLooping(true);

	resolved_path = string_cast<std::wstring>((std::string)"3D/A_CH_EnemyCrusader_HeavyAttack.fbx");
	myAnimations.push_back(modelFactory.GetAnimationPlayer(resolved_path, myAnimatedModelInstance->GetModel()));
	myAnimations.back().SetIsLooping(false);

	resolved_path = string_cast<std::wstring>((std::string)"3D/A_CH_EnemyCrusader_Dash.fbx");
	myAnimations.push_back(modelFactory.GetAnimationPlayer(resolved_path, myAnimatedModelInstance->GetModel()));
	myAnimations.back().SetIsLooping(false);

	resolved_path = string_cast<std::wstring>((std::string)"3D/A_CH_EnemyCrusader_Death.fbx");
	myAnimations.push_back(modelFactory.GetAnimationPlayer(resolved_path, myAnimatedModelInstance->GetModel()));
	myAnimations.back().SetIsLooping(false);

	resolved_path = string_cast<std::wstring>((std::string)"3D/A_CH_EnemyCrusader_Spawn.fbx");
	myAnimations.push_back(modelFactory.GetAnimationPlayer(resolved_path, myAnimatedModelInstance->GetModel()));
	myAnimations.back().SetIsLooping(false);
}

void TankEnemy::Update(float aDeltaTime)
{
	if (!myEnemyGroup->IsActive())
	{
		return;
	}

	Enemy::Update(aDeltaTime);

	if (IsAlive() && myPlayer->IsAlive() && myCurrentAnimationIndex != static_cast<int>(eTankEnemyAnimations::Spawn))
	{
		myVelocity = { 0.0f, 0.0f, 0.0f };

		float sqrDistToPlayer = (myTransform.GetPosition() - myPlayer->GetTransform()->GetPosition()).LengthSqr();

		if (!myStartedIdleSound && myCurrentAnimationIndex == static_cast<int>(eTankEnemyAnimations::Idle) && myAnimations[myCurrentAnimationIndex].GetFrame() == 15)
		{
			PlayIdleSound();
		}

		if (myChargeTarget.x != -50000.0f) 
		{
			if (DE::Vector3f::Distance(myTransform.GetPosition(), myChargeTarget) < 50.0f) 
			{
				myChargeTarget.x = -50000.0f;
				myMoveSpeedModifier = 1.0f;
			}
			TurnTowards(myChargeTarget);
			if (myCollided == false)
			{
				MoveTowards(myChargeTarget);
			}
			else
			{
				myChargeTarget = myPositionLastFrame;
			}
		}
		else if (sqrDistToPlayer < myEnemyGroup->GetDetectionRange() * myEnemyGroup->GetDetectionRange() || myIsAlertedByAlly)
		{
			myEnemyGroup->AlertAll();
			if (myEnemyGroup->GetRangedEnemies().size() > 0 && myProtectionChargeTimer.IsDone())
			{
				std::shared_ptr<RangedEnemy> rangedEnemyToChargeTo = nullptr;
				for (auto rangedEnemy : myEnemyGroup->GetRangedEnemies())
				{
					if (rangedEnemy->IsAlive() && DE::Vector3f(rangedEnemy->GetTransform()->GetPosition() - myPlayer->GetPosition()).GetNormalized().Dot(myPlayer->GetTransform()->GetMatrix().GetForward()) > 0.85f)
					{
						rangedEnemyToChargeTo = rangedEnemy;
					}
				}
				if (rangedEnemyToChargeTo != nullptr && sqrDistToPlayer > myEnemyType.attackRange * 2.0f) 
				{
					PerformProtectionCharge(rangedEnemyToChargeTo);
				}
				else if (sqrDistToPlayer > myEnemyType.attackRange * myEnemyType.attackRange)
				{
					ChasePlayer();
				}
			}
			else if(sqrDistToPlayer > myEnemyType.attackRange * myEnemyType.attackRange)
			{
				ChasePlayer();
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

		if (myPositionLastFrame != myTransform.GetPosition() && myCurrentAnimationIndex == static_cast<int>(eTankEnemyAnimations::Walk) && myAnimations[myCurrentAnimationIndex].GetFrame() % 20 == 0)
		{
			AK::SoundEngine::PostEvent("Play_EnemyTankMovement", myAkId);
		}
		if (myCurrentAnimationIndex != static_cast<int>(eTankEnemyAnimations::Death) && myCurrentAnimationIndex != static_cast<int>(eTankEnemyAnimations::Attack) && myCurrentAnimationIndex != static_cast<int>(eTankEnemyAnimations::Charge))
		{
			SetCurrentToWalkOrIdleAnimation();
		}
		if (myCurrentAnimationIndex == static_cast<int>(eTankEnemyAnimations::Attack) && myAnimations[myCurrentAnimationIndex].GetFrame() == 20)
		{
			AK::SoundEngine::PostEvent("Play_EnemyTankAttackHit", myAkId);
		}
	}
	else if (myCurrentAnimationIndex == static_cast<int>(eTankEnemyAnimations::Spawn) && myAnimations[myCurrentAnimationIndex].GetFrame() == 15)
	{
		AK::SoundEngine::PostEvent("Play_WorldGroundCrack", myAkId);
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

void TankEnemy::OnCollision(GameObject* anObject, eCollisionLayer aCollisionLayer)
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

void TankEnemy::Receive(const Message& aMessage)
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

void TankEnemy::Respawn()
{
	HealToMax();
	myTransform.SetPosition(mySpawnPos);
	myAnimatedModelInstance->SetLocation(myTransform.GetPosition());
	myCurrentAnimationIndex = static_cast<int>(eTankEnemyAnimations::Spawn);
	myAnimations[myCurrentAnimationIndex].Stop();
	myAnimations[myCurrentAnimationIndex].Update(UtilityFunctions::GetRandomFloat(0.0f, 0.5f));
	myAnimations[static_cast<int>(eTankEnemyAnimations::Death)].Stop();
	myIsAlertedByAlly = false;
	myEnemyGroup->ResetAlert();
	myPathfindingTimer.Zeroize();
	myChargeTarget.x = -50000.0f;
	AK::SoundEngine::StopAll(myAkId);
	myStartedIdleSound = false;
	myCurrentHpBarSize = myOriginalHpBarSize;
	if (myEnemyGroup->IsBubbleGroup())
	{
		myEnemyGroup->SetActive(false);
	}
}

void TankEnemy::PlayIdleSound()
{
	AK::SoundEngine::PostEvent("Play_EnemyTankIdle", myAkId);
	if (myIsAlertedByAlly)
	{
		AK::SoundEngine::StopAll(myAkId);
	}
	myStartedIdleSound = true;
}

void TankEnemy::PerformProtectionCharge(std::shared_ptr<RangedEnemy> aRangedEnemy)
{
	DE::Vector3f positionOfAttacker = myPlayer->GetPosition();
	DE::Vector3f positionOfProtectee = aRangedEnemy->GetTransform()->GetPosition();
	DE::Vector3f positionToChargeFrom = myTransform.GetPosition();

	DE::Vector3f attackerToProtecteeVector = positionOfProtectee - positionOfAttacker;
	DE::Vector3f attackerToChargeOriginVector = positionToChargeFrom - positionOfAttacker;

	float distToClosestPointOnLine = attackerToChargeOriginVector.Dot(attackerToProtecteeVector) / attackerToProtecteeVector.Dot(attackerToProtecteeVector);
	distToClosestPointOnLine = CU::Clamp(distToClosestPointOnLine, 0.0f, 1.0f);
	DE::Vector3f pointToChargeTo = positionOfAttacker + distToClosestPointOnLine * attackerToProtecteeVector;

	TurnTowards(pointToChargeTo);
	if (myCollided == false)
	{
		MoveTowards(pointToChargeTo);
	}
	myProtectionChargeTimer.Reset();
	myChargeTarget = pointToChargeTo;
	myMoveSpeedModifier = 5.0f;
	myPathfindingTimer.Zeroize();
	myCurrentAnimationIndex = static_cast<int>(eTankEnemyAnimations::Charge);
	myAnimations[myCurrentAnimationIndex].Stop();
	AK::SoundEngine::PostEvent("Play_EnemyTankChargeWhoosh", myAkId);
}

void TankEnemy::SetCurrentToWalkOrIdleAnimation()
{
	if (myPositionLastFrame != myTransform.GetPosition())
	{
		if (myCurrentAnimationIndex != static_cast<int>(eTankEnemyAnimations::Walk))
		{
			myCurrentAnimationIndex = static_cast<int>(eTankEnemyAnimations::Walk);
			myAnimations[myCurrentAnimationIndex].Update(UtilityFunctions::GetRandomFloat(0.0f, 0.5f));
		}
	}
	else if (myCurrentAnimationIndex != static_cast<int>(eTankEnemyAnimations::Idle))
	{
		myCurrentAnimationIndex = static_cast<int>(eTankEnemyAnimations::Idle);
		myAnimations[myCurrentAnimationIndex].Update(UtilityFunctions::GetRandomFloat(0.0f, 0.5f));
	}
}

void TankEnemy::RunAttackBehavior(float aSqrDistToPlayer)
{
	if (aSqrDistToPlayer < myEnemyType.attackRange * myEnemyType.attackRange && myAttackTimer.IsDone())
	{
		TurnTowards(myPlayer->GetPosition());
		myAttackTimer.Reset();
		myPlayer->TakeDamage(myEnemyType.attackDamage);
		myCurrentAnimationIndex = static_cast<int>(eTankEnemyAnimations::Attack);
		AK::SoundEngine::PostEvent("Play_EnemyTankAttack", myAkId);
		myAnimations[myCurrentAnimationIndex].Stop();
	}
}

void TankEnemy::CountDownCooldowns(float aDeltaTime)
{
	myAttackTimer.Update(aDeltaTime);
	myTakeDamageFromAbilityThreeTimer.Update(aDeltaTime);
	myTakeDamageFromHeavyAttackTimer.Update(aDeltaTime);
	myTakeDamageFromAbilityOneTimer.Update(aDeltaTime);
	myTakeDamageFromAbilityTwoTimer.Update(aDeltaTime);
	myTimeSinceLastTickFromAbilityThree.Update(aDeltaTime);
	myProtectionChargeTimer.Update(aDeltaTime);
	myPathfindingTimer.Update(aDeltaTime);
}

void TankEnemy::SetAnimationState(float aDeltaTime)
{
	if (!IsAlive())
	{
		myCurrentAnimationIndex = static_cast<int>(eTankEnemyAnimations::Death);
		myAnimations[myCurrentAnimationIndex].Play();
	}
	else if (myCurrentAnimationIndex == static_cast<int>(eTankEnemyAnimations::Spawn) && myAnimations[static_cast<int>(eTankEnemyAnimations::Spawn)].GetState() != DE::eAnimationState::Finished)
	{
		myAnimations[myCurrentAnimationIndex].Play();
	}
	else if (myCurrentAnimationIndex == static_cast<int>(eTankEnemyAnimations::Attack) && myAnimations[static_cast<int>(eTankEnemyAnimations::Attack)].GetState() != DE::eAnimationState::Finished)
	{
		myAnimations[myCurrentAnimationIndex].Play();
	}
	else if (myCurrentAnimationIndex == static_cast<int>(eTankEnemyAnimations::Charge) && myAnimations[static_cast<int>(eTankEnemyAnimations::Charge)].GetState() != DE::eAnimationState::Finished)
	{
		myAnimations[myCurrentAnimationIndex].Play();
	}
	else if (myCurrentAnimationIndex == static_cast<int>(eTankEnemyAnimations::Walk))
	{
		myAnimations[myCurrentAnimationIndex].Play();
	}
	else
	{
		if (myCurrentAnimationIndex != static_cast<int>(eTankEnemyAnimations::Idle))
		{
			myCurrentAnimationIndex = static_cast<int>(eTankEnemyAnimations::Idle);
			myAnimations[myCurrentAnimationIndex].Update(UtilityFunctions::GetRandomFloat(0.0f, 0.5f));
		}
		myAnimations[myCurrentAnimationIndex].Play();
	}

	myAnimations[myCurrentAnimationIndex].Update(aDeltaTime);
	myAnimatedModelInstance->SetPose(myAnimations[myCurrentAnimationIndex]);
}