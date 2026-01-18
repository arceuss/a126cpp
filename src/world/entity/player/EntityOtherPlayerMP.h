#pragma once

#include "world/entity/player/Player.h"

// Alpha 1.2.6: EntityOtherPlayerMP - represents other players in multiplayer
// Reference: alpha1.2.6/minecraft/src/net/minecraft/src/EntityOtherPlayerMP.java
// Key difference: yOffset = 0.0F (unlike regular Player which has heightOffset = 1.62f)
class EntityOtherPlayerMP : public Player
{
public:
	EntityOtherPlayerMP(Level &level, const jstring &name);
	
	// Alpha 1.2.6: EntityOtherPlayerMP.setPositionAndRotation2() - sets yOffset to 0.0F
	// Java: public void setPositionAndRotation2(double var1, double var3, double var5, float var7, float var8, int var9) {
	//     this.yOffset = 0.0F;
	//     ...
	// }
	void lerpTo(double x, double y, double z, float yRot, float xRot, int_t steps) override;
	
	// Alpha 1.2.6: EntityOtherPlayerMP.func_392_h_() - returns 0.0F
	// Java: public float func_392_h_() { return 0.0F; }
	float getHeadHeight() override;
	
	// Beta: RemotePlayer.aiStep() - handles interpolation for remote players (RemotePlayer.java:70-110)
	// Beta: Overrides Player.aiStep() to do interpolation BEFORE bob/tilt calculations
	void aiStep() override;
	
	// Beta: RemotePlayer.tick() - updates walk animation (RemotePlayer.java:49-62)
	void tick() override;
	
	// Alpha 1.2.6: EntityOtherPlayerMP.outfitWithItem() - updates inventory from Packet5PlayerInventory
	// Java: public void outfitWithItem(int var1, int var2, int var3) {
	//       ItemStack var4 = null;
	//       if(var2 >= 0) { var4 = new ItemStack(var2, 1, var3); }
	//       if(var1 == 0) { this.inventory.mainInventory[this.inventory.currentItem] = var4; }
	//       else { this.inventory.armorInventory[var1 - 1] = var4; } }
	void outfitWithItem(int_t slot, int_t itemID, int_t itemDamage);
};
