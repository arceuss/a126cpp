#include "world/level/dimension/HellDimension.h"

#include "world/level/Level.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/UnbreakableTile.h"
#include "world/level/chunk/storage/OldChunkStorage.h"
#include "world/level/levelgen/RandomLevelSource.h"
#include "world/level/biome/BiomeSource.h"
#include "world/level/chunk/ChunkSource.h"
#include "world/level/chunk/storage/ChunkStorage.h"

#include "util/Mth.h"
#include "util/Memory.h"
#include <memory>

// Beta 1.2: HellDimension constructor
// Java: public HellDimension() (implicit, init() is called separately)
HellDimension::HellDimension(Level &level) : Dimension(level)
{
	// Beta 1.2: init() must be called explicitly after construction
	// Dimension constructor doesn't call init(), so we call it here
	init();
}

// Beta 1.2: HellDimension.init() - initializes Nether-specific properties
// Java: public void init() {
//         this.biomeSource = new FixedBiomeSource(Biome.hell, 1.0, 0.0);
//         this.foggy = true;
//         this.ultraWarm = true;
//         this.hasCeiling = true;
//         this.id = -1;
//       }
void HellDimension::init()
{
	// Beta 1.2: Use regular BiomeSource for now (FixedBiomeSource can be added later if needed)
	// Java: this.biomeSource = new FixedBiomeSource(Biome.hell, 1.0, 0.0);
	biomeSource = Util::make_unique<BiomeSource>(level);
	
	foggy = true;
	ultraWarm = true;
	hasCeiling = true;
	id = -1;
}

// Beta 1.2: HellDimension.updateLightRamp() - darker light ramp for Nether
// Java: protected void updateLightRamp() {
//         float var1 = 0.1F;
//         for (int var2 = 0; var2 <= 15; var2++) {
//             float var3 = 1.0F - var2 / 15.0F;
//             this.brightnessRamp[var2] = (1.0F - var3) / (var3 * 3.0F + 1.0F) * (1.0F - var1) + var1;
//         }
//       }
void HellDimension::updateLightRamp()
{
	float var1 = 0.1f;
	
	for (int_t i = 0; i <= 15; i++)
	{
		float var3 = 1.0f - static_cast<float>(i) / 15.0f;
		brightnessRamp[i] = (1.0f - var3) / (var3 * 3.0f + 1.0f) * (1.0f - var1) + var1;
	}
}

// Beta 1.2: HellDimension.createRandomLevelSource() - creates Nether chunk generator
// Java: public ChunkSource createRandomLevelSource() {
//         return new HellRandomLevelSource(this.level, this.level.seed);
//       }
// Note: Using RandomLevelSource for now (HellRandomLevelSource can be added later)
ChunkSource *HellDimension::createRandomLevelSource()
{
	return new RandomLevelSource(level, level.seed);
}

// Beta 1.2: HellDimension.createStorage() - creates storage in DIM-1 directory
// Java: public ChunkStorage createStorage(File var1) {
//         File var2 = new File(var1, "DIM-1");
//         var2.mkdirs();
//         return new OldChunkStorage(var2, true);
//       }
ChunkStorage *HellDimension::createStorage(std::shared_ptr<File> dir)
{
	if (dir == nullptr)
	{
		return nullptr;  // Multiplayer levels don't use file storage
	}
	
	// Beta 1.2: File var2 = new File(var1, "DIM-1");
	//           var2.mkdirs();
	//           return new OldChunkStorage(var2, true);
	// File::open returns File*, wrap it in shared_ptr
	std::shared_ptr<File> dimDir(File::open(*dir, u"DIM-1"), [](File* f) { delete f; });
	dimDir->mkdirs();
	return new OldChunkStorage(dimDir, true);
}

// Beta 1.2: HellDimension.isValidSpawn() - checks if position is valid spawn
// Java: public boolean isValidSpawn(int var1, int var2) {
//         int var3 = this.level.getTopTile(var1, var2);
//         if (var3 == Tile.unbreakable.id) {
//             return false;
//         } else {
//             return var3 == 0 ? false : Tile.solid[var3];
//         }
//       }
bool HellDimension::isValidSpawn(int_t x, int_t z)
{
	int_t top = level.getTopTile(x, z);
	// Beta 1.2: Check if top tile is unbreakable (bedrock)
	if (top == Tile::unbreakable.id)
	{
		return false;
	}
	// Beta 1.2: return var3 == 0 ? false : Tile.solid[var3];
	return top == 0 ? false : Tile::solid[top];
}

// Beta 1.2: HellDimension.getTimeOfDay() - always returns 0.5 (no day/night cycle in Nether)
// Java: public float getTimeOfDay(long var1, float var3) {
//         return 0.5F;
//       }
float HellDimension::getTimeOfDay(int_t time, float add)
{
	return 0.5f;
}

// Beta 1.2: HellDimension.getFogColor() - returns red fog color for Nether
// Java: public Vec3 getFogColor(float var1, float var2) {
//         return Vec3.newTemp(0.2F, 0.03F, 0.03F);
//       }
Vec3 *HellDimension::getFogColor(float time, float unknown)
{
	return Vec3::newTemp(0.2f, 0.03f, 0.03f);
}

// Beta 1.2: HellDimension.mayRespawn() - players can't respawn in Nether
// Java: public boolean mayRespawn() {
//         return false;
//       }
bool HellDimension::mayRespawn()
{
	return false;
}
