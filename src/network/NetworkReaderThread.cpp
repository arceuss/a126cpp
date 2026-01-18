#include "network/NetworkReaderThread.h"
#include "network/NetworkManager.h"
#include <chrono>
#include <thread>

NetworkReaderThread::NetworkReaderThread(NetworkManager* nm, const jstring& name)
	: netManager(nm)
	, threadName(name)
{
}

void NetworkReaderThread::run()
{
	// Java: Object var1 = NetworkManager.threadSyncObject;
	//       synchronized(var1) {
	//           ++NetworkManager.numReadThreads;
	//       }
	{
		std::lock_guard<std::mutex> lock(NetworkManager::threadSyncObject);
		++NetworkManager::numReadThreads;
	}
	
	// Java: while(true) {
	while (true)
	{
		// Java: boolean var2 = false; boolean var21 = false;
		bool var2 = false;
		bool var21 = false;
		
		// Java: Object var3;
		//       label188: {
		//       label187: {
		// Java structure:
		//   label188: {
		//       label187: {
		//           try { ... } finally { ... }
		//           if(var2) { --numReadThreads; }  // After finally, still in label187
		//           continue;  // Normal path: continue while loop
		//       }
		//       // After label187 block (break label187 exits here)
		//       if(var2) { --numReadThreads; }
		//       break;  // Break from label188
		//   }
		//   // After label188 block (break label188 exits here)
		//   if(var2) { --numReadThreads; }
		//   break;  // Break from while loop
		
		// Java: label188: {
		int breakTarget = 0;  // 0 = none, 1 = label187, 2 = label188
		
		// Java: label187: {
		try
		{
			var21 = true;
			var2 = true;
			
			// Java: if(!NetworkManager.isRunning(this.netManager)) {
			if (!NetworkManager::isRunning(netManager))
			{
				var2 = false;
				var21 = false;
				breakTarget = 1;  // Java: break label187
			}
			// Java: if(NetworkManager.isServerTerminating(this.netManager)) {
			else if (NetworkManager::isServerTerminating(netManager))
			{
				var2 = false;
				var21 = false;
				breakTarget = 2;  // Java: break label188
			}
			else
			{
				// Java: while(true) {
				while (true)
				{
					// Java: if(!NetworkManager.readNetworkPacket(this.netManager)) {
					if (!NetworkManager::readNetworkPacket(netManager))
					{
						// Java: try { sleep(100L); var21 = false; } catch (InterruptedException var27) { var21 = false; }
						// In Java, thread.interrupt() wakes from sleep() immediately via InterruptedException
						// We use condition variable with timeout to achieve the same effect
						var21 = false;
						// Wait up to 100ms, but wake immediately if notify() is called (matches Java interrupt behavior)
						{
							std::unique_lock<std::mutex> lock(netManager->readPacketsMutex);
							netManager->readPacketsCondition.wait_for(lock, std::chrono::milliseconds(100));
						}
						break;  // Break inner while loop
					}
				}
			}
		}
		catch (const std::exception& e)
		{
			// Exception caught - var21 and var2 remain as set
		}
		
		// Java: finally {
		//           if(var21) {
		//               if(var2) {
		//                   Object var9 = NetworkManager.threadSyncObject;
		//                   synchronized(var9) {
		//                       --NetworkManager.numReadThreads;
		//                   }
		//               }
		//           }
		//       }
		// Note: Finally executes before any break/continue from try block
		if (var21 && var2)
		{
			{
				std::lock_guard<std::mutex> lock(NetworkManager::threadSyncObject);
				--NetworkManager::numReadThreads;
			}
		}
		
		if (breakTarget == 1)
		{
			// Java: break label187 - exits label187 block
			// Fall through to code after label187 (inside label188)
		}
		else if (breakTarget == 2)
		{
			// Java: break label188 - exits label188 block, skip code after label187
			goto afterLabel188;
		}
		else
		{
			// Java: Normal path - after finally, still inside label187 block
			//       if(var2) {
			//           var3 = NetworkManager.threadSyncObject;
			//           synchronized(var3) {
			//               --NetworkManager.numReadThreads;
			//           }
			//       }
			//       continue;
			if (var2)
			{
				{
					std::lock_guard<std::mutex> lock(NetworkManager::threadSyncObject);
					--NetworkManager::numReadThreads;
				}
			}
			continue;  // Java: continue while loop (normal path)
		}
		
		// Java: After label187 block (inside label188) - break label187 exits here
		//       if(var2) {
		//           var3 = NetworkManager.threadSyncObject;
		//           synchronized(var3) {
		//               --NetworkManager.numReadThreads;
		//           }
		//       }
		//       break;  // Break from label188
		if (var2)
		{
			{
				std::lock_guard<std::mutex> lock(NetworkManager::threadSyncObject);
				--NetworkManager::numReadThreads;
			}
		}
		// Break from label188 - exits label188 block
		
		// Java: After label188 block (inside while loop) - break label188 or break from label187 exits here
		afterLabel188:
		if (var2)
		{
			{
				std::lock_guard<std::mutex> lock(NetworkManager::threadSyncObject);
				--NetworkManager::numReadThreads;
			}
		}
		// Java: break; (break from while loop)
		break;
	}
	
	// Java: After outer while loop - if(var2) { decrement }
	// Note: var2 is local to while loop, so it's out of scope here in C++
	// In Java, var2 would still be accessible here, but would be false in all exit paths
	// This check is unreachable in practice since var2 is false when we exit, but Java has it
	
	// Java: var1 = NetworkManager.threadSyncObject;
	//       synchronized(var1) {
	//           --NetworkManager.numReadThreads;
	//       }
	// Final decrement (always executed)
	{
		std::lock_guard<std::mutex> lock(NetworkManager::threadSyncObject);
		--NetworkManager::numReadThreads;
	}
}
