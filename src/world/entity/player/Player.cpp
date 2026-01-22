#include "world/entity/player/Player.h"

#include "world/level/Level.h"
#include "world/level/tile/Tile.h"
#include "world/level/material/Material.h"
#include "world/entity/item/EntityItem.h"
#include "world/entity/monster/Monster.h"
#include "world/item/ItemStack.h"
#include "world/level/material/LiquidMaterial.h"
#include "util/Mth.h"
#include "nbt/ListTag.h"
#include "util/Memory.h"

static void (*const CS_PlaySound)(int id, int mode) = (void(*)(int, int))0x420640;

Player::Player(Level &level) : Mob(level)
{
	heightOffset = 1.62f;
	moveTo(level.xSpawn + 0.5, level.ySpawn + 1, level.zSpawn + 0.5, 0.0f, 0.0f);
	health = 20;
	modelName = u"humanoid";
	rotOffs = 180.0f;
	flameTime = 20;
	// textureName is already set to "/mob/char.png" in Mob base class, no need to override
}

void Player::tick()
{
	// Beta: Player.tick() - calls super.tick() (Player.java:64-65)
	Mob::tick();
	
	// Beta: Container menu check (Player.java:66-69) - skip for now since containers don't exist in a126cpp
	// if (!level.isOnline && containerMenu != null && !containerMenu.stillValid(this)) {
	//     closeContainer();
	//     containerMenu = inventoryMenu;
	// }
	// Note: Cloak texture updates removed - cloaks don't exist in Alpha 1.2.6
}

void Player::closeContainer()
{
	// Beta: Player.closeContainer() - sets containerMenu to inventoryMenu (Player.java:107-109)
	// containerMenu = inventoryMenu;  // Beta: this.containerMenu = this.inventoryMenu - skip for now since containers don't exist
}

void Player::rideTick()
{
	Mob::rideTick();
	oBob = bob;
	bob = 0.0f;
}

void Player::resetPos()
{
	heightOffset = 1.62f;
	setSize(0.6f, 1.8f);
	Mob::resetPos();
	health = 20;
	deathTime = 0;
}

void Player::updateAi()
{
	if (swinging)
	{
		if (++swingTime == 8)
		{
			swingTime = 0;
			swinging = false;
		}
	}
	else
	{
		swingTime = 0;
	}

	attackAnim = swingTime / 8.0f;
}

