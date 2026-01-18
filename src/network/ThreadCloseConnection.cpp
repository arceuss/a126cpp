#include "network/ThreadCloseConnection.h"
#include "network/NetworkManager.h"
#include <chrono>
#include <thread>
#include <iostream>

ThreadCloseConnection::ThreadCloseConnection(NetworkManager* nm)
	: field_28109_a(nm)
{
}

void ThreadCloseConnection::run()
{
	// Java: try {
	//           Thread.sleep(2000L);
	try
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(2000));
		
		// Java: if(NetworkManager.isRunning(this.field_28109_a)) {
		if (NetworkManager::isRunning(field_28109_a))
		{
			// Java: NetworkManager.getWriteThread(this.field_28109_a).interrupt();
			// Note: Thread interruption handled via flags in C++
			
			// Java: this.field_28109_a.networkShutdown("disconnect.closed", new Object[0]);
			field_28109_a->networkShutdown(u"disconnect.closed", {});
		}
	}
	catch (const std::exception& e)
	{
		// Java: } catch (Exception var2) {
		//           var2.printStackTrace();
		//       }
		std::cerr << "ThreadCloseConnection exception: " << e.what() << std::endl;
	}
}
