#include "network/NetworkManager.h"
#include "network/Packet255KickDisconnect.h"
#include "network/NetworkReaderThread.h"
#include "network/NetworkWriterThread.h"
#include "network/NetworkMasterThread.h"
#include "network/ThreadCloseConnection.h"
#include "network/Packet.h"
#include "java/System.h"
#include "java/String.h"
#include <iostream>
#include <stdexcept>
#include <chrono>
#include <thread>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <unistd.h>
#endif

// Static member definitions
std::mutex NetworkManager::threadSyncObject;
int NetworkManager::numReadThreads = 0;
int NetworkManager::numWriteThreads = 0;
int NetworkManager::packetReceiveStats[256] = {};
int NetworkManager::packetSendStats[256] = {};

NetworkManager::NetworkManager(SocketHandle socket, const jstring& name, NetHandler* handler)
	: networkSocket(socket)
	, netHandler(handler)
	, isRunningFlag(true)
	, isServerTerminatingFlag(false)
	, isTerminating(false)
	, timeSinceLastRead(0)
	, sendQueueByteLength(0)
	, chunkDataSendCounter(0)
	, field_20100_w(50)
{
	// Java: try { var1.setSoTimeout(30000); var1.setTrafficClass(24); } catch (SocketException var5) { ... }
	try
	{
#ifdef _WIN32
		// Set socket timeout to 30 seconds (30000ms)
		DWORD timeout = 30000;
		setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
		setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
		
		// Set traffic class to 24 (IPTOS_LOWDELAY | IPTOS_THROUGHPUT)
		int tos = 24;
		setsockopt(socket, IPPROTO_IP, IP_TOS, (const char*)&tos, sizeof(tos));
#else
		// Set socket timeout to 30 seconds
		struct timeval timeout;
		timeout.tv_sec = 30;
		timeout.tv_usec = 0;
		setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
		setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
		
		// Set traffic class to 24
		int tos = 24;
		setsockopt(socket, IPPROTO_IP, IP_TOS, &tos, sizeof(tos));
#endif
	}
	catch (...)
	{
		// Ignore socket exceptions like Java does
	}
	
	// Java: this.socketInputStream = new DataInputStream(var1.getInputStream());
	socketInputStream = std::make_unique<SocketInputStream>(socket);
	
	// Java: this.socketOutputStream = new DataOutputStream(new BufferedOutputStream(var1.getOutputStream(), 5120));
	// Note: SocketOutputStream already has buffering with 5120 byte buffer
	socketOutputStream = std::make_unique<SocketOutputStream>(socket);
	
	// Java: this.readThread = new NetworkReaderThread(this, var2 + " read thread");
	jstring readThreadName = name + u" read thread";
	jstring writeThreadName = name + u" write thread";
	
	readThread = std::make_unique<std::thread>([this, readThreadName]() {
		NetworkReaderThread thread(this, readThreadName);
		thread.run();
	});
	
	// Java: this.writeThread = new NetworkWriterThread(this, var2 + " write thread");
	writeThread = std::make_unique<std::thread>([this, writeThreadName]() {
		NetworkWriterThread thread(this, writeThreadName);
		thread.run();
	});
}

NetworkManager::~NetworkManager()
{
	networkShutdown(u"Closed", {});
}

void NetworkManager::addToSendQueue(Packet* packet)
{
	// Java: if(!this.isServerTerminating) {
	if (!isServerTerminatingFlag.load())
	{
		// Java: Object var2 = this.sendQueueLock;
		//       synchronized(var2) {
		std::lock_guard<std::mutex> lock(sendQueueLock);
		
		// Java: this.sendQueueByteLength += var1.getPacketSize() + 1;
		sendQueueByteLength += packet->getPacketSize() + 1;
		
		// Java: if(var1.isChunkDataPacket) {
		if (packet->isChunkDataPacket)
		{
			// Java: this.chunkDataPackets.add(var1);
			chunkDataPackets.push_back(std::unique_ptr<Packet>(packet));
		}
		else
		{
			// Java: this.dataPackets.add(var1);
			// Give Packet 255 (disconnect) priority by adding it to the front of the queue
			// This ensures it's sent before the connection closes
			if (packet->getPacketId() == 255)
			{
				dataPackets.push_front(std::unique_ptr<Packet>(packet));
			}
			else
			{
				dataPackets.push_back(std::unique_ptr<Packet>(packet));
			}
		}
	}
}

