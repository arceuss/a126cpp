#include "client/player/EntityClientPlayerMP.h"
#include "network/NetClientHandler.h"
#include "network/Packet3Chat.h"
#include "network/Packet9.h"
#include "network/Packet10Flying.h"
#include "network/Packet11PlayerPosition.h"
#include "network/Packet12PlayerLook.h"
#include "network/Packet13PlayerLookMove.h"
#include "network/Packet14BlockDig.h"
#include "network/Packet18ArmAnimation.h"
#include "network/Packet19EntityAction.h"
#include "network/Packet101CloseWindow.h"
#include "world/entity/item/EntityItem.h"
#include "world/level/Level.h"
#include "util/Mth.h"
#include <cmath>

// Alpha 1.2.6: EntityClientPlayerMP constructor
// Java: public EntityClientPlayerMP(Minecraft var1, World var2, Session var3, NetClientHandler var4)
// Java: super(var1, var2, var3, 0);
EntityClientPlayerMP::EntityClientPlayerMP(Minecraft& minecraft, Level& level, User* user, NetClientHandler* netHandler, int_t dimension)
	: LocalPlayer(minecraft, level, user, dimension)
	, field_797_bg(netHandler)
	, field_9380_bx(0)
	, field_21093_bH(false)
	, field_9379_by(0.0)
	, field_9378_bz(0.0)
	, field_9377_bA(0.0)
	, field_9376_bB(0.0)
	, field_9385_bC(0.0f)
	, field_9384_bD(0.0f)
	, field_9382_bF(false)
	, field_9381_bG(false)
	, field_12242_bI(0)
{
}

// Alpha 1.2.6: canAttackEntity - returns false (client can't attack entities directly)
// Java: public boolean canAttackEntity(Entity var1, int var2) { return false; }
bool EntityClientPlayerMP::canAttackEntity(Entity& var1, int_t var2)
{
	return false;
}

// Alpha 1.2.6: heal - empty override (server controls health)
// Java: public void heal(int var1) { }
void EntityClientPlayerMP::heal(int_t var1)
{
}

// Alpha 1.2.6: tick() - calls super.tick() then func_4056_N() if block exists
// Java: public void onUpdate() {
//     if(this.worldObj.blockExists(MathHelper.floor_double(this.posX), 64, MathHelper.floor_double(this.posZ))) {
//         super.onUpdate();
//         this.func_4056_N();
//     }
// }
void EntityClientPlayerMP::tick()
{
	// Beta 1.2: MultiplayerLocalPlayer.tick() - matches newb12 MultiplayerLocalPlayer.java:47-52 exactly
	// Beta: if (this.level.hasChunkAt(Mth.floor(this.x), 64, Mth.floor(this.z))) { ... } (MultiplayerLocalPlayer.java:48)
	if (level.hasChunkAt(Mth::floor(static_cast<float>(x)), 64, Mth::floor(static_cast<float>(z))))
	{
		LocalPlayer::tick();  // Beta: super.tick() (MultiplayerLocalPlayer.java:49)
		func_4056_N();  // Beta: this.sendPosition() (MultiplayerLocalPlayer.java:50)
	}
}

// Alpha 1.2.6: func_6420_o - empty method
// Java: public void func_6420_o() { }
void EntityClientPlayerMP::func_6420_o()
{
}

