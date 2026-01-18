#include "world/level/tile/RailTile.h"

#include "world/level/Level.h"
#include "world/level/LevelSource.h"
#include "world/level/material/Material.h"
#include "world/level/material/DecorationMaterial.h"
#include "world/level/tile/Tile.h"
#include "world/phys/AABB.h"
#include "world/phys/HitResult.h"
#include "world/phys/Vec3.h"
#include "java/Random.h"

// Beta: RailTile(int var1, int var2) (RailTile.java:14-16)
RailTile::RailTile(int_t id, int_t tex) : Tile(id, tex, Material::decoration)
{
	// Beta: RailTile uses Material.decoration (RailTile.java:15)
	// Beta: this.setShape(0.0F, 0.0F, 0.0F, 1.0F, 0.125F, 1.0F) (RailTile.java:17)
	Tile::setShape(0.0F, 0.0F, 0.0F, 1.0F, 0.125F, 1.0F);
}

// Beta: RailTile.getAABB() - returns null (no collision) (RailTile.java:20-22)
AABB *RailTile::getAABB(Level &level, int_t x, int_t y, int_t z)
{
	return nullptr;  // Beta: return null (RailTile.java:22)
}

// Beta: RailTile.isSolidRender() - returns false (RailTile.java:30-32)
bool RailTile::isSolidRender()
{
	return false;  // Beta: return false (RailTile.java:31)
}

// Beta: RailTile.clip() - updates shape before clipping (RailTile.java:35-38)
HitResult RailTile::clip(Level &level, int_t x, int_t y, int_t z, Vec3 &from, Vec3 &to)
{
	// Beta: this.updateShape(var1, var2, var3, var4) (RailTile.java:36)
	updateShape(level, x, y, z);
	// Beta: return super.clip(var1, var2, var3, var4, var5, var6) (RailTile.java:37)
	return Tile::clip(level, x, y, z, from, to);
}

// Beta: RailTile.updateShape() - sets shape based on data (RailTile.java:41-48)
void RailTile::updateShape(LevelSource &level, int_t x, int_t y, int_t z)
{
	int_t data = level.getData(x, y, z);  // Beta: int var5 = var1.getData(var2, var3, var4) (RailTile.java:42)
	if (data >= 2 && data <= 5)  // Beta: if (var5 >= 2 && var5 <= 5) (RailTile.java:43)
	{
		// Beta: this.setShape(0.0F, 0.0F, 0.0F, 1.0F, 0.625F, 1.0F) (RailTile.java:44)
		Tile::setShape(0.0F, 0.0F, 0.0F, 1.0F, 0.625F, 1.0F);
	}
	else
	{
		// Beta: this.setShape(0.0F, 0.0F, 0.0F, 1.0F, 0.125F, 1.0F) (RailTile.java:46)
		Tile::setShape(0.0F, 0.0F, 0.0F, 1.0F, 0.125F, 1.0F);
	}
}

// Beta: RailTile.getTexture() - returns texture based on data (RailTile.java:51-53)
int_t RailTile::getTexture(Facing face, int_t data)
{
	// Beta: return var2 >= 6 ? this.tex - 16 : this.tex (RailTile.java:52)
	return data >= 6 ? tex - 16 : tex;
}

// Beta: RailTile.isCubeShaped() - returns false (RailTile.java:56-58)
bool RailTile::isCubeShaped()
{
	return false;  // Beta: return false (RailTile.java:57)
}

// Beta: RailTile.getRenderShape() - returns 9 (SHAPE_RAIL) (RailTile.java:61-63)
Tile::Shape RailTile::getRenderShape()
{
	return SHAPE_RAIL;  // Beta: return 9 (RailTile.java:62)
}

// Beta: RailTile.getResourceCount() - returns 1 (RailTile.java:66-68)
int_t RailTile::getResourceCount(Random &random)
{
	return 1;  // Beta: return 1 (RailTile.java:67)
}

// Beta: RailTile.mayPlace() - checks if can be placed on solid block (RailTile.java:71-73)
bool RailTile::mayPlace(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: return var1.isSolidTile(var2, var3 - 1, var4) (RailTile.java:72)
	return level.isSolidTile(x, y - 1, z);
}