void Player::aiStep()
{
	// Beta: RemotePlayer.aiStep() - handles interpolation for remote players (RemotePlayer.java:49-111)
	// Beta: Player.aiStep() - difficulty healing, inventory tick, bob/tilt, entity touch (Player.java:149-183)
	if (level.difficulty == 0 && health < 20 && tickCount % 20 * 12 == 0)  // Beta: this.tickCount % 20 * 12 == 0 (Player.java:150)
	{
		heal(1);  // Beta: this.heal(1) (Player.java:151)
	}

	inventory.tick();  // Beta: this.inventory.tick() (Player.java:154)
	oBob = bob;  // Beta: this.oBob = this.bob (Player.java:155)
	
	// Beta: RemotePlayer.aiStep() - interpolation for remote players (RemotePlayer.java:72-91)
	// Beta: if (this.lSteps > 0) { ... interpolate position ... } (RemotePlayer.java:72)
	if (lSteps > 0)
	{
		// Beta: Interpolate position (RemotePlayer.java:73-75)
		double xt = x + (lx - x) / lSteps;
		double yt = y + (ly - y) / lSteps;
		double zt = z + (lz - z) / lSteps;
		
		// Beta: Interpolate rotation (RemotePlayer.java:76-87)
		double yrd = lyr - yRot;
		while (yrd < -180.0)
		{
			yrd += 360.0;
		}
		while (yrd >= 180.0)
		{
			yrd -= 360.0;
		}
		
		yRot = (float)(yRot + yrd / lSteps);
		xRot = (float)(xRot + (lxr - xRot) / lSteps);
		lSteps--;
		setPos(xt, yt, zt);
		setRot(yRot, xRot);
	}
	
	Mob::aiStep();  // Beta: super.aiStep() (Player.java:156)
	
	float targetBob = Mth::sqrt(xd * xd + zd * zd);  // Beta: float var1 = Mth.sqrt(this.xd * this.xd + this.zd * this.zd) (Player.java:157)
	float targetTilt = (float)std::atan(-yd * 0.2f) * 15.0f;  // Beta: float var2 = (float)Math.atan(-this.yd * 0.2F) * 15.0F (Player.java:158)
	
	if (targetBob > 0.1f)  // Beta: if (var1 > 0.1F) (Player.java:159)
		targetBob = 0.1f;  // Beta: var1 = 0.1F (Player.java:160)

	if (!onGround || health <= 0)  // Beta: if (!this.onGround || this.health <= 0) (Player.java:163)
		targetBob = 0.0f;  // Beta: var1 = 0.0F (Player.java:164)

	if (onGround || health <= 0)  // Beta: if (this.onGround || this.health <= 0) (Player.java:167)
		targetTilt = 0.0f;  // Beta: var2 = 0.0F (Player.java:168)

	bob = bob + (targetBob - bob) * 0.4f;  // Beta: this.bob = this.bob + (var1 - this.bob) * 0.4F (Player.java:171)
	tilt = tilt + (targetTilt - tilt) * 0.8f;  // Beta: this.tilt = this.tilt + (var2 - this.tilt) * 0.8F (Player.java:172)
	
	if (health > 0)  // Beta: if (this.health > 0) (Player.java:173)
	{
		auto &es = level.getEntities(this, *bb.grow(1.0, 0.0, 1.0));  // Beta: List var3 = this.level.getEntities(this, this.bb.grow(1.0, 0.0, 1.0)) (Player.java:174)
		if (!es.empty())  // Beta: if (var3 != null) (Player.java:175)
		{
			for (const auto &e : es)  // Beta: for (int var4 = 0; var4 < var3.size(); var4++) (Player.java:176)
			{
				Entity &entity = *e;  // Beta: Entity var5 = (Entity)var3.get(var4) (Player.java:177)
				if (!entity.removed)  // Beta: if (!var5.removed) (Player.java:178)
				{
					touch(entity);  // Beta: this.touch(var5) (Player.java:179)
				}
			}
		}
	}
}

void Player::touch(Entity &e)
{
	e.playerTouch(*this);
}

void Player::swing()
{
	swingTime = -1;
	swinging = true;
}

void Player::respawn()
{
	// Beta: Player.respawn() - empty (Player.java:409-410)
	// Beta 1.2: Player.respawn() is empty - respawn handled in Minecraft.respawnPlayer()
}

float Player::getDestroySpeed(Tile &tile)
{
	// Beta: Player.getDestroySpeed() - gets speed from inventory, applies water/air penalties (Player.java:274-285)
	float speed = inventory.getStrVsBlock(tile);  // Beta: float var2 = this.inventory.getDestroySpeed(var1) (Player.java:275)
	
	// Beta: Slow down mining if head is underwater (includes swimming) (Player.java:276-278)
	const Material &waterMat = Material::water;
	if (isUnderLiquid(waterMat))  // Beta: if (this.isUnderLiquid(Material.water)) (Player.java:276)
	{
		speed /= 5.0f;  // Beta: var2 /= 5.0F (Player.java:277)
	}

	if (!onGround)  // Beta: if (!this.onGround) (Player.java:280)
	{
		speed /= 5.0f;  // Beta: var2 /= 5.0F (Player.java:281)
	}

	return speed;  // Beta: return var2 (Player.java:284)
}

bool Player::canDestroy(Tile &tile)
{
	// Alpha: canHarvestBlock() from EntityPlayer.java:237-239
	// Delegates to inventory.canHarvestBlock()
	return inventory.canHarvestBlock(tile);
}

float Player::getHeadHeight()
{
	return 0.12f;
}

// Testing: override getTexture in Player
jstring Player::getTexture()
{
	return textureName;  // Same as base class, just for testing
}

