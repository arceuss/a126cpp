#include "world/entity/player/EntityOtherPlayerMP.h"
#include "world/level/Level.h"
#include "java/String.h"
#include "util/Mth.h"
#include <cmath>
#include <iostream>

// Alpha 1.2.6: EntityOtherPlayerMP constructor
// Java: public EntityOtherPlayerMP(World var1, String var2) {
//     super(var1);
//     this.field_771_i = var2;
//     this.yOffset = 0.0F;
//     this.field_9286_aZ = 0.0F;
//     ...
// }
EntityOtherPlayerMP::EntityOtherPlayerMP(Level &level, const jstring &name) : Player(level)
{
	this->name = name;
	// Alpha 1.2.6: Set heightOffset to 0.0F (Java: this.yOffset = 0.0F)
	// This is the key difference from regular Player (which has heightOffset = 1.62f)
	heightOffset = 0.0f;
	
	// Java: this.field_9286_aZ = 0.0F; (not needed in C++ port)
	
	// Alpha 1.2.6: Set customTextureUrl for skin loading (EntityOtherPlayerMP.java:17-19)
	// Java: if(var2 != null && var2.length() > 0) {
	//     this.skinUrl = "http://betacraft.uk:11705/skin/" + var2 + ".png";
	//     System.out.println("Loading texture " + this.skinUrl);
	// }
	if (!name.empty())
	{
		customTextureUrl = u"http://betacraft.uk:11705/skin/" + name + u".png";
		std::cout << "Loading texture " << String::toUTF8(customTextureUrl) << std::endl;
	}
	
	// Java: this.field_9314_ba = true; (not needed in C++ port)
	// Java: this.field_619_ac = 10.0D; (not needed in C++ port)
}

void EntityOtherPlayerMP::lerpTo(double x, double y, double z, float yRot, float xRot, int_t steps)
{
	// Alpha 1.2.6: EntityOtherPlayerMP.setPositionAndRotation2() - sets yOffset to 0.0F
	// Java: public void setPositionAndRotation2(double var1, double var3, double var5, float var7, float var8, int var9) {
	//     this.yOffset = 0.0F;
	//     this.field_784_bh = var1;
	//     this.field_783_bi = var3;
	//     this.field_782_bj = var5;
	//     this.field_780_bk = (double)var7;
	//     this.field_786_bl = (double)var8;
	//     this.field_785_bg = var9;
	// }
	// Ensure heightOffset is 0.0F before calling parent lerpTo
	heightOffset = 0.0f;
	Mob::lerpTo(x, y, z, yRot, xRot, steps);
}

float EntityOtherPlayerMP::getHeadHeight()
{
	// Alpha 1.2.6: EntityOtherPlayerMP.func_392_h_() - returns 0.0F
	// Java: public float func_392_h_() { return 0.0F; }
	return 0.0f;
}

// Beta: RemotePlayer.tick() - updates walk animation (RemotePlayer.java:49-62)
void EntityOtherPlayerMP::tick()
{
	Player::tick();  // Beta: super.tick() (RemotePlayer.java:51)
	
	// Beta: Update walk animation speed (RemotePlayer.java:52-61)
	walkAnimSpeedO = walkAnimSpeed;  // Beta: this.walkAnimSpeedO = this.walkAnimSpeed (RemotePlayer.java:52)
	double xxd = x - xo;  // Beta: double xxd = this.x - this.xo (RemotePlayer.java:53)
	double zzd = z - zo;  // Beta: double zzd = this.z - this.zo (RemotePlayer.java:54)
	float wst = Mth::sqrt(xxd * xxd + zzd * zzd) * 4.0f;  // Beta: float wst = Mth.sqrt(xxd * xxd + zzd * zzd) * 4.0F (RemotePlayer.java:55)
	if (wst > 1.0f)  // Beta: if (wst > 1.0F) (RemotePlayer.java:56)
	{
		wst = 1.0f;  // Beta: wst = 1.0F (RemotePlayer.java:57)
	}
	
	walkAnimSpeed = walkAnimSpeed + (wst - walkAnimSpeed) * 0.4f;  // Beta: this.walkAnimSpeed = this.walkAnimSpeed + (wst - this.walkAnimSpeed) * 0.4F (RemotePlayer.java:60)
	walkAnimPos = walkAnimPos + walkAnimSpeed;  // Beta: this.walkAnimPos = this.walkAnimPos + this.walkAnimSpeed (RemotePlayer.java:61)
}