// Beta: RailTile.onPlace() - updates rail connections (RailTile.java:76-81)
void RailTile::onPlace(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: if (!var1.isOnline) (RailTile.java:77)
	if (!level.isOnline)
	{
		// Beta: var1.setData(var2, var3, var4, 15) (RailTile.java:78)
		level.setData(x, y, z, 15);
		// Beta: this.updateDir(var1, var2, var3, var4) (RailTile.java:79)
		updateDir(level, x, y, z);
	}
}

// Beta: RailTile.neighborChanged() - updates rail connections when neighbors change (RailTile.java:84-115)
void RailTile::neighborChanged(Level &level, int_t x, int_t y, int_t z, int_t tile)
{
	// Beta: if (!var1.isOnline) (RailTile.java:85)
	if (!level.isOnline)
	{
		int_t data = level.getData(x, y, z);  // Beta: int var6 = var1.getData(var2, var3, var4) (RailTile.java:86)
		bool shouldBreak = false;  // Beta: boolean var7 = false (RailTile.java:87)
		
		// Beta: if (!var1.isSolidTile(var2, var3 - 1, var4)) (RailTile.java:88)
		if (!level.isSolidTile(x, y - 1, z))
		{
			shouldBreak = true;  // Beta: var7 = true (RailTile.java:89)
		}

		// Beta: if (var6 == 2 && !var1.isSolidTile(var2 + 1, var3, var4)) (RailTile.java:92)
		if (data == 2 && !level.isSolidTile(x + 1, y, z))
		{
			shouldBreak = true;  // Beta: var7 = true (RailTile.java:93)
		}

		// Beta: if (var6 == 3 && !var1.isSolidTile(var2 - 1, var3, var4)) (RailTile.java:96)
		if (data == 3 && !level.isSolidTile(x - 1, y, z))
		{
			shouldBreak = true;  // Beta: var7 = true (RailTile.java:97)
		}

		// Beta: if (var6 == 4 && !var1.isSolidTile(var2, var3, var4 - 1)) (RailTile.java:100)
		if (data == 4 && !level.isSolidTile(x, y, z - 1))
		{
			shouldBreak = true;  // Beta: var7 = true (RailTile.java:101)
		}

		// Beta: if (var6 == 5 && !var1.isSolidTile(var2, var3, var4 + 1)) (RailTile.java:104)
		if (data == 5 && !level.isSolidTile(x, y, z + 1))
		{
			shouldBreak = true;  // Beta: var7 = true (RailTile.java:105)
		}

		// Beta: if (var7) (RailTile.java:108)
		if (shouldBreak)
		{
			// Beta: this.spawnResources(var1, var2, var3, var4, var1.getData(var2, var3, var4)) (RailTile.java:109)
			spawnResources(level, x, y, z, level.getData(x, y, z));
			// Beta: var1.setTile(var2, var3, var4, 0) (RailTile.java:110)
			level.setTile(x, y, z, 0);
		}
		// Beta: else if (var5 > 0 && Tile.tiles[var5].isSignalSource() && new RailTile.Rail(var1, var2, var3, var4).countPotentialConnections() == 3) (RailTile.java:111)
		else if (tile > 0 && Tile::tiles[tile]->isSignalSource() && Rail(level, x, y, z).countPotentialConnections() == 3)
		{
			// Beta: this.updateDir(var1, var2, var3, var4) (RailTile.java:112)
			updateDir(level, x, y, z);
		}
	}
}

// Beta: RailTile.updateDir() - updates rail direction (RailTile.java:117-121)
void RailTile::updateDir(Level &level, int_t x, int_t y, int_t z)
{
	// Beta: if (!var1.isOnline) (RailTile.java:118)
	if (!level.isOnline)
	{
		// Beta: new RailTile.Rail(var1, var2, var3, var4).place(var1.hasNeighborSignal(var2, var3, var4)) (RailTile.java:119)
		Rail(level, x, y, z).place(level.hasNeighborSignal(x, y, z));
	}
}

