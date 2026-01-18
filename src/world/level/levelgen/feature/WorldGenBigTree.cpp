#include "world/level/levelgen/feature/WorldGenBigTree.h"
#include "world/level/Level.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/GrassTile.h"
#include "world/level/tile/DirtTile.h"
#include "world/level/tile/TreeTile.h"
#include "world/level/tile/LeafTile.h"
#include "util/Mth.h"
#include <cmath>
#include <algorithm>

// Beta 1.2 BasicTree - ported 1:1
WorldGenBigTree::WorldGenBigTree()
{
}

void WorldGenBigTree::init(double scale, double branchDensity, double foliageDensity)
{
	// Beta: BasicTree.init() - this.heightVariance = (int)(var1 * 12.0);
	this->heightVariance = static_cast<int_t>(scale * 12.0);
	// Beta: if (var1 > 0.5) { this.foliageHeight = 5; }
	if (scale > 0.5)
	{
		this->foliageHeight = 5;
	}
	
	// Beta: this.widthScale = var3; this.foliageDensity = var5;
	this->widthScale = branchDensity;
	this->foliageDensity = foliageDensity;
}

void WorldGenBigTree::prepare()
{
	// Beta: BasicTree.prepare()
	this->trunkHeight = static_cast<int_t>(this->height * this->trunkHeightScale);
	if (this->trunkHeight >= this->height)
	{
		this->trunkHeight = this->height - 1;
	}

	int_t var1 = static_cast<int_t>(1.382 + std::pow(this->foliageDensity * this->height / 13.0, 2.0));
	if (var1 < 1)
	{
		var1 = 1;
	}

	std::vector<std::array<int_t, 4>> var2(var1 * this->height);
	int_t var3 = this->origin[1] + this->height - this->foliageHeight;
	int_t var4 = 1;
	int_t var5 = this->origin[1] + this->trunkHeight;
	int_t var6 = var3 - this->origin[1];
	var2[0][0] = this->origin[0];
	var2[0][1] = var3;
	var2[0][2] = this->origin[2];
	var2[0][3] = var5;
	var3--;

	while (var6 >= 0)
	{
		int_t var7 = 0;
		float var8 = this->treeShape(var6);
		if (var8 < 0.0F)
		{
			var3--;
			var6--;
		}
		else
		{
			for (double var9 = 0.5; var7 < var1; var7++)
			{
				double var11 = this->widthScale * (var8 * (this->rnd.nextFloat() + 0.328));
				double var13 = this->rnd.nextFloat() * 2.0 * 3.14159;
				int_t var15 = static_cast<int_t>(var11 * std::sin(var13) + this->origin[0] + var9);
				int_t var16 = static_cast<int_t>(var11 * std::cos(var13) + this->origin[2] + var9);
				int_t var17[3] = {var15, var3, var16};
				int_t var18[3] = {var15, var3 + this->foliageHeight, var16};
				if (this->checkLine(var17, var18) == -1)
				{
					int_t var19[3] = {this->origin[0], this->origin[1], this->origin[2]};
					double var20 = std::sqrt(std::pow(static_cast<double>(std::abs(this->origin[0] - var17[0])), 2.0) + std::pow(static_cast<double>(std::abs(this->origin[2] - var17[2])), 2.0));
					double var22 = var20 * this->branchSlope;
					if (static_cast<double>(var17[1]) - var22 > static_cast<double>(var5))
					{
						var19[1] = var5;
					}
					else
					{
						var19[1] = static_cast<int_t>(static_cast<double>(var17[1]) - var22);
					}

					if (this->checkLine(var19, var17) == -1)
					{
						var2[var4][0] = var15;
						var2[var4][1] = var3;
						var2[var4][2] = var16;
						var2[var4][3] = var19[1];
						var4++;
					}
				}
			}

			var3--;
			var6--;
		}
	}

	this->foliageCoords.resize(var4);
	for (int_t i = 0; i < var4; i++)
	{
		this->foliageCoords[i] = var2[i];
	}
}

