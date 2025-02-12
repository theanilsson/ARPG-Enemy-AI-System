#pragma once
#include <vector>
#include <memory>

class WeakEnemy;
class RangedEnemy;
class TankEnemy;
class EnemyGroup
{
public:
	EnemyGroup() = delete;
	EnemyGroup(int aGroupId);
	std::vector<std::shared_ptr<WeakEnemy>>& GetWeakEnemies();
	std::vector<std::shared_ptr<RangedEnemy>>& GetRangedEnemies();
	std::vector<std::shared_ptr<TankEnemy>>& GetTankEnemies();
	float& GetDetectionRange();
	bool IsBossGroup();
	void SetAsBossGroup();
	bool IsBubbleGroup();
	void SetAsBubbleGroup();
	bool IsAlive();
	bool IsActive();
	void SetActive(bool anIsActive);
	void AlertAll();
	void ResetAlert();
	int GetId();

private:
	std::vector<std::shared_ptr<WeakEnemy>> myWeakEnemies;
	std::vector<std::shared_ptr<RangedEnemy>> myRangedEnemies;
	std::vector<std::shared_ptr<TankEnemy>> myTankEnemies;
	float myDetectionRange = 100.0f;
	int myEnemyGroupId = 0;
	bool myIsBossGroup = false;
	bool myIsBubbleGroup = false;
	bool myIsActive = true;
	bool myGroupIsAlerted = false;
};