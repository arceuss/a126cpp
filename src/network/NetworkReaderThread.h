#pragma once

#include "network/NetworkManager.h"
#include "java/String.h"
#include <thread>

// NetworkReaderThread - matches Java NetworkReaderThread.java exactly
class NetworkReaderThread {
private:
	NetworkManager* netManager;
	jstring threadName;
	
public:
	NetworkReaderThread(NetworkManager* nm, const jstring& name);
	void run();  // Java Thread.run()
};