void WorldGenBigTree::crossection(int_t x, int_t y, int_t z, float radius, byte_t axis, int_t blockId)
{
	// Beta: BasicTree.crossection()
	int_t var7 = static_cast<int_t>(radius + 0.618);
	byte_t var8 = axisConversionArray[axis];
	byte_t var9 = axisConversionArray[axis + 3];
	int_t var10[3] = {x, y, z};
	int_t var11[3] = {0, 0, 0};
	int_t var12 = -var7;
	int_t var13 = -var7;

	for (var11[axis] = var10[axis]; var12 <= var7; var12++)
	{
		var11[var8] = var10[var8] + var12;
		var13 = -var7;

		while (var13 <= var7)
		{
			double var15 = std::sqrt(std::pow(static_cast<double>(std::abs(var12)) + 0.5, 2.0) + std::pow(static_cast<double>(std::abs(var13)) + 0.5, 2.0));
			if (var15 > static_cast<double>(radius))
			{
				var13++;
			}
			else
			{
				var11[var9] = var10[var9] + var13;
				int_t var14 = this->thisLevel->getTile(var11[0], var11[1], var11[2]);
				if (var14 != 0 && var14 != Tile::leaves.id)
				{
					var13++;
				}
				else
				{
					this->thisLevel->setTileNoUpdate(var11[0], var11[1], var11[2], blockId);
					var13++;
				}
			}
		}
	}
}

float WorldGenBigTree::treeShape(int_t offset)
{
	// Beta: BasicTree.treeShape()
	if (static_cast<double>(offset) < static_cast<double>(this->height) * 0.3)
	{
		return -1.618F;
	}
	else
	{
		float var2 = static_cast<float>(this->height) / 2.0F;
		float var3 = static_cast<float>(this->height) / 2.0F - static_cast<float>(offset);
		float var4;
		if (var3 == 0.0F)
		{
			var4 = var2;
		}
		else if (std::abs(var3) >= var2)
		{
			var4 = 0.0F;
		}
		else
		{
			var4 = static_cast<float>(std::sqrt(std::pow(static_cast<double>(std::abs(var2)), 2.0) - std::pow(static_cast<double>(std::abs(var3)), 2.0)));
		}

		return var4 * 0.5F;
	}
}

float WorldGenBigTree::foliageShape(int_t offset)
{
	// Beta: BasicTree.foliageShape()
	if (offset < 0 || offset >= this->foliageHeight)
	{
		return -1.0F;
	}
	else
	{
		return (offset != 0 && offset != this->foliageHeight - 1) ? 3.0F : 2.0F;
	}
}

void WorldGenBigTree::foliageCluster(int_t x, int_t y, int_t z)
{
	// Beta: BasicTree.foliageCluster()
	int_t var4 = y;

	for (int_t var5 = y + this->foliageHeight; var4 < var5; var4++)
	{
		float var6 = this->foliageShape(var4 - y);
		this->crossection(x, var4, z, var6, static_cast<byte_t>(1), Tile::leaves.id);
	}
}

void WorldGenBigTree::limb(int_t *start, int_t *end, int_t blockId)
{
	// Beta: BasicTree.limb()
	int_t var4[3] = {0, 0, 0};
	byte_t var5 = 0;

	byte_t var6;
	for (var6 = 0; var5 < 3; var5++)
	{
		var4[var5] = end[var5] - start[var5];
		if (std::abs(var4[var5]) > std::abs(var4[var6]))
		{
			var6 = var5;
		}
	}

	if (var4[var6] != 0)
	{
		byte_t var7 = axisConversionArray[var6];
		byte_t var8 = axisConversionArray[var6 + 3];
		byte_t var9;
		if (var4[var6] > 0)
		{
			var9 = 1;
		}
		else
		{
			var9 = -1;
		}

		double var10 = static_cast<double>(var4[var7]) / var4[var6];
		double var12 = static_cast<double>(var4[var8]) / var4[var6];
		int_t var14[3] = {0, 0, 0};
		byte_t var15 = 0;

		for (int_t var16 = var4[var6] + var9; var15 != var16; var15 += var9)
		{
			var14[var6] = Mth::floor(static_cast<double>(start[var6] + var15) + 0.5);
			var14[var7] = Mth::floor(static_cast<double>(start[var7]) + static_cast<double>(var15) * var10 + 0.5);
			var14[var8] = Mth::floor(static_cast<double>(start[var8]) + static_cast<double>(var15) * var12 + 0.5);
			this->thisLevel->setTileNoUpdate(var14[0], var14[1], var14[2], blockId);
		}
	}
}

void WorldGenBigTree::makeFoliage()
{
	// Beta: BasicTree.makeFoliage()
	int_t var1 = 0;

	for (size_t var2 = this->foliageCoords.size(); var1 < static_cast<int_t>(var2); var1++)
	{
		int_t var3 = this->foliageCoords[var1][0];
		int_t var4 = this->foliageCoords[var1][1];
		int_t var5 = this->foliageCoords[var1][2];
		this->foliageCluster(var3, var4, var5);
	}
}

bool WorldGenBigTree::trimBranches(int_t offset)
{
	// Beta: BasicTree.trimBranches()
	return !(offset < this->height * 0.2);
}