bool NetworkManager::sendPacket()
{
	// Java: boolean var1 = false;
	bool result = false;
	
	try
	{
		// Java: int[] var2; int var3; Packet var4; Object var5;
		std::unique_ptr<Packet> packet;
		
		// Java: if(!this.dataPackets.isEmpty() && (this.chunkDataSendCounter == 0 || 
		//      System.currentTimeMillis() - ((Packet)this.dataPackets.get(0)).creationTimeMillis >= (long)this.chunkDataSendCounter)) {
		bool shouldSendDataPacket = false;
		{
			std::lock_guard<std::mutex> lock(sendQueueLock);
			if (!dataPackets.empty())
			{
				if (chunkDataSendCounter == 0 ||
				    System::currentTimeMillis() - dataPackets.front()->creationTimeMillis >= static_cast<long_t>(chunkDataSendCounter))
				{
					shouldSendDataPacket = true;
				}
			}
		}
		
		if (shouldSendDataPacket)
		{
			// Java: var5 = this.sendQueueLock;
			//       synchronized(var5) {
			//           var4 = (Packet)this.dataPackets.remove(0);
			//           this.sendQueueByteLength -= var4.getPacketSize() + 1;
			//       }
			{
				std::lock_guard<std::mutex> lock(sendQueueLock);
				if (!dataPackets.empty())
				{
					packet = std::move(dataPackets.front());
					dataPackets.pop_front();
					sendQueueByteLength -= packet->getPacketSize() + 1;
				}
			}
			
			if (packet)
			{
				// Java: Packet.writePacket(var4, this.socketOutputStream);
				Packet::writePacket(packet.get(), *socketOutputStream);
				
				// Java: var2 = field_28144_e;
				//       var3 = var4.getPacketId();
				//       var2[var3] += var4.getPacketSize() + 1;
				int packetId = packet->getPacketId();
				packetSendStats[packetId] += packet->getPacketSize() + 1;
				
				// Java: var1 = true;
				result = true;
			}
		}
		
		// Java: if(this.field_20100_w-- <= 0 && !this.chunkDataPackets.isEmpty() && 
		//      (this.chunkDataSendCounter == 0 || System.currentTimeMillis() - ((Packet)this.chunkDataPackets.get(0)).creationTimeMillis >= (long)this.chunkDataSendCounter)) {
		bool shouldSendChunkPacket = false;
		{
			std::lock_guard<std::mutex> lock(sendQueueLock);
			if (field_20100_w-- <= 0 && !chunkDataPackets.empty())
			{
				if (chunkDataSendCounter == 0 ||
				    System::currentTimeMillis() - chunkDataPackets.front()->creationTimeMillis >= static_cast<long_t>(chunkDataSendCounter))
				{
					shouldSendChunkPacket = true;
				}
			}
		}
		
		if (shouldSendChunkPacket)
		{
			std::unique_ptr<Packet> chunkPacket;
			// Java: var5 = this.sendQueueLock;
			//       synchronized(var5) {
			//           var4 = (Packet)this.chunkDataPackets.remove(0);
			//           this.sendQueueByteLength -= var4.getPacketSize() + 1;
			//       }
			{
				std::lock_guard<std::mutex> lock(sendQueueLock);
				if (!chunkDataPackets.empty())
				{
					chunkPacket = std::move(chunkDataPackets.front());
					chunkDataPackets.pop_front();
					sendQueueByteLength -= chunkPacket->getPacketSize() + 1;
				}
			}
			
			if (chunkPacket)
			{
				// Java: Packet.writePacket(var4, this.socketOutputStream);
				Packet::writePacket(chunkPacket.get(), *socketOutputStream);
				
				// Java: var2 = field_28144_e;
				//       var3 = var4.getPacketId();
				//       var2[var3] += var4.getPacketSize() + 1;
				int packetId = chunkPacket->getPacketId();
				packetSendStats[packetId] += chunkPacket->getPacketSize() + 1;
				
				// Java: this.field_20100_w = 0;
				field_20100_w = 0;
				
				// Java: var1 = true;
				result = true;
			}
		}
		
		return result;
	}
	catch (const std::exception& e)
	{
		// Java: if(!this.isTerminating) {
		if (!isTerminating.load())
		{
			// Java: this.onNetworkError(var11);
			onNetworkError(e);
		}
		
		return false;
	}
}

