#include "WeakEnemy.h"
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
#include <DreamEngine\graphics\TextureManager.h>
#include <DreamEngine\windows\settings.h>
#include <DreamEngine\utilities\StringCast.h>
#include <DreamEngine\utilities\UtilityFunctions.h>

WeakEnemy::WeakEnemy(const EnemyType& anEnemyType) : Enemy(anEnemyType){}

WeakEnemy::~WeakEnemy()
{
	MainSingleton::GetInstance()->GetCollisionManager().DeregisterObject(this, eCollisionLayer::PlayerAttacks);
	MainSingleton::GetInstance()->GetCollisionManager().DeregisterObject(this, eCollisionLayer::Movement);
	MainSingleton::GetInstance()->GetPostMaster().Unsubscribe(eMessageType::PlayerRespawned, this);
}

void WeakEnemy::Init()
{
	SetAnimatedModel();

	if (UtilityFunctions::GetRandomInt(1, 100) > 90)
	{
		myIsElite = true;
		myAdditionalDamage = myEnemyType.attackDamage;
		myTransform.SetScale(1.05f);	
		DE::TextureResource* albedoTexture = DreamEngine::Engine::GetInstance()->GetTextureManager().TryGetTexture(L"../Assets/3D/t_ch_enemyzombie_2_c.dds");
		DE::TextureResource* normalTexture = DreamEngine::Engine::GetInstance()->GetTextureManager().TryGetTexture(L"../Assets/3D/t_ch_enemyzombie_2_n.dds");
		DE::TextureResource* materialTexture = DreamEngine::Engine::GetInstance()->GetTextureManager().TryGetTexture(L"../Assets/3D/t_ch_enemyzombie_2_m.dds");
		for (int i = 0; i < myAnimatedModelInstance->GetModel()->GetMeshCount(); i++)
		{
			myAnimatedModelInstance->SetTexture(i, 0, albedoTexture);
			myAnimatedModelInstance->SetTexture(i, 1, normalTexture);
			myAnimatedModelInstance->SetTexture(i, 2, materialTexture);
		}
	}
	
	Enemy::Init(); 
	myVerticalHpBarOffset = 190.f;

	AddComponent<ColliderComponent>();
	auto* colliderCmp = GetComponent<ColliderComponent>();
	colliderCmp->SetCollider(std::make_shared<DE::SphereCollider<float>>(DE::SphereCollider<float>(&myTransform, { 0.0f, 50.0f, 0.0f }, 40.0f)));
	MainSingleton::GetInstance()->GetCollisionManager().RegisterObject(this, eCollisionLayer::PlayerAttacks);
	MainSingleton::GetInstance()->GetCollisionManager().RegisterObject(this, eCollisionLayer::Movement);
	MainSingleton::GetInstance()->GetPostMaster().Subscribe(eMessageType::PlayerRespawned, this);
	DE::Vector3f selectionOffset(0.0f, 50.0f, 0.0f);
	mySelectionCylinder = DE::CylinderCollider<float>(&myTransform, selectionOffset, 70.0f, 100.0f);
	myAttackSoundDelayTimer = CU::CountupTimer(0.25f);

	AK::SoundEngine::RegisterGameObj(myAkId, "WeakEnemy");
	myCurrentAnimationIndex = static_cast<int>(eWeakEnemyAnimations::Spawn);
	myAnimations[myCurrentAnimationIndex].Update(UtilityFunctions::GetRandomFloat(0.0f, 0.5f));
	UpdateSoundEnginePosition();
}