// Alpha 1.2.6: func_4056_N - sends position updates (CRITICAL METHOD)
// Java: public void func_4056_N() { ... }
void EntityClientPlayerMP::func_4056_N()
{
	// Alpha 1.2.6: Don't send position packets if we're disconnecting
	// This ensures the disconnect packet (Packet255) is sent before any position packets
	if (field_797_bg != nullptr && field_797_bg->isDisconnected())
	{
		return;
	}
	
	// Java: if(this.field_9380_bx++ == 20) {
	//     this.sendInventoryChanged();
	//     this.field_9380_bx = 0;
	// }
	if (field_9380_bx++ == 20)
	{
		sendInventoryChanged();
		field_9380_bx = 0;
	}

	// Java: boolean var1 = this.isSneaking();
	//       if(var1 != this.field_9381_bG) {
	//           if(var1) {
	//               this.field_797_bg.addToSendQueue(new Packet19EntityAction(this, 1));
	//           } else {
	//               this.field_797_bg.addToSendQueue(new Packet19EntityAction(this, 2));
	//           }
	//           this.field_9381_bG = var1;
	//       }
	bool var1 = isSneaking();
	if (var1 != field_9381_bG)
	{
		if (var1)
		{
			field_797_bg->addToSendQueue(new Packet19EntityAction(entityId, 1));
		}
		else
		{
			field_797_bg->addToSendQueue(new Packet19EntityAction(entityId, 2));
		}
		field_9381_bG = var1;
	}

	// Java: double var2 = this.posX - this.field_9379_by;
	//       double var4 = this.boundingBox.minY - this.field_9378_bz;
	//       double var6 = this.posY - this.field_9377_bA;
	//       double var8 = this.posZ - this.field_9376_bB;
	//       double var10 = (double)(this.rotationYaw - this.field_9385_bC);
	//       double var12 = (double)(this.rotationPitch - this.field_9384_bD);
	double var2 = x - field_9379_by;
	double var4 = bb.y0 - field_9378_bz;
	double var6 = y - field_9377_bA;
	double var8 = z - field_9376_bB;
	double var10 = static_cast<double>(yRot - field_9385_bC);
	double var12 = static_cast<double>(xRot - field_9384_bD);

	// Java: boolean var14 = var4 != 0.0D || var6 != 0.0D || var2 != 0.0D || var8 != 0.0D;
	//       boolean var15 = var10 != 0.0D || var12 != 0.0D;
	bool var14 = var4 != 0.0 || var6 != 0.0 || var2 != 0.0 || var8 != 0.0;
	bool var15 = var10 != 0.0 || var12 != 0.0;

	// Java: if(this.ridingEntity != null) {
	//     if(var15) {
	//         this.field_797_bg.addToSendQueue(new Packet11PlayerPosition(this.motionX, -999.0D, -999.0D, this.motionZ, this.onGround));
	//     } else {
	//         this.field_797_bg.addToSendQueue(new Packet13PlayerLookMove(this.motionX, -999.0D, -999.0D, this.motionZ, this.rotationYaw, this.rotationPitch, this.onGround));
	//     }
	//     var14 = false;
	// } else if(var14 && var15) {
	if (riding != nullptr)
	{
		if (var15)
		{
			field_797_bg->addToSendQueue(new Packet11PlayerPosition(xd, -999.0, -999.0, zd, onGround));
		}
		else
		{
			field_797_bg->addToSendQueue(new Packet13PlayerLookMove(xd, -999.0, -999.0, zd, yRot, xRot, onGround));
		}
		var14 = false;
	}
	else if (var14 && var15)
	{
		// Java: this.field_797_bg.addToSendQueue(new Packet13PlayerLookMove(this.posX, this.boundingBox.minY, this.posY, this.posZ, this.rotationYaw, this.rotationPitch, this.onGround));
		field_797_bg->addToSendQueue(new Packet13PlayerLookMove(x, bb.y0, y, z, yRot, xRot, onGround));
		field_12242_bI = 0;
	}
	else if (var14)
	{
		// Java: this.field_797_bg.addToSendQueue(new Packet11PlayerPosition(this.posX, this.boundingBox.minY, this.posY, this.posZ, this.onGround));
		field_797_bg->addToSendQueue(new Packet11PlayerPosition(x, bb.y0, y, z, onGround));
		field_12242_bI = 0;
	}
	else if (var15)
	{
		// Java: this.field_797_bg.addToSendQueue(new Packet12PlayerLook(this.rotationYaw, this.rotationPitch, this.onGround));
		field_797_bg->addToSendQueue(new Packet12PlayerLook(yRot, xRot, onGround));
		field_12242_bI = 0;
	}
	else
	{
		// Java: this.field_797_bg.addToSendQueue(new Packet10Flying(this.onGround));
		//       if(this.field_9382_bF == this.onGround && this.field_12242_bI <= 20) {
		//           ++this.field_12242_bI;
		//       } else {
		//           this.field_12242_bI = 0;
		//       }
		field_797_bg->addToSendQueue(new Packet10Flying(onGround));
		if (field_9382_bF == onGround && field_12242_bI <= 20)
		{
			++field_12242_bI;
		}
		else
		{
			field_12242_bI = 0;
		}
	}

	// Java: this.field_9382_bF = this.onGround;
	field_9382_bF = onGround;
	
	// Java: if(var14) {
	//     this.field_9379_by = this.posX;
	//     this.field_9378_bz = this.boundingBox.minY;
	//     this.field_9377_bA = this.posY;
	//     this.field_9376_bB = this.posZ;
	// }
	// Alpha 1.2.6: When riding, positionRider() updates position every tick, so we need to
	// update tracking fields even when var14 is false (which happens when riding)
	// This prevents position deltas from accumulating and causing jitter
	if (var14 || riding != nullptr)
	{
		field_9379_by = x;
		field_9378_bz = bb.y0;
		field_9377_bA = y;
		field_9376_bB = z;
	}

	// Java: if(var15) {
	//     this.field_9385_bC = this.rotationYaw;
	//     this.field_9384_bD = this.rotationPitch;
	// }
	if (var15)
	{
		field_9385_bC = yRot;
		field_9384_bD = xRot;
	}
}