// Beta: Player helper methods (Player.java:190-196, 376-387)
bool Player::addResource(int_t itemID)
{
	// Beta: Player.addResource(int var1) - adds item to inventory (Player.java:190-192)
	ItemStack stack(itemID, 1, 0);  // Beta: new ItemInstance(var1, 1, 0) (Player.java:191)
	return inventory.addItemStackToInventory(stack);  // Beta: this.inventory.add(var2) (Player.java:191)
}

int_t Player::getScore() const
{
	// Beta: Player.getScore() - returns score (Player.java:194-196)
	return score;  // Beta: return this.score (Player.java:195)
}

void Player::take(Entity &entity, int_t count)
{
	// Beta: Player.take(Entity var1, int var2) - empty method (Player.java:312-313)
	// Beta 1.2: This method is empty but is called during item pickup
}

ItemStack *Player::getSelectedItem()
{
	// Beta: Player.getSelectedItem() - returns inventory.getSelected() (Player.java:376-378)
	return inventory.getSelected();  // Beta: return this.inventory.getSelected() (Player.java:377)
}

void Player::removeSelectedItem()
{
	// Beta: Player.removeSelectedItem() - sets selected slot to null (Player.java:380-382)
	ItemStack empty;  // Beta: null (Player.java:381)
	inventory.setItem(inventory.currentItem, empty);  // Beta: this.inventory.setItem(this.inventory.selected, null) (Player.java:381)
}

double Player::getRidingHeight()
{
	// Beta: Player.getRidingHeight() - returns heightOffset - 0.5F (Player.java:385-387)
	return heightOffset - 0.5;  // Beta: return this.heightOffset - 0.5F (Player.java:386)
}

// Beta: Player drop methods (Player.java:234-272)
void Player::drop()
{
	// Beta: Player.drop() - drops selected item (Player.java:234-236)
	ItemStack *selected = inventory.removeItem(inventory.currentItem, 1);  // Beta: this.inventory.removeItem(this.inventory.selected, 1) (Player.java:235)
	if (selected != nullptr)
	{
		drop(*selected, false);  // Beta: this.drop(var1, false) (Player.java:235)
		delete selected;  // Clean up the returned ItemStack
	}
}

void Player::drop(ItemStack &stack)
{
	// Beta: Player.drop(ItemInstance var1) - drops item with default spread (Player.java:238-240)
	drop(stack, false);  // Beta: this.drop(var1, false) (Player.java:239)
}