// Beta: RailTile.Rail constructor (RailTile.java:131-138)
RailTile::Rail::Rail(Level &level, int_t x, int_t y, int_t z) : level(level), x(x), y(y), z(z)
{
	// Beta: this.level = var2 (RailTile.java:132)
	// Beta: this.x = var3 (RailTile.java:133)
	// Beta: this.y = var4 (RailTile.java:134)
	// Beta: this.z = var5 (RailTile.java:135)
	// Beta: this.data = var2.getData(var3, var4, var5) (RailTile.java:136)
	this->data = level.getData(x, y, z);
	// Beta: this.updateConnections() (RailTile.java:137)
	updateConnections();
}

// Beta: RailTile.Rail.updateConnections() (RailTile.java:140-173)
void RailTile::Rail::updateConnections()
{
	connections.clear();  // Beta: this.connections.clear() (RailTile.java:141)
	
	if (data == 0)  // Beta: if (this.data == 0) (RailTile.java:142)
	{
		connections.push_back(TilePos(x, y, z - 1));  // Beta: this.connections.add(new TilePos(this.x, this.y, this.z - 1)) (RailTile.java:143)
		connections.push_back(TilePos(x, y, z + 1));  // Beta: this.connections.add(new TilePos(this.x, this.y, this.z + 1)) (RailTile.java:144)
	}
	else if (data == 1)  // Beta: else if (this.data == 1) (RailTile.java:145)
	{
		connections.push_back(TilePos(x - 1, y, z));  // Beta: this.connections.add(new TilePos(this.x - 1, this.y, this.z)) (RailTile.java:146)
		connections.push_back(TilePos(x + 1, y, z));  // Beta: this.connections.add(new TilePos(this.x + 1, this.y, this.z)) (RailTile.java:147)
	}
	else if (data == 2)  // Beta: else if (this.data == 2) (RailTile.java:148)
	{
		connections.push_back(TilePos(x - 1, y, z));  // Beta: this.connections.add(new TilePos(this.x - 1, this.y, this.z)) (RailTile.java:149)
		connections.push_back(TilePos(x + 1, y + 1, z));  // Beta: this.connections.add(new TilePos(this.x + 1, this.y + 1, this.z)) (RailTile.java:150)
	}
	else if (data == 3)  // Beta: else if (this.data == 3) (RailTile.java:151)
	{
		connections.push_back(TilePos(x - 1, y + 1, z));  // Beta: this.connections.add(new TilePos(this.x - 1, this.y + 1, this.z)) (RailTile.java:152)
		connections.push_back(TilePos(x + 1, y, z));  // Beta: this.connections.add(new TilePos(this.x + 1, this.y, this.z)) (RailTile.java:153)
	}
	else if (data == 4)  // Beta: else if (this.data == 4) (RailTile.java:154)
	{
		connections.push_back(TilePos(x, y + 1, z - 1));  // Beta: this.connections.add(new TilePos(this.x, this.y + 1, this.z - 1)) (RailTile.java:155)
		connections.push_back(TilePos(x, y, z + 1));  // Beta: this.connections.add(new TilePos(this.x, this.y, this.z + 1)) (RailTile.java:156)
	}
	else if (data == 5)  // Beta: else if (this.data == 5) (RailTile.java:157)
	{
		connections.push_back(TilePos(x, y, z - 1));  // Beta: this.connections.add(new TilePos(this.x, this.y, this.z - 1)) (RailTile.java:158)
		connections.push_back(TilePos(x, y + 1, z + 1));  // Beta: this.connections.add(new TilePos(this.x, this.y + 1, this.z + 1)) (RailTile.java:159)
	}
	else if (data == 6)  // Beta: else if (this.data == 6) (RailTile.java:160)
	{
		connections.push_back(TilePos(x + 1, y, z));  // Beta: this.connections.add(new TilePos(this.x + 1, this.y, this.z)) (RailTile.java:161)
		connections.push_back(TilePos(x, y, z + 1));  // Beta: this.connections.add(new TilePos(this.x, this.y, this.z + 1)) (RailTile.java:162)
	}
	else if (data == 7)  // Beta: else if (this.data == 7) (RailTile.java:163)
	{
		connections.push_back(TilePos(x - 1, y, z));  // Beta: this.connections.add(new TilePos(this.x - 1, this.y, this.z)) (RailTile.java:164)
		connections.push_back(TilePos(x, y, z + 1));  // Beta: this.connections.add(new TilePos(this.x, this.y, this.z + 1)) (RailTile.java:165)
	}
	else if (data == 8)  // Beta: else if (this.data == 8) (RailTile.java:166)
	{
		connections.push_back(TilePos(x - 1, y, z));  // Beta: this.connections.add(new TilePos(this.x - 1, this.y, this.z)) (RailTile.java:167)
		connections.push_back(TilePos(x, y, z - 1));  // Beta: this.connections.add(new TilePos(this.x, this.y, this.z - 1)) (RailTile.java:168)
	}
	else if (data == 9)  // Beta: else if (this.data == 9) (RailTile.java:169)
	{
		connections.push_back(TilePos(x + 1, y, z));  // Beta: this.connections.add(new TilePos(this.x + 1, this.y, this.z)) (RailTile.java:170)
		connections.push_back(TilePos(x, y, z - 1));  // Beta: this.connections.add(new TilePos(this.x, this.y, this.z - 1)) (RailTile.java:171)
	}
}