// Alpha 1.2.6: Update position tracking fields after positionRider() moves the player
// This prevents position deltas from accumulating when riding, which causes jitter
void EntityClientPlayerMP::updatePositionTracking()
{
	// Update tracking fields to match current position (after positionRider() has moved us)
	field_9379_by = x;
	field_9378_bz = bb.y0;
	field_9377_bA = y;
	field_9376_bB = z;
	field_9385_bC = yRot;
	field_9384_bD = xRot;
}

// Alpha 1.2.6: sendInventoryChanged - empty method
// Java: private void sendInventoryChanged() { }
void EntityClientPlayerMP::sendInventoryChanged()
{
}

// Alpha 1.2.6: joinEntityItemWithWorld - empty override
// Java: protected void joinEntityItemWithWorld(EntityItem var1) { }
void EntityClientPlayerMP::joinEntityItemWithWorld(EntityItem& var1)
{
}

// Alpha 1.2.6: sendChatMessage - sends Packet3Chat
// Java: public void sendChatMessage(String var1) {
//     this.field_797_bg.addToSendQueue(new Packet3Chat(var1));
// }
void EntityClientPlayerMP::sendChatMessage(const jstring& var1)
{
	field_797_bg->addToSendQueue(new Packet3Chat(var1));
}

// Alpha 1.2.6: swing() override - calls func_457_w() to send Packet18ArmAnimation
// Java: func_457_w() is called directly from Minecraft.java, but in C++ we override swing()
// to ensure Packet18ArmAnimation is sent when player->swing() is called
void EntityClientPlayerMP::swing()
{
	func_457_w();  // Calls func_457_w() which does super.swing() + sends packet
}

// Alpha 1.2.6: func_457_w - swing arm, sends Packet18ArmAnimation
// Java: public void func_457_w() {
//     super.func_457_w();
//     this.field_797_bg.addToSendQueue(new Packet18ArmAnimation(this, 1));
// }
// Note: func_457_w() in Java EntityPlayerSP corresponds to swing() in our C++ Player class
void EntityClientPlayerMP::func_457_w()
{
	LocalPlayer::swing();  // Java: super.func_457_w(); - calls Player::swing() which sets swinging flag
	field_797_bg->addToSendQueue(new Packet18ArmAnimation(entityId, 1));
}

// Alpha 1.2.6: func_9367_r - respawn, sends Packet9
// Java: public void func_9367_r() {
//     this.sendInventoryChanged();
//     this.field_797_bg.addToSendQueue(new Packet9());
// }
void EntityClientPlayerMP::func_9367_r()
{
	sendInventoryChanged();
	field_797_bg->addToSendQueue(new Packet9());
}