void NetworkManager::wakeThreads()
{
	// Java: this.readThread.interrupt();
	//       this.writeThread.interrupt();
	// In Java, interrupt() wakes threads from sleep() immediately
	// We use condition variable to achieve the same effect
	readPacketsCondition.notify_all();
}

bool NetworkManager::readPacket()
{
	// Java: boolean var1 = false;
	bool result = false;
	
	try
	{
		// Java: Packet var2 = Packet.readPacket(this.socketInputStream, this.netHandler.isServerHandler());
		std::unique_ptr<Packet> packet = Packet::readPacket(*socketInputStream, netHandler->isServerHandler());
		
		if (packet)
		{
			// Java: int[] var3 = field_28145_d;
			//       int var4 = var2.getPacketId();
			//       var3[var4] += var2.getPacketSize() + 1;
			int packetId = packet->getPacketId();
			packetReceiveStats[packetId] += packet->getPacketSize() + 1;
			
			// Java: this.readPackets.add(var2);
			{
				std::lock_guard<std::mutex> lock(readPacketsMutex);
				readPackets.push_back(std::move(packet));
				readPacketsCondition.notify_one();  // Wake processing thread
			}
			
			// Java: var1 = true;
			result = true;
		}
		else
		{
			// Java: this.networkShutdown("disconnect.endOfStream", new Object[0]);
			networkShutdown(u"disconnect.endOfStream", {});
		}
		
		return result;
	}
	catch (const std::exception& e)
	{
		// Java: SocketTimeoutException is thrown when socket times out, but connection stays alive
		// We should handle timeouts gracefully (return false, don't disconnect)
		std::string errorMsg = e.what();
		if (errorMsg == "Socket timeout")
		{
			// Socket timeout - this is normal, not an error
			// Return false to indicate no packet was read, but don't disconnect
			return false;
		}
		
		// Java: if(!this.isTerminating) {
		if (!isTerminating.load())
		{
			// Java: this.onNetworkError(var5);
			onNetworkError(e);
		}
		
		return false;
	}
}

void NetworkManager::onNetworkError(const std::exception& e)
{
	// Java: var1.printStackTrace();
	std::cerr << "Network error: " << e.what() << std::endl;
	
	// Java: this.networkShutdown("disconnect.genericReason", new Object[]{"Internal exception: " + var1.toString()});
	std::vector<jstring> args;
	args.push_back(String::fromUTF8("Internal exception: " + std::string(e.what())));
	networkShutdown(u"disconnect.genericReason", args);
}

void NetworkManager::forceSendDisconnectPacket()
{
	// Force send Packet 255 from the queue before closing
	// This ensures the disconnect packet is sent synchronously
	// Search the entire queue for Packet 255 (it should be at the front due to priority, but check all)
	std::lock_guard<std::mutex> lock(sendQueueLock);
	
	if (socketOutputStream == nullptr)
		return;
	
	// Find and send Packet 255 from the queue
	for (auto it = dataPackets.begin(); it != dataPackets.end(); ++it)
	{
		if ((*it)->getPacketId() == 255)
		{
			try
			{
				std::unique_ptr<Packet> packet = std::move(*it);
				dataPackets.erase(it);
				sendQueueByteLength -= packet->getPacketSize() + 1;
				
				// Send the packet directly
				Packet::writePacket(packet.get(), *socketOutputStream);
				socketOutputStream->flush();
				break;  // Only send one disconnect packet
			}
			catch (...)
			{
				// Ignore errors - we're disconnecting anyway
			}
		}
	}
}