// Beta: RailTile.Rail.removeSoftConnections() (RailTile.java:175-184)
void RailTile::Rail::removeSoftConnections()
{
	// Beta: for (int var1 = 0; var1 < this.connections.size(); var1++) (RailTile.java:176)
	for (size_t i = 0; i < connections.size(); i++)
	{
		Rail *other = getRail(connections[i]);  // Beta: RailTile.Rail var2 = this.getRail(this.connections.get(var1)) (RailTile.java:177)
		if (other != nullptr && other->connectsTo(*this))  // Beta: if (var2 != null && var2.connectsTo(this)) (RailTile.java:178)
		{
			connections[i] = TilePos(other->x, other->y, other->z);  // Beta: this.connections.set(var1, new TilePos(var2.x, var2.y, var2.z)) (RailTile.java:179)
		}
		else
		{
			connections.erase(connections.begin() + i);  // Beta: this.connections.remove(var1--) (RailTile.java:181)
			i--;  // Decrement index after erase
		}
	}
}

// Beta: RailTile.Rail.hasRail() (RailTile.java:186-192)
bool RailTile::Rail::hasRail(int_t x, int_t y, int_t z)
{
	int_t railId = Tile::rail.id;
	// Beta: if (this.level.getTile(var1, var2, var3) == RailTile.this.id) (RailTile.java:187)
	if (level.getTile(x, y, z) == railId)
	{
		return true;  // Beta: return true (RailTile.java:188)
	}
	else
	{
		// Beta: return this.level.getTile(var1, var2 + 1, var3) == RailTile.this.id ? true : this.level.getTile(var1, var2 - 1, var3) == RailTile.this.id (RailTile.java:190)
		return (level.getTile(x, y + 1, z) == railId) || 
		       (level.getTile(x, y - 1, z) == railId);
	}
}

// Beta: RailTile.Rail.getRail() (RailTile.java:194-202)
RailTile::Rail *RailTile::Rail::getRail(const TilePos &pos)
{
	int_t railId = Tile::rail.id;
	// Beta: if (this.level.getTile(var1.x, var1.y, var1.z) == RailTile.this.id) (RailTile.java:195)
	if (level.getTile(pos.x, pos.y, pos.z) == railId)
	{
		return new Rail(level, pos.x, pos.y, pos.z);  // Beta: return RailTile.this.new Rail(this.level, var1.x, var1.y, var1.z) (RailTile.java:196)
	}
	// Beta: else if (this.level.getTile(var1.x, var1.y + 1, var1.z) == RailTile.this.id) (RailTile.java:197)
	else if (level.getTile(pos.x, pos.y + 1, pos.z) == railId)
	{
		return new Rail(level, pos.x, pos.y + 1, pos.z);  // Beta: return RailTile.this.new Rail(this.level, var1.x, var1.y + 1, var1.z) (RailTile.java:198)
	}
	else
	{
		// Beta: return this.level.getTile(var1.x, var1.y - 1, var1.z) == RailTile.this.id ? RailTile.this.new Rail(this.level, var1.x, var1.y - 1, var1.z) : null (RailTile.java:200)
		if (level.getTile(pos.x, pos.y - 1, pos.z) == railId)
		{
			return new Rail(level, pos.x, pos.y - 1, pos.z);
		}
		return nullptr;  // Beta: return null
	}
}

