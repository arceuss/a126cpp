#include "network/NetworkWriterThread.h"
#include "network/NetworkManager.h"
#include <chrono>
#include <thread>
#include <stdexcept>
#include <iostream>

NetworkWriterThread::NetworkWriterThread(NetworkManager* nm, const jstring& name)
	: netManager(nm)
	, threadName(name)
{
}

void NetworkWriterThread::run()
{
	// Java: Object var1 = NetworkManager.threadSyncObject;
	//       synchronized(var1) {
	//           ++NetworkManager.numWriteThreads;
	//       }
	{
		std::lock_guard<std::mutex> lock(NetworkManager::threadSyncObject);
		++NetworkManager::numWriteThreads;
	}
	
	// Java: boolean var2;
	//       Object var3;
	//       while(true) {
	bool var2 = false;
	
	while (true)
	{
		// Java: var2 = false;
		var2 = false;
		// Java: boolean var20 = false;
		bool var20 = false;
		
		// Java: try {
		try
		{
			var20 = true;
			var2 = true;
			
			// Java: if(!NetworkManager.isRunning(this.netManager)) {
			if (!NetworkManager::isRunning(netManager))
			{
				var2 = false;
				var20 = false;
				break;  // Java: break out of while(true)
			}
			
			// Java: while(true) {
			while (true)
			{
				// Java: if(!NetworkManager.sendNetworkPacket(this.netManager)) {
				if (!NetworkManager::sendNetworkPacket(netManager))
				{
					// Java: try { sleep(100L); } catch (InterruptedException var25) { }
					try
					{
						std::this_thread::sleep_for(std::chrono::milliseconds(100));
					}
					catch (...)
					{
						// Ignore interruption
					}
					
					// Java: try {
					//           if(NetworkManager.func_28140_f(this.netManager) != null) {
					//               NetworkManager.func_28140_f(this.netManager).flush();
					//               var20 = false;
					//           } else {
					//               var20 = false;
					//           }
					//       } catch (IOException var27) {
					//           if(!NetworkManager.func_28138_e(this.netManager)) {
					//               NetworkManager.func_30005_a(this.netManager, var27);
					//           }
					//           var27.printStackTrace();
					//           var20 = false;
					//       }
					try
					{
						SocketOutputStream* out = NetworkManager::func_28140_f(netManager);
						if (out != nullptr)
						{
							out->flush();
						}
						var20 = false;
					}
					catch (const std::exception& e)
					{
						// Java: if(!NetworkManager.func_28138_e(this.netManager)) {
						if (!NetworkManager::func_28138_e(netManager))
						{
							// Java: NetworkManager.func_30005_a(this.netManager, var27);
							NetworkManager::func_30005_a(netManager, e);
						}
						
						// Java: var27.printStackTrace();
						std::cerr << "NetworkWriterThread exception: " << e.what() << std::endl;
						var20 = false;
					}
					
					break;  // Java: break inner while loop
				}
			}
		}
		catch (const std::exception& e)
		{
			// Exception handling - var20 and var2 remain as set
		}
		
		// Java: finally {
		//           if(var20) {
		//               if(var2) {
		//                   Object var8 = NetworkManager.threadSyncObject;
		//                   synchronized(var8) {
		//                       --NetworkManager.numWriteThreads;
		//                   }
		//               }
		//           }
		//       }
		if (var20 && var2)
		{
			{
				std::lock_guard<std::mutex> lock(NetworkManager::threadSyncObject);
				--NetworkManager::numWriteThreads;
			}
		}
		
		// Java: if(var2) {
		//           var3 = NetworkManager.threadSyncObject;
		//           synchronized(var3) {
		//               --NetworkManager.numWriteThreads;
		//           }
		//       }
		if (var2)
		{
			{
				std::lock_guard<std::mutex> lock(NetworkManager::threadSyncObject);
				--NetworkManager::numWriteThreads;
			}
		}
		
		// Check if we should exit outer loop (set by !isRunning check)
		if (!NetworkManager::isRunning(netManager))
		{
			break;
		}
	}
	
	// Java: if(var2) {
	//           var3 = NetworkManager.threadSyncObject;
	//           synchronized(var3) {
	//               --NetworkManager.numWriteThreads;
	//           }
	//       }
	// Note: var2 will be false at this point if we broke from !isRunning, so this won't execute
	// But Java has this check, so we need it for accuracy
	if (var2)
	{
		{
			std::lock_guard<std::mutex> lock(NetworkManager::threadSyncObject);
			--NetworkManager::numWriteThreads;
		}
	}
	
	// Java: var1 = NetworkManager.threadSyncObject;
	//       synchronized(var1) {
	//           --NetworkManager.numWriteThreads;
	//       }
	// Final decrement (always executed)
	{
		std::lock_guard<std::mutex> lock(NetworkManager::threadSyncObject);
		--NetworkManager::numWriteThreads;
	}
}