void NetworkManager::sendDisconnectPacketDirect(const jstring& reason)
{
	// Send Packet255 directly via TCP, bypassing the queue
	// This ensures the packet is sent synchronously before connection closes
	if (socketOutputStream == nullptr)
		return;
	
	try
	{
		// Create Packet255 and write it directly to the socket
		Packet255KickDisconnect packet(reason);
		
		// Write packet ID
		socketOutputStream->writeByte(255);
		
		// Write packet data directly
		packet.writePacketData(*socketOutputStream);
		
		// Flush immediately to ensure it's sent
		socketOutputStream->flush();
	}
	catch (...)
	{
		// Ignore errors - we're disconnecting anyway
	}
}

void NetworkManager::sendInvalidPacket(int packetId)
{
	// EXPERIMENTAL: Send an invalid packet ID to force server to disconnect us
	// This is just the packet ID byte with no data - server will see it as bad packet
	if (socketOutputStream == nullptr)
		return;
	
	try
	{
		// Just write the packet ID byte - no packet data
		// Server will try to read packet data and fail, causing IOException
		socketOutputStream->writeByte(static_cast<byte_t>(packetId & 0xFF));
		socketOutputStream->flush();
	}
	catch (...)
	{
		// Ignore errors - we're disconnecting anyway
	}
}

void NetworkManager::networkShutdown(const jstring& reason, const std::vector<jstring>& args)
{
	// Java: if(this.isRunning) {
	if (isRunningFlag.load())
	{
		// Force send any disconnect packet (Packet 255) before closing
		// This ensures the server receives the disconnect notification
		forceSendDisconnectPacket();
		
		// Java: this.isTerminating = true;
		isTerminating = true;
		
		// Java: this.terminationReason = var1;
		terminationReason = reason;
		
		// Java: this.field_20101_t = var2;
		terminationArgs = args;
		
		// Java: (new NetworkMasterThread(this)).start();
		NetworkMasterThread* masterThread = new NetworkMasterThread(this);
		std::thread([masterThread]() {
			masterThread->run();
			delete masterThread;
		}).detach();
		
		// Java: this.isRunning = false;
		isRunningFlag = false;
		
		// Java: try { this.socketInputStream.close(); this.socketInputStream = null; } catch (Throwable var6) {}
		try
		{
			socketInputStream.reset();
		}
		catch (...)
		{
		}
		
		// Java: try { this.socketOutputStream.close(); this.socketOutputStream = null; } catch (Throwable var5) {}
		try
		{
			socketOutputStream.reset();
		}
		catch (...)
		{
		}
		
		// Java: try { this.networkSocket.close(); this.networkSocket = null; } catch (Throwable var4) {}
		try
		{
#ifdef _WIN32
			closesocket(networkSocket);
#else
			close(networkSocket);
#endif
			networkSocket = INVALID_SOCKET_HANDLE;
		}
		catch (...)
		{
		}
	}
}

