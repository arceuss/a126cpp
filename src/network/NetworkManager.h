#pragma once

#include "network/SocketStreams.h"
#include "network/Packet.h"
#include "java/String.h"
#include <memory>
#include <vector>
#include <list>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>

class NetHandler;
class Packet;

// NetworkManager - matches Java NetworkManager.java exactly
class NetworkManager {
public:
	static std::mutex threadSyncObject;
	static int numReadThreads;
	static int numWriteThreads;
	static int packetReceiveStats[256];
	static int packetSendStats[256];
	
	NetworkManager(SocketHandle socket, const jstring& name, NetHandler* handler);
	~NetworkManager();
	
	void addToSendQueue(Packet* packet);
	void wakeThreads();
	void processReadPackets();
	void wakeWriterThread();  // func_28142_c in Java
	void networkShutdown(const jstring& reason, const std::vector<jstring>& args);
	
	// Force send Packet 255 (disconnect) before closing - ensures it's sent synchronously
	void forceSendDisconnectPacket();
	
	// Send Packet255 directly via TCP (bypasses queue) - guarantees it's sent
	void sendDisconnectPacketDirect(const jstring& reason);
	
	// EXPERIMENTAL: Send invalid packet ID to force disconnect (for testing)
	void sendInvalidPacket(int packetId = 0x38);
	
	// Static accessors matching Java implementation
	static bool isRunning(NetworkManager* nm);
	static bool isServerTerminating(NetworkManager* nm);
	static bool readNetworkPacket(NetworkManager* nm);
	static bool sendNetworkPacket(NetworkManager* nm);
	static SocketOutputStream* func_28140_f(NetworkManager* nm);  // getSocketOutputStream
	static bool func_28138_e(NetworkManager* nm);  // isTerminating
	static void func_30005_a(NetworkManager* nm, const std::exception& e);  // onNetworkError
	static std::thread* getReadThread(NetworkManager* nm);
	static std::thread* getWriteThread(NetworkManager* nm);
	
private:
	std::mutex sendQueueLock;
	SocketHandle networkSocket;
	
	std::unique_ptr<SocketInputStream> socketInputStream;
	std::unique_ptr<SocketOutputStream> socketOutputStream;
	
	std::atomic<bool> isRunningFlag;  // Renamed to avoid conflict with static function
	
	// Synchronized lists matching Java Collections.synchronizedList
	std::mutex readPacketsMutex;
	std::list<std::unique_ptr<Packet>> readPackets;
	
	// Condition variable to wake threads (replaces Java's thread.interrupt())
	std::condition_variable readPacketsCondition;
	
	std::mutex dataPacketsMutex;  // Not used, but kept for structure
	std::list<std::unique_ptr<Packet>> dataPackets;
	std::mutex chunkDataPacketsMutex;  // Not used, but kept for structure
	std::list<std::unique_ptr<Packet>> chunkDataPackets;
	
	NetHandler* netHandler;
	std::atomic<bool> isServerTerminatingFlag;  // Renamed to avoid conflict with static function
	std::unique_ptr<std::thread> writeThread;
	std::unique_ptr<std::thread> readThread;
	std::atomic<bool> isTerminating;
	jstring terminationReason;
	std::vector<jstring> terminationArgs;  // Object[] field_20101_t in Java
	
	int timeSinceLastRead;
	int sendQueueByteLength;
	
	int chunkDataSendCounter;
	int field_20100_w;  // Chunk data send delay (starts at 50)
	
	// Private methods
	bool sendPacket();
	bool readPacket();
	void onNetworkError(const std::exception& e);
	
	friend class NetworkReaderThread;
	friend class NetworkWriterThread;
	friend class NetworkMasterThread;
	friend class ThreadCloseConnection;
};