// Alpha 1.2.6: damageEntity - just updates health (server controls damage)
// Java: protected void damageEntity(int var1) {
//     this.health -= var1;
// }
void EntityClientPlayerMP::damageEntity(int_t var1)
{
	health -= var1;
}

// Alpha 1.2.6: setHealth - special behavior for first call
// Java: public void setHealth(int var1) {
//     if(this.field_21093_bH) {
//         super.setHealth(var1);
//     } else {
//         this.health = var1;
//         this.field_21093_bH = true;
//     }
// }
// Note: In Java, the first call just sets health directly, subsequent calls call super.setHealth()
// Since we don't have a base setHealth(), we'll always set health directly
// The flag field_21093_bH tracks if this is the first call (for potential future use)
void EntityClientPlayerMP::setHealth(int_t var1)
{
	if (field_21093_bH)
	{
		// Java: super.setHealth(var1); - subsequent calls
		// Since we don't have base setHealth(), just set it directly
		health = var1;
	}
	else
	{
		// Java: this.health = var1; this.field_21093_bH = true; - first call
		health = var1;
		field_21093_bH = true;
	}
}

// Alpha 1.2.6: dropCurrentItem - sends Packet14BlockDig with status 4
// Java: public void dropCurrentItem() {
//     this.field_797_bg.addToSendQueue(new Packet14BlockDig(4, 0, 0, 0, 0));
// }
// Note: Packet14BlockDig constructor is (status, x, y, z, face)
void EntityClientPlayerMP::dropCurrentItem()
{
	field_797_bg->addToSendQueue(new Packet14BlockDig(4, 0, 0, 0, 0));
}

// Alpha 1.2.6: closeContainer - sends Packet101CloseWindow
// Java: public void closeScreen() {
//     this.field_797_bg.addToSendQueue(new Packet101CloseWindow(this.craftingInventory.windowId));
//     this.inventory.setItemStack((ItemStack)null);
//     super.closeScreen();
// }
// Note: craftingInventory doesn't exist in Alpha 1.2.6, this is Beta behavior
// For Alpha 1.2.6, we'll send with windowId 0 (default inventory)
void EntityClientPlayerMP::closeContainer()
{
	field_797_bg->addToSendQueue(new Packet101CloseWindow());
	// Java: this.inventory.setItemStack((ItemStack)null); - not needed in Alpha 1.2.6
	LocalPlayer::closeContainer();  // Java: super.closeScreen();
}

// Beta 1.2: MultiplayerLocalPlayer.hurtTo() - matches newb12 MultiplayerLocalPlayer.java:165-172 exactly
void EntityClientPlayerMP::hurtTo(int_t newHealth)
{
	// Beta: if (this.flashOnSetHealth) { super.hurtTo(newHealth); } else { ... } (MultiplayerLocalPlayer.java:166-171)
	if (field_21093_bH)  // Beta: flashOnSetHealth flag (field_21093_bH in our code)
	{
		LocalPlayer::hurtTo(newHealth);  // Beta: super.hurtTo(newHealth) (MultiplayerLocalPlayer.java:167)
	}
	else
	{
		health = newHealth;  // Beta: this.health = newHealth (MultiplayerLocalPlayer.java:169)
		field_21093_bH = true;  // Beta: this.flashOnSetHealth = true (MultiplayerLocalPlayer.java:170)
	}
}

// Beta 1.2: MultiplayerLocalPlayer.reallyDrop() - matches newb12 MultiplayerLocalPlayer.java:132-133 exactly
void EntityClientPlayerMP::reallyDrop(std::shared_ptr<EntityItem> itemEntity)
{
	// Beta: Empty override - client doesn't drop items in multiplayer (MultiplayerLocalPlayer.java:132-133)
	// Server handles item dropping via Packet14BlockDig
}

// Beta 1.2: MultiplayerLocalPlayer.chat() - matches newb12 MultiplayerLocalPlayer.java:136-138 exactly
void EntityClientPlayerMP::chat(const jstring& message)
{
	// Beta: this.connection.send(new ChatPacket(message)) (MultiplayerLocalPlayer.java:137)
	field_797_bg->addToSendQueue(new Packet3Chat(message));
}