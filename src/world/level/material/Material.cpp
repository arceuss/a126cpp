#include "world/level/material/Material.h"

#include "world/level/material/GasMaterial.h"
#include "world/level/material/LiquidMaterial.h"
#include "world/level/material/PlantMaterial.h"
#include "world/level/material/DecorationMaterial.h"

GasMaterial Material::air;
Material Material::ground;  // Alpha Material.ground - base material for dirt/grass (harvestable=true)
Material Material::dirt;    // Legacy/compat (harvestable=true)
Material Material::wood = Material().flammable();  // Alpha Material.wood - harvestable=true
Material Material::stone = Material().setNoHarvest();  // Alpha Material.rock - setNoHarvest() (Material.java:7)
Material Material::metal;  // Beta 1.2: Material.metal (Material.java:8) - harvestable=true
Material Material::leaves = Material().flammable();  // Alpha Material.leaves - harvestable=true

Material Material::sand;  // Alpha Material.sand - harvestable=true
Material Material::cactus;  // Alpha Material.cactus (Material.java:24) - harvestable=true
Material Material::sponge;  // Beta 1.2: Material.sponge (Material.java:13) - harvestable=true
Material Material::cloth = Material().flammable();  // Beta 1.2: Material.cloth (Material.java:14) - flammable, harvestable=true
Material Material::glass;   // Beta 1.2: Material.glass (Material.java:18) - harvestable=true
Material Material::explosive = Material().flammable();  // Beta 1.2: Material.explosive (Material.java:19) - flammable, harvestable=true
Material Material::cake;   // Beta 1.2: Material.cake (Material.java:28) - harvestable=true
Material Material::vegetable;  // Beta 1.2: Material.vegetable (Material.java:26) - harvestable=true
Material Material::portal;     // Beta 1.2: Material.portal (Material.java:27) - harvestable=true

LiquidMaterial Material::water;  // Alpha: Material.water (Material.java:13)
LiquidMaterial Material::lava;   // Alpha: Material.lava (Material.java:14)

GasMaterial Material::fire;  // Beta 1.2: Material.fire (Material.java:15) - GasMaterial

Material Material::ice;  // Alpha: Material.ice (Material.java:21) - harvestable=true
Material Material::clay; // Alpha: Material.clay (Material.java:25) - harvestable=true
Material Material::snow = Material().setNoHarvest(); // Alpha: Material.snow (Material.java:22) - setNoHarvest()
DecorationMaterial Material::topSnow;  // Beta 1.2: Material.topSnow (Material.java:22) - DecorationMaterial
DecorationMaterial Material::plant;    // Beta 1.2: Material.plant (Material.java:12) - DecorationMaterial
DecorationMaterial Material::decoration;  // Beta 1.2: Material.decoration (Material.java:17) - DecorationMaterial

// PlantMaterial accessor implementation is in PlantMaterial.cpp

bool Material::isLiquid() const
{
	return false;
}

bool Material::letsWaterThrough() const
{
	return (!isLiquid() && !isSolid());
}

bool Material::isSolid() const
{
	return true;
}

bool Material::blocksLight() const
{
	return true;
}

bool Material::blocksMotion() const
{
	return true;
}

Material &Material::flammable()
{
	flammableFlag = true;
	return *this;
}

Material &Material::setNoHarvest()
{
	// Alpha: setNoHarvest() sets canHarvest = false (Material.java:47-50)
	canHarvest = false;
	return *this;
}

bool Material::isFlammable()
{
	return flammableFlag;
}
