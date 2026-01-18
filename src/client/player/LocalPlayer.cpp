#include "client/player/LocalPlayer.h"

#include "client/Minecraft.h"
#include "client/particle/TakeAnimationParticle.h"
#include "client/gui/WorkbenchScreen.h"
#include "client/gui/ChestScreen.h"
#include "client/gui/FurnaceScreen.h"
#include "client/gui/TextEditScreen.h"
#include "world/entity/Entity.h"
#include "world/level/tile/entity/ChestTileEntity.h"
#include "world/level/tile/entity/FurnaceTileEntity.h"
#include "world/level/tile/entity/SignTileEntity.h"
#include "world/CompoundContainer.h"
#include "util/Memory.h"
#include "java/String.h"
#include <iostream>

LocalPlayer::LocalPlayer(Minecraft &minecraft, Level &level, User *user, int_t dimension) : Player(level), minecraft(minecraft)
{
	this->dimension = dimension;
	
	// newb12: Set customTextureUrl for skin loading (LocalPlayer.java:32-35)
	if (user != nullptr && !user->name.empty())
	{
		name = user->name;
		customTextureUrl = u"http://betacraft.uk:11705/skin/" + user->name + u".png";
		std::cout << "Loading texture " << String::toUTF8(customTextureUrl) << std::endl;
	}
}


void LocalPlayer::updateAi()
{
	Player::updateAi();
	xxa = input->xa;
	yya = input->ya;
	jumping = input->jumping;
}

void LocalPlayer::aiStep()
{
	// Beta 1.2: LocalPlayer.aiStep() - matches newb12 LocalPlayer.java:49-85 exactly
	oPortalTime = portalTime;  // Beta: this.oPortalTime = this.portalTime (LocalPlayer.java:50)
	
	if (isInsidePortal)  // Beta: if (this.isInsidePortal) (LocalPlayer.java:51)
	{
		// Beta: if (this.portalTime == 0.0F) { play portal trigger sound } (LocalPlayer.java:52-54)
		if (portalTime == 0.0f)
		{
			minecraft.soundEngine.playUI(u"portal.trigger", 1.0f, random.nextFloat() * 0.4f + 0.8f);
		}
		
		// Beta: this.portalTime += 0.0125F (LocalPlayer.java:56)
		portalTime += 0.0125f;
		
		// Beta: if (this.portalTime >= 1.0F) { ... } (LocalPlayer.java:57-62)
		if (portalTime >= 1.0f)
		{
			portalTime = 1.0f;  // Beta: this.portalTime = 1.0F (LocalPlayer.java:58)
			changingDimensionDelay = 10;  // Beta: this.changingDimensionDelay = 10 (LocalPlayer.java:59)
			minecraft.soundEngine.playUI(u"portal.travel", 1.0f, random.nextFloat() * 0.4f + 0.8f);  // Beta: play portal.travel sound (LocalPlayer.java:60)
			minecraft.toggleDimension();  // Beta: this.minecraft.toggleDimension() (LocalPlayer.java:61)
		}
		
		isInsidePortal = false;  // Beta: this.isInsidePortal = false (LocalPlayer.java:64)
	}
	else
	{
		// Beta: if (this.portalTime > 0.0F) { this.portalTime -= 0.05F; } (LocalPlayer.java:66-68)
		if (portalTime > 0.0f)
		{
			portalTime -= 0.05f;
		}
		
		// Beta: if (this.portalTime < 0.0F) { this.portalTime = 0.0F; } (LocalPlayer.java:70-72)
		if (portalTime < 0.0f)
		{
			portalTime = 0.0f;
		}
	}
	
	// Beta: if (this.changingDimensionDelay > 0) { this.changingDimensionDelay--; } (LocalPlayer.java:75-77)
	if (changingDimensionDelay > 0)
	{
		changingDimensionDelay--;
	}
	
	input->tick(*this);  // Beta: this.input.tick(this) (LocalPlayer.java:79)
	
	if (input->sneaking && ySlideOffset < 0.2f)  // Beta: if (this.input.sneaking && this.ySlideOffset < 0.2F) (LocalPlayer.java:80)
		ySlideOffset = 0.2f;  // Beta: this.ySlideOffset = 0.2F (LocalPlayer.java:81)
	
	Player::aiStep();  // Beta: super.aiStep() (LocalPlayer.java:84)
}

void LocalPlayer::releaseAllKeys()
{
	input->releaseAllKeys();
}

void LocalPlayer::setKey(int_t eventKey, bool eventKeyState)
{
	input->setKey(eventKey, eventKeyState);
}

void LocalPlayer::addAdditionalSaveData(CompoundTag &tag)
{
	Player::addAdditionalSaveData(tag);
}

void LocalPlayer::readAdditionalSaveData(CompoundTag &tag)
{
	Player::readAdditionalSaveData(tag);
}

void LocalPlayer::closeContainer()
{
	Player::closeContainer();
	minecraft.setScreen(nullptr);
}

