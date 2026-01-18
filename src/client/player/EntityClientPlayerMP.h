#pragma once

#include "client/player/LocalPlayer.h"

class NetClientHandler;

// Alpha 1.2.6: EntityClientPlayerMP - multiplayer client player implementation
// Reference: alpha1.2.6/minecraft/src/net/minecraft/src/EntityClientPlayerMP.java
// Extends EntityPlayerSP (our LocalPlayer) to add multiplayer networking functionality
class EntityClientPlayerMP : public LocalPlayer
{
public:
	NetClientHandler* field_797_bg;  // NetClientHandler for sending packets

private:
	// Position tracking fields (used in func_4056_N for delta calculation)
	double field_9379_by;  // Last X position
	double field_9378_bz;  // Last boundingBox.minY
	double field_9377_bA;  // Last Y position
	double field_9376_bB;  // Last Z position
	float field_9385_bC;   // Last rotationYaw
	float field_9384_bD;   // Last rotationPitch
	bool field_9382_bF;    // Last onGround state
	bool field_9381_bG;    // Last sneaking state
	
	int field_9380_bx;     // Counter for sending inventory updates (every 20 ticks)
	bool field_21093_bH;   // Flag for setHealth() behavior
	int field_12242_bI;    // noSendTime counter

public:
	EntityClientPlayerMP(Minecraft& minecraft, Level& level, User* user, NetClientHandler* netHandler, int_t dimension);
	
	// Override EntityPlayerSP (LocalPlayer) methods
	void tick() override;  // Override to call func_4056_N() (Java: onUpdate())
	
	// Alpha 1.2.6: Override Entity::updatePositionTracking() to update multiplayer position tracking
	void updatePositionTracking() override;
	
	// Alpha 1.2.6 methods
	bool canAttackEntity(Entity& var1, int_t var2);
	void heal(int_t var1);  // Not an override - Mob::heal() is not virtual
	
	void func_6420_o();  // Empty method
	
	void func_4056_N();  // Sends position updates - called from tick() (Java: onUpdate calls this)
	
	void sendInventoryChanged();  // Sends inventory changes (empty in Alpha 1.2.6)
	
	void joinEntityItemWithWorld(EntityItem& var1);  // Empty override
	
	void sendChatMessage(const jstring& var1);
	
	// Alpha 1.2.6: Override swing() to call func_457_w() which sends Packet18ArmAnimation
	// Java: func_457_w() is called directly, but in C++ we override swing() to match the call pattern
	void swing() override;  // Override to call func_457_w() which sends Packet18ArmAnimation
	
	void func_457_w();  // Swing arm - sends Packet18ArmAnimation (Java: not an override, but we call it from swing())
	void func_9367_r();  // Respawn - sends Packet9 (not an override in Java, but may override respawn())
	void setHealth(int_t var1);  // Override behavior (Java has special first-call handling)
	
	void dropCurrentItem();  // Sends Packet14BlockDig with state 4 - public so Minecraft can call it
	
	// Beta 1.2: MultiplayerLocalPlayer overrides (matches newb12 MultiplayerLocalPlayer.java)
	void hurtTo(int_t newHealth);  // Beta: hurtTo() override (MultiplayerLocalPlayer.java:165-172) - overrides LocalPlayer::hurtTo()
	
protected:
	void reallyDrop(std::shared_ptr<EntityItem> itemEntity);  // Beta: reallyDrop() override (MultiplayerLocalPlayer.java:132-133) - overrides Player::reallyDrop()
	
	void chat(const jstring& message) override;  // Beta: chat() override (MultiplayerLocalPlayer.java:136-138)
	
private:
	void damageEntity(int_t var1);  // Protected override in Java - just updates health
	
	void closeContainer() override;  // Sends Packet101CloseWindow
};
