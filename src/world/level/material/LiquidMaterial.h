#pragma once

#include "world/level/material/Material.h"

// Alpha 1.2.6 LiquidMaterial (water/lava)
// Reference: apclient/minecraft/src/net/minecraft/src/Material.java
class LiquidMaterial : public Material
{
public:
	bool isLiquid() const override;
	bool isSolid() const override;
	bool blocksMotion() const override;
};

// Overloads for LiquidMaterial comparisons (help MSVC with overload resolution)
inline bool operator==(const Material &lhs, const LiquidMaterial &rhs) { return &lhs == static_cast<const Material*>(&rhs); }
inline bool operator==(const LiquidMaterial &lhs, const Material &rhs) { return static_cast<const Material*>(&lhs) == &rhs; }
inline bool operator!=(const Material &lhs, const LiquidMaterial &rhs) { return &lhs != static_cast<const Material*>(&rhs); }
inline bool operator!=(const LiquidMaterial &lhs, const Material &rhs) { return static_cast<const Material*>(&lhs) != &rhs; }