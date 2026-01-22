#pragma once

#include "world/level/material/Material.h"

// Alpha: Material.plants = new MaterialLogic()
// Reference: apclient/minecraft/src/net/minecraft/src/Material.java:12
// For now, using a simple material since MaterialLogic just returns different flags
class PlantMaterial : public Material
{
public:
	virtual bool isLiquid() const override { return false; }
	virtual bool letsWaterThrough() const override { return true; }
	virtual bool isSolid() const override { return false; }  // Alpha: MaterialLogic.isSolid() returns false
	virtual bool blocksLight() const override { return false; }
	virtual bool blocksMotion() const override { return false; }
};