// Beta: RailTile.Rail.connectsTo() (RailTile.java:204-213)
bool RailTile::Rail::connectsTo(Rail &other)
{
	// Beta: for (int var2 = 0; var2 < this.connections.size(); var2++) (RailTile.java:205)
	for (size_t i = 0; i < connections.size(); i++)
	{
		TilePos &pos = connections[i];  // Beta: TilePos var3 = this.connections.get(var2) (RailTile.java:206)
		// Beta: if (var3.x == var1.x && var3.z == var1.z) (RailTile.java:207)
		if (pos.x == other.x && pos.z == other.z)
		{
			return true;  // Beta: return true (RailTile.java:208)
		}
	}
	return false;  // Beta: return false (RailTile.java:212)
}

// Beta: RailTile.Rail.hasConnection() (RailTile.java:215-224)
bool RailTile::Rail::hasConnection(int_t x, int_t z)
{
	// Beta: for (int var4 = 0; var4 < this.connections.size(); var4++) (RailTile.java:216)
	for (size_t i = 0; i < connections.size(); i++)
	{
		TilePos &pos = connections[i];  // Beta: TilePos var5 = this.connections.get(var4) (RailTile.java:217)
		// Beta: if (var5.x == var1 && var5.z == var3) (RailTile.java:218)
		if (pos.x == x && pos.z == z)
		{
			return true;  // Beta: return true (RailTile.java:219)
		}
	}
	return false;  // Beta: return false (RailTile.java:223)
}

// Beta: RailTile.Rail.countPotentialConnections() (RailTile.java:226-245)
int_t RailTile::Rail::countPotentialConnections()
{
	int_t count = 0;  // Beta: int var1 = 0 (RailTile.java:227)
	
	// Beta: if (this.hasRail(this.x, this.y, this.z - 1)) (RailTile.java:228)
	if (hasRail(x, y, z - 1))
		count++;  // Beta: var1++ (RailTile.java:229)

	// Beta: if (this.hasRail(this.x, this.y, this.z + 1)) (RailTile.java:232)
	if (hasRail(x, y, z + 1))
		count++;  // Beta: var1++ (RailTile.java:233)

	// Beta: if (this.hasRail(this.x - 1, this.y, this.z)) (RailTile.java:236)
	if (hasRail(x - 1, y, z))
		count++;  // Beta: var1++ (RailTile.java:237)

	// Beta: if (this.hasRail(this.x + 1, this.y, this.z)) (RailTile.java:240)
	if (hasRail(x + 1, y, z))
		count++;  // Beta: var1++ (RailTile.java:241)

	return count;  // Beta: return var1 (RailTile.java:244)
}

// Beta: RailTile.Rail.canConnectTo() (RailTile.java:247-258)
bool RailTile::Rail::canConnectTo(Rail &other)
{
	// Beta: if (this.connectsTo(var1)) (RailTile.java:248)
	if (connectsTo(other))
		return true;  // Beta: return true (RailTile.java:249)
	// Beta: else if (this.connections.size() == 2) (RailTile.java:250)
	else if (connections.size() == 2)
		return false;  // Beta: return false (RailTile.java:251)
	// Beta: else if (this.connections.size() == 0) (RailTile.java:252)
	else if (connections.size() == 0)
		return true;  // Beta: return true (RailTile.java:253)
	else
	{
		TilePos &pos = connections[0];  // Beta: TilePos var2 = this.connections.get(0) (RailTile.java:255)
		// Beta: return var1.y == this.y && var2.y == this.y ? true : true (RailTile.java:256)
		return (other.y == y && pos.y == y) ? true : true;  // Always true in Java
	}
}

