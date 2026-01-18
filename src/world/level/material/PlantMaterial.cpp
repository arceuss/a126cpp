#include "world/level/material/PlantMaterial.h"
#include "world/level/material/Material.h"

// Static instance for Material::plants - accessed via Material::getPlantsMaterial()
static PlantMaterial g_plantMaterial;

// Material::getPlantsMaterial() implementation
const Material &Material::getPlantsMaterial()
{
	return g_plantMaterial;
}
