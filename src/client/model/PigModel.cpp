#include "client/model/PigModel.h"

// newb12: PigModel constructor
// Reference: newb12/net/minecraft/client/model/PigModel.java:4-6
PigModel::PigModel() : QuadrupedModel(6, 0.0f)
{
}

// newb12: PigModel constructor with grow parameter
// Reference: newb12/net/minecraft/client/model/PigModel.java:8-10
PigModel::PigModel(float grow) : QuadrupedModel(6, grow)
{
}