void Player::drop(ItemStack &stack, bool randomSpread)
{
	// Beta: Player.drop(ItemInstance var1, boolean var2) - drops item (Player.java:242-268)
	if (stack.isEmpty())  // Beta: if (var1 != null) (Player.java:243)
		return;

	// Beta: Create EntityItem (Player.java:244)
	double dropY = y - 0.3f + getHeadHeight();  // Beta: this.y - 0.3F + this.getHeadHeight() (Player.java:244)
	auto itemEntity = std::make_shared<EntityItem>(level, x, dropY, z, stack);  // Beta: new ItemEntity(this.level, this.x, var3, this.z, var1) (Player.java:244)
	itemEntity->throwTime = 40;  // Beta: var3.throwTime = 40 (Player.java:245)
	
	float speed = 0.1f;  // Beta: float var4 = 0.1F (Player.java:246)
	if (randomSpread)  // Beta: if (var2) (Player.java:247)
	{
		// Beta: Random spread (Player.java:248-252)
		float spread = random.nextFloat() * 0.5f;  // Beta: float var5 = this.random.nextFloat() * 0.5F (Player.java:248)
		float angle = random.nextFloat() * Mth::PI * 2.0f;  // Beta: float var6 = this.random.nextFloat() * (float)Math.PI * 2.0F (Player.java:249)
		itemEntity->xd = -Mth::sin(angle) * spread;  // Beta: var3.xd = -Mth.sin(var6) * var5 (Player.java:250)
		itemEntity->zd = Mth::cos(angle) * spread;  // Beta: var3.zd = Mth.cos(var6) * var5 (Player.java:251)
		itemEntity->yd = 0.2f;  // Beta: var3.yd = 0.2F (Player.java:252)
	}
	else
	{
		// Beta: Directional throw (Player.java:254-263)
		speed = 0.3f;  // Beta: var4 = 0.3F (Player.java:254)
		float yRotRad = yRot * Mth::DEGRAD;  // Beta: this.yRot / 180.0F * (float)Math.PI (Player.java:255)
		float xRotRad = xRot * Mth::DEGRAD;  // Beta: this.xRot / 180.0F * (float)Math.PI (Player.java:255)
		itemEntity->xd = -Mth::sin(yRotRad) * Mth::cos(xRotRad) * speed;  // Beta: var3.xd = -Mth.sin(var7) * Mth.cos(var8) * var4 (Player.java:255)
		itemEntity->zd = Mth::cos(yRotRad) * Mth::cos(xRotRad) * speed;  // Beta: var3.zd = Mth.cos(var7) * Mth.cos(var8) * var4 (Player.java:256)
		itemEntity->yd = -Mth::sin(xRotRad) * speed + 0.1f;  // Beta: var3.yd = -Mth.sin(var8) * var4 + 0.1F (Player.java:257)
		speed = 0.02f;  // Beta: var4 = 0.02F (Player.java:258)
		float randomAngle = random.nextFloat() * Mth::PI * 2.0f;  // Beta: float var10 = this.random.nextFloat() * (float)Math.PI * 2.0F (Player.java:259)
		speed *= random.nextFloat();  // Beta: var4 *= this.random.nextFloat() (Player.java:260)
		itemEntity->xd += Mth::cos(randomAngle) * speed;  // Beta: var3.xd = var3.xd + Math.cos(var10) * var4 (Player.java:261)
		itemEntity->yd += (random.nextFloat() - random.nextFloat()) * 0.1f;  // Beta: var3.yd = var3.yd + (this.random.nextFloat() - this.random.nextFloat()) * 0.1F (Player.java:262)
		itemEntity->zd += Mth::sin(randomAngle) * speed;  // Beta: var3.zd = var3.zd + Math.sin(var10) * var4 (Player.java:263)
	}
	
	this->reallyDrop(itemEntity);  // Beta: this.reallyDrop(var3) (Player.java:266)
}

void Player::reallyDrop(std::shared_ptr<EntityItem> itemEntity)
{
	// Beta: Player.reallyDrop(ItemEntity var1) - adds entity to level (Player.java:270-272)
	this->level.addEntity(itemEntity);  // Beta: this.level.addEntity(var1) (Player.java:271)
}

// Beta: Player die() override (Player.java:199-217)
void Player::die(Entity *source)
{
	// Beta: Player.die(Entity var1) - drops inventory and changes size (Player.java:199-217)
	Mob::die(source);  // Beta: super.die(var1) (Player.java:200)
	setSize(0.2f, 0.2f);  // Beta: this.setSize(0.2F, 0.2F) (Player.java:201)
	setPos(x, y, z);  // Beta: this.setPos(this.x, this.y, this.z) (Player.java:202)
	yd = 0.1f;  // Beta: this.yd = 0.1F (Player.java:203)
	
	// Beta: Notch easter egg - drop apple (Player.java:204-206)
	if (name == u"Notch")  // Beta: if (this.name.equals("Notch")) (Player.java:204)
	{
		ItemStack apple(256 + 4, 1, 0);  // Beta: new ItemInstance(Item.apple, 1) - apple is item ID 4, shiftedIndex = 256 + 4
		drop(apple, true);  // Beta: this.drop(var2, true) (Player.java:205)
	}
	
	inventory.dropAll();  // Beta: this.inventory.dropAll() (Player.java:208)
	if (source != nullptr)  // Beta: if (var1 != null) (Player.java:209)
	{
		// Beta: Knockback direction (Player.java:210-211)
		float angle = (hurtDir + yRot) * Mth::DEGRAD;  // Beta: (this.hurtDir + this.yRot) * (float)Math.PI / 180.0F (Player.java:210)
		xd = -Mth::cos(angle) * 0.1f;  // Beta: this.xd = -Mth.cos(var2) * 0.1F (Player.java:210)
		zd = -Mth::sin(angle) * 0.1f;  // Beta: this.zd = -Mth.sin(var2) * 0.1F (Player.java:211)
	}
	else
	{
		// Beta: No knockback (Player.java:213)
		xd = zd = 0.0;  // Beta: this.xd = this.zd = 0.0 (Player.java:213)
	}
	
	heightOffset = 0.1f;  // Beta: this.heightOffset = 0.1F (Player.java:216)
}

