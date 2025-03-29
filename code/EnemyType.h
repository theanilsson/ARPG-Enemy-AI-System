#pragma once

enum class eEnemyType
{
	Weak,
	Ranged,
	Tank,
	DefaultTypeAndCount
};

struct EnemyType
{
	eEnemyType enemyType;
	int attackDamage;
	float attackCooldown;
	float attackRange;
	int maxHealth;
	float detectionRange;
	float baseSpeed;
};