// Beta: RemotePlayer.aiStep() - handles interpolation for remote players (RemotePlayer.java:70-110)
// Beta: Overrides Player.aiStep() to do interpolation BEFORE bob/tilt calculations
void EntityOtherPlayerMP::aiStep()
{
	// Beta: Call updateAi() first (swing animation logic) (RemotePlayer.java:71)
	// Beta: RemotePlayer calls super.updateAi(), not super.aiStep()
	updateAi();  // Beta: super.updateAi() (RemotePlayer.java:71)
	
	// Beta: Interpolation logic (RemotePlayer.java:72-91)
	if (lSteps > 0)  // Beta: if (this.lSteps > 0) (RemotePlayer.java:72)
	{
		// Beta: Interpolate position (RemotePlayer.java:73-75)
		double xt = x + (lx - x) / lSteps;  // Beta: double xt = this.x + (this.lx - this.x) / this.lSteps (RemotePlayer.java:73)
		double yt = y + (ly - y) / lSteps;  // Beta: double yt = this.y + (this.ly - this.y) / this.lSteps (RemotePlayer.java:74)
		double zt = z + (lz - z) / lSteps;  // Beta: double zt = this.z + (this.lz - this.z) / this.lSteps (RemotePlayer.java:75)
		
		// Beta: Interpolate rotation (RemotePlayer.java:76-87)
		double yrd = lyr - yRot;  // Beta: double yrd = this.lyr - this.yRot (RemotePlayer.java:76)
		while (yrd < -180.0)  // Beta: while (yrd < -180.0) (RemotePlayer.java:78)
		{
			yrd += 360.0;  // Beta: yrd += 360.0 (RemotePlayer.java:79)
		}
		while (yrd >= 180.0)  // Beta: while (yrd >= 180.0) (RemotePlayer.java:82)
		{
			yrd -= 360.0;  // Beta: yrd -= 360.0 (RemotePlayer.java:83)
		}
		
		yRot = (float)(yRot + yrd / lSteps);  // Beta: this.yRot = (float)(this.yRot + yrd / this.lSteps) (RemotePlayer.java:86)
		xRot = (float)(xRot + (lxr - xRot) / lSteps);  // Beta: this.xRot = (float)(this.xRot + (this.lxr - this.xRot) / this.lSteps) (RemotePlayer.java:87)
		lSteps--;  // Beta: this.lSteps-- (RemotePlayer.java:88)
		setPos(xt, yt, zt);  // Beta: this.setPos(xt, yt, zt) (RemotePlayer.java:89)
		setRot(yRot, xRot);  // Beta: this.setRot(this.yRot, this.xRot) (RemotePlayer.java:90)
	}
	
	// Beta: Bob/tilt calculations (RemotePlayer.java:93-109)
	oBob = bob;  // Beta: this.oBob = this.bob (RemotePlayer.java:93)
	float tBob = Mth::sqrt(xd * xd + zd * zd);  // Beta: float tBob = Mth.sqrt(this.xd * this.xd + this.zd * this.zd) (RemotePlayer.java:94)
	float tTilt = (float)std::atan(-yd * 0.2f) * 15.0f;  // Beta: float tTilt = (float)Math.atan(-this.yd * 0.2F) * 15.0F (RemotePlayer.java:95)
	if (tBob > 0.1f)  // Beta: if (tBob > 0.1F) (RemotePlayer.java:96)
	{
		tBob = 0.1f;  // Beta: tBob = 0.1F (RemotePlayer.java:97)
	}
	
	if (!onGround || health <= 0)  // Beta: if (!this.onGround || this.health <= 0) (RemotePlayer.java:100)
	{
		tBob = 0.0f;  // Beta: tBob = 0.0F (RemotePlayer.java:101)
	}
	
	if (onGround || health <= 0)  // Beta: if (this.onGround || this.health <= 0) (RemotePlayer.java:104)
	{
		tTilt = 0.0f;  // Beta: tTilt = 0.0F (RemotePlayer.java:105)
	}
	
	bob = bob + (tBob - bob) * 0.4f;  // Beta: this.bob = this.bob + (tBob - this.bob) * 0.4F (RemotePlayer.java:108)
	tilt = tilt + (tTilt - tilt) * 0.8f;  // Beta: this.tilt = this.tilt + (tTilt - this.tilt) * 0.8F (RemotePlayer.java:109)
}

// Alpha 1.2.6: EntityOtherPlayerMP.outfitWithItem() - updates inventory from Packet5PlayerInventory
// Java: public void outfitWithItem(int var1, int var2, int var3) {
//       ItemStack var4 = null;
//       if(var2 >= 0) { var4 = new ItemStack(var2, 1, var3); }
//       if(var1 == 0) { this.inventory.mainInventory[this.inventory.currentItem] = var4; }
//       else { this.inventory.armorInventory[var1 - 1] = var4; } }
void EntityOtherPlayerMP::outfitWithItem(int_t slot, int_t itemID, int_t itemDamage)
{
	ItemStack stack;
	if (itemID >= 0)
	{
		// Java: new ItemStack(var2, 1, var3) - item ID, stack size 1, damage
		stack = ItemStack(itemID, 1, itemDamage);
	}
	// If itemID < 0, stack remains empty (null in Java)
	
	if (slot == 0)
	{
		// Java: this.inventory.mainInventory[this.inventory.currentItem] = var4;
		inventory.mainInventory[inventory.currentItem] = stack;
	}
	else if (slot >= 1 && slot <= 4)
	{
		// Java: this.inventory.armorInventory[var1 - 1] = var4;
		// Alpha 1.2.6: slot 1-4 maps to armorInventory[0-3]
		inventory.armorInventory[slot - 1] = stack;
	}
}