bool Player::hurt(Entity *source, int_t dmg)
{
	// Beta: Player.hurt(Entity var1, int var2) - difficulty scaling for Monster/Arrow (Player.java:321-342)
	noActionTime = 0;  // Beta: this.noActionTime = 0 (Player.java:322)
	
	if (health <= 0)  // Beta: if (this.health <= 0) (Player.java:323)
		return false;  // Beta: return false (Player.java:324)
	
	// Beta: Difficulty scaling for Monster/Arrow (Player.java:326-338)
	if (source != nullptr)  // Beta: if (var1 instanceof Monster || var1 instanceof Arrow) (Player.java:326)
	{
		Monster *monster = dynamic_cast<Monster*>(source);  // Beta: Check if Monster
		// TODO: Check for Arrow when it exists in a126cpp
		if (monster != nullptr)  // Beta: if (var1 instanceof Monster || var1 instanceof Arrow) (Player.java:326)
		{
			if (level.difficulty == 0)  // Beta: if (this.level.difficulty == 0) (Player.java:327)
			{
				dmg = 0;  // Beta: var2 = 0 (Player.java:328)
			}
			
			if (level.difficulty == 1)  // Beta: if (this.level.difficulty == 1) (Player.java:331)
			{
				dmg = dmg / 3 + 1;  // Beta: var2 = var2 / 3 + 1 (Player.java:332)
			}
			
			if (level.difficulty == 3)  // Beta: if (this.level.difficulty == 3) (Player.java:335)
			{
				dmg = dmg * 3 / 2;  // Beta: var2 = var2 * 3 / 2 (Player.java:336)
			}
		}
	}
	
	return dmg == 0 ? false : Mob::hurt(source, dmg);  // Beta: return var2 == 0 ? false : super.hurt(var1, var2) (Player.java:340)
}

void Player::actuallyHurt(int_t dmg)
{
	// Beta: Player.actuallyHurt(int var1) - armor damage reduction with dmgSpill (Player.java:345-352)
	int_t armorValue = 25 - inventory.getArmorValue();  // Beta: int var2 = 25 - this.inventory.getArmorValue() (Player.java:346)
	int_t totalDmg = dmg * armorValue + dmgSpill;  // Beta: int var3 = var1 * var2 + this.dmgSpill (Player.java:347)
	inventory.hurtArmor(dmg);  // Beta: this.inventory.hurtArmor(var1) (Player.java:348)
	dmg = totalDmg / 25;  // Beta: var1 = var3 / 25 (Player.java:349)
	dmgSpill = totalDmg % 25;  // Beta: this.dmgSpill = var3 % 25 (Player.java:350)
	Mob::actuallyHurt(dmg);  // Beta: super.actuallyHurt(var1) (Player.java:351)
}

void Player::interact(const std::shared_ptr<Entity> &entity)
{
	// Beta: Player.interact(Entity var1) - selected item interaction (Player.java:363-374)
	if (entity->interact(*this))  // Beta: if (!var1.interact(this)) (Player.java:364)
		return;  // Beta: return early if entity handled interaction (Player.java:364)
	
	ItemStack *selected = getSelectedItem();  // Beta: ItemInstance var2 = this.getSelectedItem() (Player.java:365)
	if (selected != nullptr)  // Beta: if (var2 != null) (Player.java:365)
	{
		Mob *mob = dynamic_cast<Mob*>(entity.get());  // Beta: if (var1 instanceof Mob) (Player.java:366)
		if (mob != nullptr)
		{
			// Beta: Item interaction with mob (Player.java:367-371)
			// TODO: selected->interactEnemy(*mob); - need to add interactEnemy() to ItemStack
			// if (selected->stackSize <= 0)  // Beta: if (var2.count <= 0) (Player.java:368)
			// {
			//     selected->snap(*this);  // Beta: var2.snap(this) (Player.java:369)
			//     removeSelectedItem();  // Beta: this.removeSelectedItem() (Player.java:370)
			// }
		}
	}
}

