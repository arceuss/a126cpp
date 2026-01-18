#pragma once

#include "network/NetworkManager.h"

// NetworkMasterThread - matches Java NetworkMasterThread.java exactly
class NetworkMasterThread {
private:
	NetworkManager* netManager;
	
public:
	NetworkMasterThread(NetworkManager* nm);
	void run();  // Java Thread.run()
};
