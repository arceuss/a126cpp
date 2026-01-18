#pragma once

class DecorationMaterial;
class GasMaterial;
class LiquidMaterial;

// Forward declaration for static members

class Material
{
public:
	static GasMaterial air;
	static Material ground;  // Base material for dirt/grass (Alpha Material.ground)
	static Material dirt;    // Legacy/compat - prefer ground
	static Material wood;
	static Material stone;
	static Material metal;   // Beta 1.2: Material.metal (Material.java:8)
	static Material leaves;  // Leaves material (flammable)

	static Material sand;
	static Material cactus;  // Alpha: Material.cactus (Material.java:24)
	static Material sponge;  // Beta 1.2: Material.sponge (Material.java:13)
	static Material cloth;  // Beta 1.2: Material.cloth (Material.java:14) - flammable
	static Material glass;   // Beta 1.2: Material.glass (Material.java:18)
	static Material explosive;  // Beta 1.2: Material.explosive (Material.java:19) - flammable
	static Material cake;    // Beta 1.2: Material.cake (Material.java:28)
	static Material vegetable;  // Beta 1.2: Material.vegetable (Material.java:26) - for pumpkin blocks
	static Material portal;     // Beta 1.2: Material.portal (Material.java:27) - for portal blocks
	
	// Liquid materials
	static LiquidMaterial water;  // Alpha: Material.water (Material.java:13)
	static LiquidMaterial lava;   // Alpha: Material.lava (Material.java:14)
	
	// Gas materials
	static GasMaterial fire;  // Beta 1.2: Material.fire (Material.java:15) - GasMaterial
	
	// Special materials
	static Material ice;  // Alpha: Material.ice (Material.java:21)
	static Material clay; // Alpha: Material.clay (Material.java:25)
	static Material snow; // Alpha: Material.snow (Material.java:22)
	static DecorationMaterial topSnow;  // Beta 1.2: Material.topSnow (Material.java:22) - DecorationMaterial
	static DecorationMaterial plant;     // Beta 1.2: Material.plant (Material.java:12) - DecorationMaterial
	static DecorationMaterial decoration;  // Beta 1.2: Material.decoration (Material.java:17) - DecorationMaterial
	
	// Plant materials - PlantMaterial is defined in PlantMaterial.h (not nested)
	static const Material &getPlantsMaterial();  // Alpha: Material.plants (Material.java:12) - accessor to avoid incomplete type issues

private:
	bool flammableFlag = false;
	bool canHarvest = true;  // Alpha: canHarvest default true (Material.java:29)

public:
	virtual ~Material() {}

	virtual bool isLiquid() const;
	virtual bool letsWaterThrough() const;
	virtual bool isSolid() const;
	virtual bool blocksLight() const;
	virtual bool blocksMotion() const;

protected:
	Material &flammable();
	Material &setNoHarvest();  // Alpha: setNoHarvest() sets canHarvest = false (Material.java:47-50)

public:
	virtual bool isFlammable();
	bool getIsHarvestable() const { return canHarvest; }  // Alpha: getIsHarvestable() (Material.java:61-63)
};

// Identity comparison operators - Material instances are singletons, so compare by address
// Free functions to handle comparisons with derived types (DecorationMaterial, LiquidMaterial, etc.)
inline bool operator==(const Material &lhs, const Material &rhs) { return &lhs == &rhs; }
inline bool operator!=(const Material &lhs, const Material &rhs) { return &lhs != &rhs; }