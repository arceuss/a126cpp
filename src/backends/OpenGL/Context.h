#pragma once

namespace renderbackend
{

struct Configuration;

}

namespace openglbackend
{

void initialize(const renderbackend::Configuration &configuration);
void present();
void shutdown();
bool hasCapability(const char *capability);

}