void NetworkManager::processReadPackets()
{
	// Java: if(this.sendQueueByteLength > 1048576) {
	if (sendQueueByteLength > 1048576)
	{
		// Java: this.networkShutdown("disconnect.overflow", new Object[0]);
		networkShutdown(u"disconnect.overflow", {});
	}
	
	// Java: if(this.readPackets.isEmpty()) {
	bool isEmpty;
	{
		std::lock_guard<std::mutex> lock(readPacketsMutex);
		isEmpty = readPackets.empty();
	}
	
	if (isEmpty)
	{
		// Java: if(this.timeSinceLastRead++ == 1200) {
		if (++timeSinceLastRead == 1200)
		{
			// Java: this.networkShutdown("disconnect.timeout", new Object[0]);
			networkShutdown(u"disconnect.timeout", {});
		}
	}
	else
	{
		// Java: } else { this.timeSinceLastRead = 0; }
		timeSinceLastRead = 0;
	}
	
	// Java: int var1 = 100;
	// Removed limit - process all packets in queue to prevent delays
	// Original Alpha 1.2.6 had 100 limit, but modern hardware can handle unlimited processing
	// This ensures responsive packet handling for chests, chat, etc.
	while (true)
	{
		// Check if queue is empty (removed var1 limit check)
		bool isEmpty;
		{
			std::lock_guard<std::mutex> lock(readPacketsMutex);
			isEmpty = readPackets.empty();
		}
		
		if (isEmpty)
			break;
		
		// Java: Packet var2 = (Packet)this.readPackets.remove(0);
		std::unique_ptr<Packet> packet;
		{
			std::lock_guard<std::mutex> lock(readPacketsMutex);
			// Double-check empty inside lock (packets could have been consumed by another thread in theory)
			if (readPackets.empty())
				break;
			
			packet = std::move(readPackets.front());
			readPackets.pop_front();
		}
		
		// Java: var2.processPacket(this.netHandler);
		packet->processPacket(netHandler);
	}
	
	// Java: this.wakeThreads();
	wakeThreads();
	
	// Java: if(this.isTerminating && this.readPackets.isEmpty()) {
	bool shouldCallErrorHandler = false;
	{
		std::lock_guard<std::mutex> lock(readPacketsMutex);
		if (isTerminating.load() && readPackets.empty())
		{
			shouldCallErrorHandler = true;
		}
	}
	
	if (shouldCallErrorHandler)
	{
		// Java: this.netHandler.handleErrorMessage(this.terminationReason, this.field_20101_t);
		netHandler->handleErrorMessage(terminationReason, terminationArgs);
	}
}

void NetworkManager::wakeWriterThread()
{
	// Java: this.wakeThreads();
	wakeThreads();
	
	// Java: this.isServerTerminating = true;
	isServerTerminatingFlag = true;
	
	// Java: this.readThread.interrupt();
	// Note: Java interrupts readThread again after wakeThreads() (which already interrupts it)
	// This ensures the read thread wakes up and sees isServerTerminating flag
	// In C++, we notify the condition variable again to match this behavior
	readPacketsCondition.notify_all();
	
	// Java: (new ThreadCloseConnection(this)).start();
	ThreadCloseConnection* closeThread = new ThreadCloseConnection(this);
	std::thread([closeThread]() {
		closeThread->run();
		delete closeThread;
	}).detach();
}

// Static accessors matching Java implementation exactly
bool NetworkManager::isRunning(NetworkManager* nm)
{
	return nm->isRunningFlag.load();
}

bool NetworkManager::isServerTerminating(NetworkManager* nm)
{
	return nm->isServerTerminatingFlag.load();
}

bool NetworkManager::readNetworkPacket(NetworkManager* nm)
{
	return nm->readPacket();
}

bool NetworkManager::sendNetworkPacket(NetworkManager* nm)
{
	return nm->sendPacket();
}

SocketOutputStream* NetworkManager::func_28140_f(NetworkManager* nm)
{
	return nm->socketOutputStream.get();
}

bool NetworkManager::func_28138_e(NetworkManager* nm)
{
	return nm->isTerminating.load();
}

void NetworkManager::func_30005_a(NetworkManager* nm, const std::exception& e)
{
	nm->onNetworkError(e);
}

std::thread* NetworkManager::getReadThread(NetworkManager* nm)
{
	return nm->readThread.get();
}

std::thread* NetworkManager::getWriteThread(NetworkManager* nm)
{
	return nm->writeThread.get();
}
