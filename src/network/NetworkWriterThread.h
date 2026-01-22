#pragma once

#include "network/NetworkManager.h"
#include "java/String.h"
#include <thread>

// NetworkWriterThread - matches Java NetworkWriterThread.java exactly
class NetworkWriterThread {
private:
	NetworkManager* netManager;
	jstring threadName;
	
public:
	NetworkWriterThread(NetworkManager* nm, const jstring& name);
	void run();  // Java Thread.run()
};
