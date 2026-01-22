#pragma once

#include "client/model/QuadrupedModel.h"

// newb12: PigModel - model for pig entities
// Reference: newb12/net/minecraft/client/model/PigModel.java
class PigModel : public QuadrupedModel
{
public:
	PigModel();
	PigModel(float grow);
};