// Beta: RailTile.Rail.connectTo() (RailTile.java:260-316)
void RailTile::Rail::connectTo(Rail &other)
{
	connections.push_back(TilePos(other.x, other.y, other.z));  // Beta: this.connections.add(new TilePos(var1.x, var1.y, var1.z)) (RailTile.java:261)
	
	bool hasNorth = hasConnection(x, z - 1);  // Beta: boolean var2 = this.hasConnection(this.x, this.y, this.z - 1) (RailTile.java:262)
	bool hasSouth = hasConnection(x, z + 1);  // Beta: boolean var3 = this.hasConnection(this.x, this.y, this.z + 1) (RailTile.java:263)
	bool hasWest = hasConnection(x - 1, z);  // Beta: boolean var4 = this.hasConnection(this.x - 1, this.y, this.z) (RailTile.java:264)
	bool hasEast = hasConnection(x + 1, z);  // Beta: boolean var5 = this.hasConnection(this.x + 1, this.y, this.z) (RailTile.java:265)
	
	byte_t newData = -1;  // Beta: byte var6 = -1 (RailTile.java:266)
	
	if (hasNorth || hasSouth)  // Beta: if (var2 || var3) (RailTile.java:267)
		newData = 0;  // Beta: var6 = 0 (RailTile.java:268)

	if (hasWest || hasEast)  // Beta: if (var4 || var5) (RailTile.java:271)
		newData = 1;  // Beta: var6 = 1 (RailTile.java:272)

	if (hasSouth && hasEast && !hasNorth && !hasWest)  // Beta: if (var3 && var5 && !var2 && !var4) (RailTile.java:275)
		newData = 6;  // Beta: var6 = 6 (RailTile.java:276)

	if (hasSouth && hasWest && !hasNorth && !hasEast)  // Beta: if (var3 && var4 && !var2 && !var5) (RailTile.java:279)
		newData = 7;  // Beta: var6 = 7 (RailTile.java:280)

	if (hasNorth && hasWest && !hasSouth && !hasEast)  // Beta: if (var2 && var4 && !var3 && !var5) (RailTile.java:283)
		newData = 8;  // Beta: var6 = 8 (RailTile.java:284)

	if (hasNorth && hasEast && !hasSouth && !hasWest)  // Beta: if (var2 && var5 && !var3 && !var4) (RailTile.java:287)
		newData = 9;  // Beta: var6 = 9 (RailTile.java:288)

	int_t railId = Tile::rail.id;
	if (newData == 0)  // Beta: if (var6 == 0) (RailTile.java:291)
	{
		if (level.getTile(x, y + 1, z - 1) == railId)  // Beta: if (this.level.getTile(this.x, this.y + 1, this.z - 1) == RailTile.this.id) (RailTile.java:292)
			newData = 4;  // Beta: var6 = 4 (RailTile.java:293)

		if (level.getTile(x, y + 1, z + 1) == railId)  // Beta: if (this.level.getTile(this.x, this.y + 1, this.z + 1) == RailTile.this.id) (RailTile.java:296)
			newData = 5;  // Beta: var6 = 5 (RailTile.java:297)
	}

	if (newData == 1)  // Beta: if (var6 == 1) (RailTile.java:301)
	{
		if (level.getTile(x + 1, y + 1, z) == railId)  // Beta: if (this.level.getTile(this.x + 1, this.y + 1, this.z) == RailTile.this.id) (RailTile.java:302)
			newData = 2;  // Beta: var6 = 2 (RailTile.java:303)

		if (level.getTile(x - 1, y + 1, z) == railId)  // Beta: if (this.level.getTile(this.x - 1, this.y + 1, this.z) == RailTile.this.id) (RailTile.java:306)
			newData = 3;  // Beta: var6 = 3 (RailTile.java:307)
	}

	if (newData < 0)  // Beta: if (var6 < 0) (RailTile.java:311)
		newData = 0;  // Beta: var6 = 0 (RailTile.java:312)

	level.setData(this->x, this->y, this->z, newData);  // Beta: this.level.setData(this.x, this.y, this.z, var6) (RailTile.java:315)
}

