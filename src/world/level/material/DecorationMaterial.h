#pragma once

#include "world/level/material/Material.h"

// Beta 1.2: DecorationMaterial.java
// Used for plant, topSnow, decoration materials
// Overrides: isSolid()=false, blocksLight()=false, blocksMotion()=false
class DecorationMaterial : public Material
{
public:
	virtual bool isSolid() const override { return false; }
	virtual bool blocksLight() const override { return false; }
	virtual bool blocksMotion() const override { return false; }
};

// Overloads for DecorationMaterial comparisons (help MSVC with overload resolution)
inline bool operator==(const Material &lhs, const DecorationMaterial &rhs) { return &lhs == static_cast<const Material*>(&rhs); }
inline bool operator==(const DecorationMaterial &lhs, const Material &rhs) { return static_cast<const Material*>(&lhs) == &rhs; }
inline bool operator!=(const Material &lhs, const DecorationMaterial &rhs) { return &lhs != static_cast<const Material*>(&rhs); }
inline bool operator!=(const DecorationMaterial &lhs, const Material &rhs) { return static_cast<const Material*>(&lhs) != &rhs; }