void WorldGenBigTree::makeTrunk()
{
	// Beta: BasicTree.makeTrunk()
	int_t var1 = this->origin[0];
	int_t var2 = this->origin[1];
	int_t var3 = this->origin[1] + this->trunkHeight;
	int_t var4 = this->origin[2];
	int_t var5[3] = {var1, var2, var4};
	int_t var6[3] = {var1, var3, var4};
	this->limb(var5, var6, Tile::treeTrunk.id);
	if (this->trunkWidth == 2)
	{
		var5[0]++;
		var6[0]++;
		this->limb(var5, var6, Tile::treeTrunk.id);
		var5[2]++;
		var6[2]++;
		this->limb(var5, var6, Tile::treeTrunk.id);
		var5[0] += -1;
		var6[0] += -1;
		this->limb(var5, var6, Tile::treeTrunk.id);
	}
}

void WorldGenBigTree::makeBranches()
{
	// Beta: BasicTree.makeBranches()
	int_t var1 = 0;
	size_t var2 = this->foliageCoords.size();

	int_t var3[3] = {this->origin[0], this->origin[1], this->origin[2]};
	for (; var1 < static_cast<int_t>(var2); var1++)
	{
		std::array<int_t, 4> &var4 = this->foliageCoords[var1];
		int_t var5[3] = {var4[0], var4[1], var4[2]};
		var3[1] = var4[3];
		int_t var6 = var3[1] - this->origin[1];
		if (this->trimBranches(var6))
		{
			this->limb(var3, var5, Tile::treeTrunk.id);
		}
	}
}

int_t WorldGenBigTree::checkLine(int_t *start, int_t *end)
{
	// Beta: BasicTree.checkLine()
	int_t var3[3] = {0, 0, 0};
	byte_t var4 = 0;

	byte_t var5;
	for (var5 = 0; var4 < 3; var4++)
	{
		var3[var4] = end[var4] - start[var4];
		if (std::abs(var3[var4]) > std::abs(var3[var5]))
		{
			var5 = var4;
		}
	}

	if (var3[var5] == 0)
	{
		return -1;
	}
	else
	{
		byte_t var6 = axisConversionArray[var5];
		byte_t var7 = axisConversionArray[var5 + 3];
		byte_t var8;
		if (var3[var5] > 0)
		{
			var8 = 1;
		}
		else
		{
			var8 = -1;
		}

		double var9 = static_cast<double>(var3[var6]) / var3[var5];
		double var11 = static_cast<double>(var3[var7]) / var3[var5];
		int_t var13[3] = {0, 0, 0};
		byte_t var14 = 0;

		int_t var15;
		for (var15 = var3[var5] + var8; var14 != var15; var14 += var8)
		{
			var13[var5] = start[var5] + var14;
			var13[var6] = static_cast<int_t>(static_cast<double>(start[var6]) + static_cast<double>(var14) * var9);
			var13[var7] = static_cast<int_t>(static_cast<double>(start[var7]) + static_cast<double>(var14) * var11);
			int_t var16 = this->thisLevel->getTile(var13[0], var13[1], var13[2]);
			if (var16 != 0 && var16 != Tile::leaves.id)
			{
				break;
			}
		}

		return (var14 == var15) ? -1 : std::abs(static_cast<int_t>(var14));
	}
}

bool WorldGenBigTree::checkLocation()
{
	// Beta: BasicTree.checkLocation()
	int_t var1[3] = {this->origin[0], this->origin[1], this->origin[2]};
	int_t var2[3] = {this->origin[0], this->origin[1] + this->height - 1, this->origin[2]};
	int_t var3 = this->thisLevel->getTile(this->origin[0], this->origin[1] - 1, this->origin[2]);
	if (var3 != Tile::grass.id && var3 != Tile::dirt.id)
	{
		return false;
	}
	else
	{
		int_t var4 = this->checkLine(var1, var2);
		if (var4 == -1)
		{
			return true;
		}
		else if (var4 < 6)
		{
			return false;
		}
		else
		{
			this->height = var4;
			return true;
		}
	}
}

bool WorldGenBigTree::place(Level &level, Random &random, int_t x, int_t y, int_t z)
{
	// Beta: BasicTree.place()
	this->thisLevel = &level;
	long var6 = random.nextLong();
	this->rnd.setSeed(var6);
	this->origin[0] = x;
	this->origin[1] = y;
	this->origin[2] = z;
	if (this->height == 0)
	{
		this->height = 5 + this->rnd.nextInt(this->heightVariance);
	}

	if (!this->checkLocation())
	{
		return false;
	}
	else
	{
		this->prepare();
		this->makeFoliage();
		this->makeTrunk();
		this->makeBranches();
		return true;
	}
}
