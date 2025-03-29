#include "EnemyFactory.h"
#include "WeakEnemy.h"
#include "RangedEnemy.h"
#include "TankEnemy.h"

EnemyFactory::EnemyFactory()
{
	myEnemyTypes[static_cast<int>(eEnemyType::Weak)].enemyType = eEnemyType::Weak;
	myEnemyTypes[static_cast<int>(eEnemyType::Weak)].attackDamage = 5;
	myEnemyTypes[static_cast<int>(eEnemyType::Weak)].attackCooldown = 5.0f;
	myEnemyTypes[static_cast<int>(eEnemyType::Weak)].attackRange = 100.0f;
	myEnemyTypes[static_cast<int>(eEnemyType::Weak)].maxHealth = 50;
	myEnemyTypes[static_cast<int>(eEnemyType::Weak)].detectionRange = 1000.0f;
	myEnemyTypes[static_cast<int>(eEnemyType::Weak)].baseSpeed = 150.0f;
								  
	myEnemyTypes[static_cast<int>(eEnemyType::Ranged)].enemyType = eEnemyType::Ranged;
	myEnemyTypes[static_cast<int>(eEnemyType::Ranged)].attackDamage = 10;
	myEnemyTypes[static_cast<int>(eEnemyType::Ranged)].attackCooldown = 6.0f;
	myEnemyTypes[static_cast<int>(eEnemyType::Ranged)].attackRange = 500.0f;
	myEnemyTypes[static_cast<int>(eEnemyType::Ranged)].maxHealth = 75;
	myEnemyTypes[static_cast<int>(eEnemyType::Ranged)].detectionRange = 1000.0f;
	myEnemyTypes[static_cast<int>(eEnemyType::Ranged)].baseSpeed = 300.0f;
								  
	myEnemyTypes[static_cast<int>(eEnemyType::Tank)].enemyType = eEnemyType::Tank;
	myEnemyTypes[static_cast<int>(eEnemyType::Tank)].attackDamage = 20;
	myEnemyTypes[static_cast<int>(eEnemyType::Tank)].attackCooldown = 5.0f;
	myEnemyTypes[static_cast<int>(eEnemyType::Tank)].attackRange = 100.0f;
	myEnemyTypes[static_cast<int>(eEnemyType::Tank)].maxHealth = 200;
	myEnemyTypes[static_cast<int>(eEnemyType::Tank)].detectionRange = 1000.0f;
	myEnemyTypes[static_cast<int>(eEnemyType::Tank)].baseSpeed = 200.0f;
}

std::shared_ptr<WeakEnemy> EnemyFactory::CreateWeakEnemy() const
{
	return std::make_shared<WeakEnemy>(WeakEnemy(myEnemyTypes[static_cast<int>(eEnemyType::Weak)]));
}

std::shared_ptr<RangedEnemy> EnemyFactory::CreateRangedEnemy() const
{
	return std::make_shared<RangedEnemy>(RangedEnemy(myEnemyTypes[static_cast<int>(eEnemyType::Ranged)]));
}

std::shared_ptr<TankEnemy> EnemyFactory::CreateTankEnemy() const
{
	return std::make_shared<TankEnemy>(TankEnemy(myEnemyTypes[static_cast<int>(eEnemyType::Tank)]));
}

const EnemyType& EnemyFactory::GetEnemyType(const eEnemyType anEnemyType) const
{
	return myEnemyTypes[static_cast<int>(anEnemyType)];
}