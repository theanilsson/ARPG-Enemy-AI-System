#pragma once
#include <array>
#include <memory>
#include "Enemy.h"
#include "EnemyType.h"

class WeakEnemy;
class RangedEnemy;
class TankEnemy;
class EnemyFactory
{
public:
	EnemyFactory();

	std::shared_ptr<WeakEnemy> CreateWeakEnemy() const;
	std::shared_ptr<RangedEnemy> CreateRangedEnemy() const;
	std::shared_ptr<TankEnemy> CreateTankEnemy() const;
	const EnemyType& GetEnemyType(const eEnemyType anEnemyType) const;

private:
	std::array<EnemyType, static_cast<int>(eEnemyType::DefaultTypeAndCount)> myEnemyTypes;
};