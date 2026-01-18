#pragma once

#include "world/level/tile/Tile.h"
#include "world/level/TilePos.h"
#include <vector>

// Beta 1.2: RailTile.java - ID 66
// Alpha 1.2.6: Rail blocks exist
class RailTile : public Tile
{
public:
	RailTile(int_t id, int_t tex);

	// Beta: RailTile.getAABB() - returns null (no collision) (RailTile.java:20-22)
	virtual AABB *getAABB(Level &level, int_t x, int_t y, int_t z) override;

	// Beta: RailTile.blocksLight() - returns false (RailTile.java:25-27)
	bool blocksLight() { return false; }

	// Beta: RailTile.isSolidRender() - returns false (RailTile.java:30-32)
	virtual bool isSolidRender() override;

	// Beta: RailTile.clip() - updates shape before clipping (RailTile.java:35-38)
	virtual HitResult clip(Level &level, int_t x, int_t y, int_t z, Vec3 &from, Vec3 &to) override;

	// Beta: RailTile.updateShape() - sets shape based on data (RailTile.java:41-48)
	virtual void updateShape(LevelSource &level, int_t x, int_t y, int_t z) override;

	// Beta: RailTile.getTexture() - returns texture based on data (RailTile.java:51-53)
	virtual int_t getTexture(Facing face, int_t data) override;

	// Beta: RailTile.isCubeShaped() - returns false (RailTile.java:56-58)
	virtual bool isCubeShaped() override;

	// Beta: RailTile.getRenderShape() - returns 9 (SHAPE_RAIL) (RailTile.java:61-63)
	virtual Shape getRenderShape() override;

	// Beta: RailTile.getResourceCount() - returns 1 (RailTile.java:66-68)
	virtual int_t getResourceCount(Random &random) override;

	// Beta: RailTile.mayPlace() - checks if can be placed on solid block (RailTile.java:71-73)
	virtual bool mayPlace(Level &level, int_t x, int_t y, int_t z) override;

	// Beta: RailTile.onPlace() - updates rail connections (RailTile.java:76-81)
	virtual void onPlace(Level &level, int_t x, int_t y, int_t z) override;

	// Beta: RailTile.neighborChanged() - updates rail connections when neighbors change (RailTile.java:84-115)
	virtual void neighborChanged(Level &level, int_t x, int_t y, int_t z, int_t tile) override;

private:
	// Beta: RailTile.updateDir() - updates rail direction (RailTile.java:117-121)
	void updateDir(Level &level, int_t x, int_t y, int_t z);

	// Beta: RailTile.Rail - inner class for rail connection logic (RailTile.java:123-440)
	class Rail
	{
	public:
		Rail(Level &level, int_t x, int_t y, int_t z);
		void place(bool hasSignal);

	public:
		int_t countPotentialConnections();

	private:
		void updateConnections();
		void removeSoftConnections();
		bool hasRail(int_t x, int_t y, int_t z);
		Rail *getRail(const TilePos &pos);
		bool connectsTo(Rail &other);
		bool hasConnection(int_t x, int_t z);
		bool canConnectTo(Rail &other);
		void connectTo(Rail &other);
		bool hasNeighborRail(int_t x, int_t y, int_t z);

		Level &level;
		int_t x;
		int_t y;
		int_t z;
		int_t data;
		std::vector<TilePos> connections;
	};
};