void WeakEnemy::SetAnimatedModel()
{
	DE::ModelFactory& modelFactory = DE::ModelFactory::GetInstance();
	std::wstring resolved_path = DE::Settings::ResolveAssetPathW(L"3D/SK_CH_EnemyZombie.fbx");
	myAnimatedModelInstance = std::make_shared<DE::AnimatedModelInstance>(modelFactory.GetAnimatedModelInstance(resolved_path));

	myAnimations.reserve(6);
	resolved_path = string_cast<std::wstring>((std::string)"3D/A_CH_EnemyZombie_Idle.fbx");
	myAnimations.push_back(modelFactory.GetAnimationPlayer(resolved_path, myAnimatedModelInstance->GetModel()));
	myAnimations.back().SetIsLooping(true);

	resolved_path = string_cast<std::wstring>((std::string)"3D/A_CH_EnemyZombie_Walk.fbx");
	myAnimations.push_back(modelFactory.GetAnimationPlayer(resolved_path, myAnimatedModelInstance->GetModel()));
	myAnimations.back().SetIsLooping(true);

	resolved_path = string_cast<std::wstring>((std::string)"3D/A_CH_EnemyZombie_LightAttack.fbx");
	myAnimations.push_back(modelFactory.GetAnimationPlayer(resolved_path, myAnimatedModelInstance->GetModel()));
	myAnimations.back().SetIsLooping(false);

	resolved_path = string_cast<std::wstring>((std::string)"3D/A_CH_EnemyZombie_LightAttack.fbx");
	myAnimations.push_back(modelFactory.GetAnimationPlayer(resolved_path, myAnimatedModelInstance->GetModel()));
	myAnimations.back().SetIsLooping(false);

	resolved_path = string_cast<std::wstring>((std::string)"3D/A_CH_EnemyZombie_Death.fbx");
	myAnimations.push_back(modelFactory.GetAnimationPlayer(resolved_path, myAnimatedModelInstance->GetModel()));
	myAnimations.back().SetIsLooping(false);

	resolved_path = string_cast<std::wstring>((std::string)"3D/A_CH_EnemyZombie_Spawn.fbx");
	myAnimations.push_back(modelFactory.GetAnimationPlayer(resolved_path, myAnimatedModelInstance->GetModel()));
	myAnimations.back().SetIsLooping(false);
}

