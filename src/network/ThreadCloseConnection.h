#pragma once

#include "network/NetworkManager.h"

// ThreadCloseConnection - matches Java ThreadCloseConnection.java exactly
class ThreadCloseConnection {
private:
	NetworkManager* field_28109_a;  // netManager (matching Java field name)
	
public:
	ThreadCloseConnection(NetworkManager* nm);
	void run();  // Java Thread.run()
};
