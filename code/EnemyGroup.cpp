#include "EnemyGroup.h"
#include "WeakEnemy.h"
#include "RangedEnemy.h"
#include "TankEnemy.h"
#include "MainSingleton.h"

EnemyGroup::EnemyGroup(int aGroupId) : myEnemyGroupId(aGroupId) {}

std::vector<std::shared_ptr<WeakEnemy>>& EnemyGroup::GetWeakEnemies()
{
	return myWeakEnemies;
}

std::vector<std::shared_ptr<RangedEnemy>>& EnemyGroup::GetRangedEnemies()
{
	return myRangedEnemies;
}

std::vector<std::shared_ptr<TankEnemy>>& EnemyGroup::GetTankEnemies()
{
	return myTankEnemies;
}

float& EnemyGroup::GetDetectionRange()
{
	return myDetectionRange;
}

bool EnemyGroup::IsBossGroup()
{
	return myIsBossGroup;
}

void EnemyGroup::SetAsBossGroup()
{
	myIsBossGroup = true;
}

bool EnemyGroup::IsBubbleGroup()
{
	return myIsBubbleGroup;
}

void EnemyGroup::SetAsBubbleGroup()
{
	myIsBubbleGroup = true;
}

bool EnemyGroup::IsAlive()
{
	bool isAlive = false;
	for (auto& weakEnemy : myWeakEnemies) 
	{
		if (weakEnemy->IsAlive()) 
		{
			isAlive = true;
		}
	}
	for (auto& rangedEnemy : myRangedEnemies) 
	{
		if (rangedEnemy->IsAlive())
		{
			isAlive = true;
		}
	}
	for (auto& tankEnemy : myTankEnemies) 
	{
		if (tankEnemy->IsAlive())
		{
			isAlive = true;
		}
	}
	return isAlive;
}

bool EnemyGroup::IsActive()
{
	return myIsActive;
}

void EnemyGroup::SetActive(bool anIsActive)
{
	if (myIsBossGroup && anIsActive == false)
	{
		bool isIdle = true;
		myIsActive = anIsActive;
		for (auto weakEnemy : myWeakEnemies)
		{
			weakEnemy->SetIsIdleDuringBoss(isIdle);
		}
		for (auto rangedEnemy : myRangedEnemies)
		{
			rangedEnemy->SetIsIdleDuringBoss(isIdle);
		}
		for (auto tankEnemy : myTankEnemies)
		{
			tankEnemy->SetIsIdleDuringBoss(isIdle);
		}
	}
	else if (myIsBossGroup && anIsActive == true)
	{
		bool isIdle = false;
		myIsActive = anIsActive;
		for (auto weakEnemy : myWeakEnemies)
		{
			weakEnemy->SetIsIdleDuringBoss(isIdle);
		}
		for (auto rangedEnemy : myRangedEnemies)
		{
			rangedEnemy->SetIsIdleDuringBoss(isIdle);
		}
		for (auto tankEnemy : myTankEnemies)
		{
			tankEnemy->SetIsIdleDuringBoss(isIdle);
		}
	}
	else
	{
		myIsActive = anIsActive;
	}
}

void EnemyGroup::AlertAll()
{
	if (myGroupIsAlerted) 
	{
		return;
	}
	myGroupIsAlerted = true;
	for (auto weakEnemy : myWeakEnemies)
	{
		weakEnemy->Alert();
	}
	for (auto rangedEnemy : myRangedEnemies)
	{
		rangedEnemy->Alert();
	}
	for (auto tankEnemy : myTankEnemies)
	{
		tankEnemy->Alert();
	}
}

void EnemyGroup::ResetAlert()
{
	if (myGroupIsAlerted) 
	{
		myGroupIsAlerted = false;
	}
}

int EnemyGroup::GetId()
{
	return myEnemyGroupId;
}