void LocalPlayer::take(Entity &entity, int_t count)
{
	// Beta: LocalPlayer.take() - adds TakeAnimationParticle (LocalPlayer.java:139-141)
	// Find the entity in the level's entities set to get a shared_ptr (needed to keep it alive for the particle)
	std::shared_ptr<Entity> itemEntityPtr = nullptr;
	for (const auto &e : minecraft.level->getAllEntities())
	{
		if (e.get() == &entity)
		{
			itemEntityPtr = e;
			break;
		}
	}
	
	if (itemEntityPtr)
	{
		std::shared_ptr<Entity> playerPtr = nullptr;
		for (const auto &p : minecraft.level->players)
		{
			if (p.get() == this)
			{
				playerPtr = p;
				break;
			}
		}
		
		if (playerPtr)
		{
			minecraft.particleEngine.add(std::make_unique<TakeAnimationParticle>(*minecraft.level, itemEntityPtr, playerPtr, -0.5f));
		}
	}
}

void LocalPlayer::prepareForTick()
{
	// Beta 1.2: LocalPlayer.prepareForTick() - empty method (LocalPlayer.java:150-151)
}

bool LocalPlayer::isSneaking()
{
	return input->sneaking;
}

// Beta 1.2: LocalPlayer.handleInsidePortal() - matches newb12 LocalPlayer.java:158-165
void LocalPlayer::handleInsidePortal()
{
	// Beta: if (this.changingDimensionDelay > 0) { this.changingDimensionDelay = 10; } else { this.isInsidePortal = true; }
	if (changingDimensionDelay > 0)
	{
		changingDimensionDelay = 10;  // Beta: this.changingDimensionDelay = 10 (LocalPlayer.java:161)
	}
	else
	{
		isInsidePortal = true;  // Beta: this.isInsidePortal = true (LocalPlayer.java:163)
	}
}

// Beta 1.2: LocalPlayer.hurtTo() - matches newb12 LocalPlayer.java:167-178
void LocalPlayer::hurtTo(int_t newHealth)
{
	// Beta: int dmg = this.health - newHealth; (LocalPlayer.java:168)
	int_t dmg = health - newHealth;
	
	// Beta: if (dmg <= 0) { this.health = newHealth; } else { ... } (LocalPlayer.java:169-177)
	if (dmg <= 0)
	{
		health = newHealth;  // Beta: this.health = newHealth (LocalPlayer.java:170)
	}
	else
	{
		lastHurt = dmg;  // Beta: this.lastHurt = dmg (LocalPlayer.java:172)
		lastHealth = health;  // Beta: this.lastHealth = this.health (LocalPlayer.java:173)
		invulnerableTime = invulnerableDuration;  // Beta: this.invulnerableTime = this.invulnerableDuration (LocalPlayer.java:174)
		actuallyHurt(dmg);  // Beta: this.actuallyHurt(dmg) (LocalPlayer.java:175)
		hurtTime = hurtDuration = 10;  // Beta: this.hurtTime = this.hurtDuration = 10 (LocalPlayer.java:176)
	}
}

// Beta 1.2: LocalPlayer.chat() - empty method (LocalPlayer.java:147-148)
void LocalPlayer::chat(const jstring& message)
{
	// Beta: Empty method - overridden in MultiplayerLocalPlayer to send ChatPacket
}

void LocalPlayer::respawn()
{
	// Beta: LocalPlayer.respawn() - calls minecraft.respawnPlayer() (LocalPlayer.java:181-183)
	minecraft.respawnPlayer();  // Beta: this.minecraft.respawnPlayer() (LocalPlayer.java:182)
}

// Beta: LocalPlayer.startCrafting() - opens workbench screen (LocalPlayer.java:124-126)
void LocalPlayer::startCrafting(int_t x, int_t y, int_t z)
{
	// Beta: this.minecraft.setScreen(new CraftingScreen(this.inventory, this.level, x, y, z)) (LocalPlayer.java:125)
	minecraft.setScreen(Util::make_shared<WorkbenchScreen>(minecraft, level, x, y, z));
}

// Beta: LocalPlayer.openContainer() - opens chest screen (LocalPlayer.java:119-121)
void LocalPlayer::openContainer(std::shared_ptr<ChestTileEntity> chestEntity)
{
	// Beta: this.minecraft.setScreen(new ContainerScreen(this.inventory, container)) (LocalPlayer.java:120)
	minecraft.setScreen(Util::make_shared<ChestScreen>(minecraft, chestEntity));
}

// Beta: LocalPlayer.openContainer() - opens double chest screen
void LocalPlayer::openContainer(std::shared_ptr<CompoundContainer> compoundContainer)
{
	minecraft.setScreen(Util::make_shared<ChestScreen>(minecraft, compoundContainer));
}

// Beta: LocalPlayer.openFurnace() - opens furnace screen (LocalPlayer.java:129-131)
void LocalPlayer::openFurnace(std::shared_ptr<FurnaceTileEntity> furnaceEntity)
{
	// Beta: this.minecraft.setScreen(new FurnaceScreen(this.inventory, furnace)) (LocalPlayer.java:130)
	minecraft.setScreen(Util::make_shared<FurnaceScreen>(minecraft, furnaceEntity));
}

// Beta: LocalPlayer.openTextEdit() - opens sign edit screen (LocalPlayer.java:114-116)
void LocalPlayer::openTextEdit(std::shared_ptr<SignTileEntity> signEntity)
{
	// Beta: this.minecraft.setScreen(new TextEditScreen(sign)) (LocalPlayer.java:115)
	minecraft.setScreen(Util::make_shared<TextEditScreen>(minecraft, signEntity));
}