// Beta: RailTile.Rail.hasNeighborRail() (RailTile.java:318-326)
bool RailTile::Rail::hasNeighborRail(int_t x, int_t y, int_t z)
{
	Rail *other = getRail(TilePos(x, y, z));  // Beta: RailTile.Rail var4 = this.getRail(new TilePos(var1, var2, var3)) (RailTile.java:319)
	if (other == nullptr)  // Beta: if (var4 == null) (RailTile.java:320)
		return false;  // Beta: return false (RailTile.java:321)
	else
	{
		other->removeSoftConnections();  // Beta: var4.removeSoftConnections() (RailTile.java:323)
		return other->canConnectTo(*this);  // Beta: return var4.canConnectTo(this) (RailTile.java:324)
	}
}

// Beta: RailTile.Rail.place() (RailTile.java:328-439)
void RailTile::Rail::place(bool hasSignal)
{
	bool hasNorth = hasNeighborRail(x, y, z - 1);  // Beta: boolean var2 = this.hasNeighborRail(this.x, this.y, this.z - 1) (RailTile.java:329)
	bool hasSouth = hasNeighborRail(x, y, z + 1);  // Beta: boolean var3 = this.hasNeighborRail(this.x, this.y, this.z + 1) (RailTile.java:330)
	bool hasWest = hasNeighborRail(x - 1, y, z);  // Beta: boolean var4 = this.hasNeighborRail(this.x - 1, this.y, this.z) (RailTile.java:331)
	bool hasEast = hasNeighborRail(x + 1, y, z);  // Beta: boolean var5 = this.hasNeighborRail(this.x + 1, this.y, this.z) (RailTile.java:332)
	
	byte_t newData = -1;  // Beta: byte var6 = -1 (RailTile.java:333)
	
	if ((hasNorth || hasSouth) && !hasWest && !hasEast)  // Beta: if ((var2 || var3) && !var4 && !var5) (RailTile.java:334)
		newData = 0;  // Beta: var6 = 0 (RailTile.java:335)

	if ((hasWest || hasEast) && !hasNorth && !hasSouth)  // Beta: if ((var4 || var5) && !var2 && !var3) (RailTile.java:338)
		newData = 1;  // Beta: var6 = 1 (RailTile.java:339)

	if (hasSouth && hasEast && !hasNorth && !hasWest)  // Beta: if (var3 && var5 && !var2 && !var4) (RailTile.java:342)
		newData = 6;  // Beta: var6 = 6 (RailTile.java:343)

	if (hasSouth && hasWest && !hasNorth && !hasEast)  // Beta: if (var3 && var4 && !var2 && !var5) (RailTile.java:346)
		newData = 7;  // Beta: var6 = 7 (RailTile.java:347)

	if (hasNorth && hasWest && !hasSouth && !hasEast)  // Beta: if (var2 && var4 && !var3 && !var5) (RailTile.java:350)
		newData = 8;  // Beta: var6 = 8 (RailTile.java:351)

	if (hasNorth && hasEast && !hasSouth && !hasWest)  // Beta: if (var2 && var5 && !var3 && !var4) (RailTile.java:354)
		newData = 9;  // Beta: var6 = 9 (RailTile.java:355)

	if (newData == -1)  // Beta: if (var6 == -1) (RailTile.java:358)
	{
		if (hasNorth || hasSouth)  // Beta: if (var2 || var3) (RailTile.java:359)
			newData = 0;  // Beta: var6 = 0 (RailTile.java:360)

		if (hasWest || hasEast)  // Beta: if (var4 || var5) (RailTile.java:363)
			newData = 1;  // Beta: var6 = 1 (RailTile.java:364)

		if (hasSignal)  // Beta: if (var1) (RailTile.java:367)
		{
			if (hasSouth && hasEast)  // Beta: if (var3 && var5) (RailTile.java:368)
				newData = 6;  // Beta: var6 = 6 (RailTile.java:369)

			if (hasWest && hasSouth)  // Beta: if (var4 && var3) (RailTile.java:372)
				newData = 7;  // Beta: var6 = 7 (RailTile.java:373)

			if (hasEast && hasNorth)  // Beta: if (var5 && var2) (RailTile.java:376)
				newData = 9;  // Beta: var6 = 9 (RailTile.java:377)

			if (hasNorth && hasWest)  // Beta: if (var2 && var4) (RailTile.java:380)
				newData = 8;  // Beta: var6 = 8 (RailTile.java:381)
		}
		else
		{
			if (hasNorth && hasWest)  // Beta: if (var2 && var4) (RailTile.java:384)
				newData = 8;  // Beta: var6 = 8 (RailTile.java:385)

			if (hasEast && hasNorth)  // Beta: if (var5 && var2) (RailTile.java:388)
				newData = 9;  // Beta: var6 = 9 (RailTile.java:389)

			if (hasWest && hasSouth)  // Beta: if (var4 && var3) (RailTile.java:392)
				newData = 7;  // Beta: var6 = 7 (RailTile.java:393)

			if (hasSouth && hasEast)  // Beta: if (var3 && var5) (RailTile.java:396)
				newData = 6;  // Beta: var6 = 6 (RailTile.java:397)
		}
	}

	int_t railId2 = Tile::rail.id;
	if (newData == 0)  // Beta: if (var6 == 0) (RailTile.java:402)
	{
		if (level.getTile(x, y + 1, z - 1) == railId2)  // Beta: if (this.level.getTile(this.x, this.y + 1, this.z - 1) == RailTile.this.id) (RailTile.java:403)
			newData = 4;  // Beta: var6 = 4 (RailTile.java:404)

		if (level.getTile(x, y + 1, z + 1) == railId2)  // Beta: if (this.level.getTile(this.x, this.y + 1, this.z + 1) == RailTile.this.id) (RailTile.java:407)
			newData = 5;  // Beta: var6 = 5 (RailTile.java:408)
	}

	if (newData == 1)  // Beta: if (var6 == 1) (RailTile.java:412)
	{
		if (level.getTile(x + 1, y + 1, z) == railId2)  // Beta: if (this.level.getTile(this.x + 1, this.y + 1, this.z) == RailTile.this.id) (RailTile.java:413)
			newData = 2;  // Beta: var6 = 2 (RailTile.java:414)

		if (level.getTile(x - 1, y + 1, z) == railId2)  // Beta: if (this.level.getTile(this.x - 1, this.y + 1, this.z) == RailTile.this.id) (RailTile.java:417)
			newData = 3;  // Beta: var6 = 3 (RailTile.java:418)
	}

	if (newData < 0)  // Beta: if (var6 < 0) (RailTile.java:422)
		newData = 0;  // Beta: var6 = 0 (RailTile.java:423)

	this->data = newData;  // Beta: this.data = var6 (RailTile.java:426)
	updateConnections();  // Beta: this.updateConnections() (RailTile.java:427)
	level.setData(this->x, this->y, this->z, newData);  // Beta: this.level.setData(this.x, this.y, this.z, var6) (RailTile.java:428)

	// Beta: for (int var7 = 0; var7 < this.connections.size(); var7++) (RailTile.java:430)
	for (size_t i = 0; i < connections.size(); i++)
	{
		Rail *other = getRail(connections[i]);  // Beta: RailTile.Rail var8 = this.getRail(this.connections.get(var7)) (RailTile.java:431)
		if (other != nullptr)  // Beta: if (var8 != null) (RailTile.java:432)
		{
			other->removeSoftConnections();  // Beta: var8.removeSoftConnections() (RailTile.java:433)
			if (other->canConnectTo(*this))  // Beta: if (var8.canConnectTo(this)) (RailTile.java:434)
			{
				other->connectTo(*this);  // Beta: var8.connectTo(this) (RailTile.java:435)
			}
		}
	}
}