void WeakEnemy::Update(float aDeltaTime)
{
	if (!myEnemyGroup->IsActive()) 
	{
		return;
	}

	Enemy::Update(aDeltaTime);

	if (IsAlive() && myPlayer->IsAlive() && myCurrentAnimationIndex != static_cast<int>(eWeakEnemyAnimations::Spawn))
	{
		myVelocity = { 0.0f, 0.0f, 0.0f };

		float sqrDistToPlayer = (myTransform.GetPosition() - myPlayer->GetTransform()->GetPosition()).LengthSqr();
		if (!myStartedIdleSound && myCurrentAnimationIndex == static_cast<int>(eWeakEnemyAnimations::Idle) && myAnimations[myCurrentAnimationIndex].GetFrame() == 15)
		{
			PlayIdleSound();
		}
		if (sqrDistToPlayer < myEnemyGroup->GetDetectionRange() * myEnemyGroup->GetDetectionRange() || myIsAlertedByAlly)
		{
			myEnemyGroup->AlertAll();
			if (sqrDistToPlayer > myEnemyType.attackRange * myEnemyType.attackRange)
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

		if (myPositionLastFrame != myTransform.GetPosition() && myCurrentAnimationIndex == static_cast<int>(eWeakEnemyAnimations::Walk) && myAnimations[myCurrentAnimationIndex].GetFrame() == 35)
		{
			AK::SoundEngine::PostEvent("Play_EnemyPopcornMovement", myAkId);
		}
		if (myCurrentAnimationIndex != static_cast<int>(eWeakEnemyAnimations::Death) && myCurrentAnimationIndex != static_cast<int>(eWeakEnemyAnimations::AttackVariationOne) && myCurrentAnimationIndex != static_cast<int>(eWeakEnemyAnimations::AttackVariationTwo))
		{
			SetCurrentToWalkOrIdleAnimation();
		}
		if ((myCurrentAnimationIndex == static_cast<int>(eWeakEnemyAnimations::AttackVariationOne) || myCurrentAnimationIndex == static_cast<int>(eWeakEnemyAnimations::AttackVariationTwo)) && myAnimations[myCurrentAnimationIndex].GetFrame() == 20)
		{
			AK::SoundEngine::PostEvent("Play_EnemyPopcornAttackHit", myAkId);
		}
	}
	else if (myCurrentAnimationIndex == static_cast<int>(eWeakEnemyAnimations::Spawn) && myAnimations[myCurrentAnimationIndex].GetFrame() == 15)
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

void WeakEnemy::Alert()
{
	if (!myIsAlertedByAlly)
	{
		AK::SoundEngine::PostEvent("Play_EnemyPopcornAggro", myAkId);
		if (myEnemyGroup->GetTankEnemies().size() > 0)
		{
			ApplyTankBuff();
		}
	}
	myIsAlertedByAlly = true;
}

void WeakEnemy::OnCollision(GameObject* anObject, eCollisionLayer aCollisionLayer)
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
					myPlayer->AddResource(5);
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

void WeakEnemy::Receive(const Message& aMessage)
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

void WeakEnemy::PlayIdleSound()
{
	AK::SoundEngine::PostEvent("Play_EnemyPopcornIdle", myAkId);
	if (myIsAlertedByAlly)
	{
		AK::SoundEngine::StopAll(myAkId);
	}
	myStartedIdleSound = true;
}

void WeakEnemy::ApplyTankBuff()
{
	myMoveSpeedModifier = 1.5f;
}

void WeakEnemy::SetCurrentToWalkOrIdleAnimation()
{
	if (myPositionLastFrame != myTransform.GetPosition() && myCurrentAnimationIndex != static_cast<int>(eWeakEnemyAnimations::Walk))
	{
		myCurrentAnimationIndex = static_cast<int>(eWeakEnemyAnimations::Walk);
		myAnimations[myCurrentAnimationIndex].Update(UtilityFunctions::GetRandomFloat(0.0f, 0.5f));
	}
	else if (myPositionLastFrame == myTransform.GetPosition() && myCurrentAnimationIndex != static_cast<int>(eWeakEnemyAnimations::Idle))
	{
		myCurrentAnimationIndex = static_cast<int>(eWeakEnemyAnimations::Idle);
		myAnimations[myCurrentAnimationIndex].Update(UtilityFunctions::GetRandomFloat(0.0f, 0.5f));
	}
}

void WeakEnemy::RunAttackBehavior(float aSqrDistToPlayer)
{
	if (aSqrDistToPlayer < myEnemyType.attackRange * myEnemyType.attackRange && myAttackTimer.IsDone())
	{
		TurnTowards(myPlayer->GetPosition());
		myAttackTimer.Reset();
		myPlayer->TakeDamage(myEnemyType.attackDamage + myAdditionalDamage);
		AK::SoundEngine::PostEvent("Play_EnemyPopcornAttack", myAkId);
		if (myUseAttackVarOne) 
		{
			myCurrentAnimationIndex = static_cast<int>(eWeakEnemyAnimations::AttackVariationOne);
		}
		else
		{
			myCurrentAnimationIndex = static_cast<int>(eWeakEnemyAnimations::AttackVariationTwo);
		}
		myAnimations[myCurrentAnimationIndex].Stop();
	}
}

void WeakEnemy::CountDownCooldowns(float aDeltaTime)
{
	myAttackTimer.Update(aDeltaTime);
	myTakeDamageFromAbilityThreeTimer.Update(aDeltaTime);
	myTakeDamageFromHeavyAttackTimer.Update(aDeltaTime);
	myTakeDamageFromAbilityOneTimer.Update(aDeltaTime);
	myTakeDamageFromAbilityTwoTimer.Update(aDeltaTime);
	myTimeSinceLastTickFromAbilityThree.Update(aDeltaTime);
	myPathfindingTimer.Update(aDeltaTime);
}

void WeakEnemy::SetAnimationState(float aDeltaTime)
{
	if (!IsAlive())
	{
		myCurrentAnimationIndex = static_cast<int>(eWeakEnemyAnimations::Death);
		myAnimations[myCurrentAnimationIndex].Play();
	}
	else if (myCurrentAnimationIndex == static_cast<int>(eWeakEnemyAnimations::Spawn) && myAnimations[static_cast<int>(eWeakEnemyAnimations::Spawn)].GetState() != DE::eAnimationState::Finished)
	{
		myAnimations[myCurrentAnimationIndex].Play();
	}
	else if (myCurrentAnimationIndex == static_cast<int>(eWeakEnemyAnimations::AttackVariationOne) && myAnimations[static_cast<int>(eWeakEnemyAnimations::AttackVariationOne)].GetState() != DE::eAnimationState::Finished)
	{
		myAnimations[myCurrentAnimationIndex].Play();
	}
	else if (myCurrentAnimationIndex == static_cast<int>(eWeakEnemyAnimations::AttackVariationTwo) && myAnimations[static_cast<int>(eWeakEnemyAnimations::AttackVariationTwo)].GetState() != DE::eAnimationState::Finished)
	{
		myAnimations[myCurrentAnimationIndex].Play();
	}
	else if(myCurrentAnimationIndex == static_cast<int>(eWeakEnemyAnimations::Walk))
	{
		myAnimations[myCurrentAnimationIndex].Play();
	}
	else
	{
		if (myCurrentAnimationIndex != static_cast<int>(eWeakEnemyAnimations::Idle)) 
		{
			myCurrentAnimationIndex = static_cast<int>(eWeakEnemyAnimations::Idle);
			myAnimations[myCurrentAnimationIndex].Update(UtilityFunctions::GetRandomFloat(0.0f, 0.5f));
		}
		myAnimations[myCurrentAnimationIndex].Play();
	}

	myAnimations[myCurrentAnimationIndex].Update(aDeltaTime);
	myAnimatedModelInstance->SetPose(myAnimations[myCurrentAnimationIndex]);
}