void Player::attack(const std::shared_ptr<Entity> &entity)
{
	// Beta: Player.attack(Entity var1) - attack damage and item damage (Player.java:394-407)
	int_t attackDmg = inventory.getAttackDamage(*entity);  // Beta: int var2 = this.inventory.getAttackDamage(var1) (Player.java:395)
	if (attackDmg > 0)  // Beta: if (var2 > 0) (Player.java:396)
	{
		entity->hurt(this, attackDmg);  // Beta: var1.hurt(this, var2) (Player.java:397)
		ItemStack *selected = getSelectedItem();  // Beta: ItemInstance var3 = this.getSelectedItem() (Player.java:398)
		if (selected != nullptr)  // Beta: if (var3 != null) (Player.java:398)
		{
			Mob *mob = dynamic_cast<Mob*>(entity.get());  // Beta: if (var1 instanceof Mob) (Player.java:399)
			if (mob != nullptr)
			{
				// Beta: Item damage on attack (Player.java:400-404)
				selected->hitEntity(*mob);  // Beta: var3.hurtEnemy((Mob)var1) - use hitEntity() for now
				if (selected->stackSize <= 0)  // Beta: if (var3.count <= 0) (Player.java:401)
				{
					// TODO: selected->snap(*this);  // Beta: var3.snap(this) (Player.java:402)
					removeSelectedItem();  // Beta: this.removeSelectedItem() (Player.java:403)
				}
			}
		}
	}
}

void Player::remove()
{
	// Beta: Player.remove() - container cleanup (Player.java:420-426)
	Entity::remove();  // Beta: super.remove() (Player.java:421)
	// TODO: inventoryMenu and containerMenu don't exist in a126cpp yet
	// inventoryMenu.removed(this);  // Beta: this.inventoryMenu.removed(this) (Player.java:422)
	// if (containerMenu != nullptr)  // Beta: if (this.containerMenu != null) (Player.java:423)
	// {
	//     containerMenu->removed(this);  // Beta: this.containerMenu.removed(this) (Player.java:424)
	// }
}

// Beta: Player helper methods (Player.java:220-232)
void Player::awardKillScore(Entity &source, int_t dmg)
{
	// Beta: Player.awardKillScore(Entity var1, int var2) - adds to score (Player.java:220-222)
	score += dmg;  // Beta: this.score += var2 (Player.java:221)
}

bool Player::isShootable()
{
	// Beta: Player.isShootable() - returns true (Player.java:225-227)
	return true;  // Beta: return true (Player.java:226)
}

bool Player::isCreativeModeAllowed()
{
	// Beta: Player.isCreativeModeAllowed() - returns true (Player.java:230-232)
	return true;  // Beta: return true (Player.java:231)
}

// Beta: Player.addAdditionalSaveData() - saves inventory and dimension (Player.java:300-304)
void Player::addAdditionalSaveData(CompoundTag &tag)
{
	Mob::addAdditionalSaveData(tag);
	
	std::shared_ptr<ListTag> inventoryTag = Util::make_shared<ListTag>();
	inventory.save(*inventoryTag);
	tag.put(u"Inventory", inventoryTag);
	
	tag.putInt(u"Dimension", dimension);
}

// Beta: Player.readAdditionalSaveData() - loads inventory and dimension (Player.java:292-297)
void Player::readAdditionalSaveData(CompoundTag &tag)
{
	Mob::readAdditionalSaveData(tag);
	
	std::shared_ptr<ListTag> inventoryTag = tag.getList(u"Inventory");
	if (inventoryTag != nullptr)
	{
		inventory.load(*inventoryTag);
	}
	
	dimension = tag.getInt(u"Dimension");
}
