#include "world/entity/MobCategory.h"

#include "world/level/material/Material.h"
#include "world/level/material/GasMaterial.h"
#include "world/level/material/LiquidMaterial.h"

int_t MobCategoryHelper::getBaseClassType(MobCategory category)
{
	switch (category)
	{
	case MobCategory::monster:
		return 1; // Enemy/Monster type
	case MobCategory::creature:
		return 2; // Animal type
	case MobCategory::waterCreature:
		return 3; // WaterAnimal type
	default:
		return 0;
	}
}

int_t MobCategoryHelper::getMaxInstancesPerChunk(MobCategory category)
{
	switch (category)
	{
	case MobCategory::monster:
		return 70;
	case MobCategory::creature:
		return 15;
	case MobCategory::waterCreature:
		return 5;
	default:
		return 0;
	}
}

const Material &MobCategoryHelper::getSpawnPositionMaterial(MobCategory category)
{
	switch (category)
	{
	case MobCategory::monster:
	case MobCategory::creature:
		return Material::air;
	case MobCategory::waterCreature:
		return Material::water;
	default:
		return Material::air;
	}
}

bool MobCategoryHelper::isFriendly(MobCategory category)
{
	switch (category)
	{
	case MobCategory::monster:
		return false;
	case MobCategory::creature:
	case MobCategory::waterCreature:
		return true;
	default:
		return false;
	}
}