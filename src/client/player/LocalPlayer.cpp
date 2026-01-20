#include "client/player/LocalPlayer.h"

#include "client/Minecraft.h"
#include "client/particle/TakeAnimationParticle.h"
#include "client/gui/WorkbenchScreen.h"
#include "client/gui/ChestScreen.h"
#include "client/gui/FurnaceScreen.h"
#include "client/gui/TextEditScreen.h"
#include "client/gui/DeathScreen.h"
#include "world/level/tile/entity/ChestTileEntity.h"
#include "world/level/tile/entity/FurnaceTileEntity.h"
#include "world/level/tile/entity/SignTileEntity.h"
#include "world/CompoundContainer.h"
#include "util/Memory.h"
#include "java/String.h"
#include "client/player/KeyboardInput.h"
#include "client/player/ControllerInput.h"
#include "pc/lwjgl/Gamepad.h"
#include "pc/lwjgl/Keyboard.h"
#include "client/MouseHandler.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <algorithm>

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
	
	// Initialize water tracking for rumble
	wasInWaterLastTick = isInWater();
	
	// Initialize input (will be switched to controller if available)
	updatePlayerInput();
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
	
	// Check if we should switch to controller input (like Controlify)
	ensureCorrectInput();
	
	input->tick(*this);  // Beta: this.input.tick(this) (LocalPlayer.java:79)
	
	if (input->sneaking && ySlideOffset < 0.2f)  // Beta: if (this.input.sneaking && this.ySlideOffset < 0.2F) (LocalPlayer.java:80)
		ySlideOffset = 0.2f;  // Beta: this.ySlideOffset = 0.2F (LocalPlayer.java:81)
	
	// Check for water/land splash for rumble (Controlify-style)
	// This happens before Player::aiStep() so we can check if we just entered water
	// Note: firstTick is private, so we check tickCount instead (tickCount > 0 means not first tick)
	bool currentlyInWater = isInWater();
	if (currentlyInWater && !wasInWaterLastTick && tickCount > 0)
	{
		// Controlify splash rumble - based on impact force
		ControllerInput* controllerInput = dynamic_cast<ControllerInput*>(input.get());
		if (controllerInput != nullptr)
		{
			// Calculate impact force (matches Controlify formula)
			float f = 0.2f;  // Default multiplier for player (0.9f for passenger)
			float impactForce = (std::min)(1.0f, static_cast<float>(std::sqrt(xd * xd * 0.2f + yd * yd + zd * zd * 0.2f)) * f);  // Use parentheses to avoid Windows.h macro
			
			if (impactForce >= 0.05f)
			{
				float multiplier = (std::min)(1.0f, impactForce / 0.5f);  // Use parentheses to avoid Windows.h macro
				// Controlify uses BasicRumbleEffect.byTime with multiplier * (1-t) for strong, multiplier * 0.5f for weak
				// For simplicity, we'll use constant rumble with average values
				// Controlify: RumbleState(multiplier * (1-t), multiplier * 0.5f) over 10-20 ticks
				int_t durationTicks = impactForce < 0.25f ? 10 : 20;
				float avgStrong = multiplier * 0.5f;  // Average of multiplier * (1-t) from 0 to 1
				float avgWeak = multiplier * 0.5f;
				controllerInput->playRumble(avgStrong, avgWeak, durationTicks);
			}
		}
	}
	wasInWaterLastTick = currentlyInWater;
	
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

void LocalPlayer::updatePlayerInput()
{
	// Controlify-style input switching: use controller if available, otherwise keyboard
	if (shouldBeControllerInput())
	{
		// Get first connected controller
		auto* controllerState = lwjgl::Gamepad::getFirstController();
		if (controllerState != nullptr && controllerState->connected)
		{
			// Use controller input
			auto* controllerInput = new ControllerInput(controllerState, minecraft.options);
			// Set screen open pointer (will be updated each frame in Minecraft::tick)
			controllerInput->setScreenOpenPtr(&minecraft.screenOpenForController);
			input = std::unique_ptr<ControllerInput>(controllerInput);
			return;
		}
	}
	
	// Fall back to keyboard input
	input = Util::make_unique<KeyboardInput>(minecraft.options);
}

void LocalPlayer::ensureCorrectInput()
{
	// Controlify-style: ensure we're using the right input type
	// Check for keyboard/mouse input first - if detected, use keyboard input
	// This allows keyboard/mouse to work even when controller is connected (mixed input)
	bool keyboardInput = false;
	
	// Check for keyboard input (movement keys)
	if (lwjgl::Keyboard::isKeyDown(lwjgl::Keyboard::KEY_W) || lwjgl::Keyboard::isKeyDown(lwjgl::Keyboard::KEY_A) ||
	    lwjgl::Keyboard::isKeyDown(lwjgl::Keyboard::KEY_S) || lwjgl::Keyboard::isKeyDown(lwjgl::Keyboard::KEY_D) ||
	    lwjgl::Keyboard::isKeyDown(lwjgl::Keyboard::KEY_SPACE) || lwjgl::Keyboard::isKeyDown(lwjgl::Keyboard::KEY_LSHIFT))
	{
		keyboardInput = true;
	}
	
	// Check for mouse movement (only if mouse is grabbed, to avoid false positives)
	if (minecraft.mouseGrabbed)
	{
		minecraft.mouseHandler.poll(); // Update mouse state
		float mouseDx = minecraft.mouseHandler.xd;
		float mouseDy = minecraft.mouseHandler.yd;
		if (std::abs(mouseDx) > 0.001f || std::abs(mouseDy) > 0.001f)
		{
			keyboardInput = true;
		}
	}
	
	// If keyboard/mouse input detected, use keyboard input (mixed input mode)
	if (keyboardInput && dynamic_cast<KeyboardInput*>(input.get()) == nullptr)
	{
		updatePlayerInput();
		return;
	}
	
	// Otherwise, check if controller input should be used
	if (shouldBeControllerInput() && dynamic_cast<KeyboardInput*>(input.get()) != nullptr)
	{
		updatePlayerInput();
	}
	else if (!shouldBeControllerInput() && dynamic_cast<ControllerInput*>(input.get()) != nullptr)
	{
		updatePlayerInput();
	}
}

bool LocalPlayer::shouldBeControllerInput()
{
	// Controlify-style: only use controller input if controller is actually being used
	// Allow keyboard/mouse to work even when controller is connected (mixed input)
	auto* controllerState = lwjgl::Gamepad::getFirstController();
	if (controllerState == nullptr || !controllerState->connected)
		return false;
	
	// Check if controller is actually giving input (buttons pressed or sticks moved)
	// This allows keyboard/mouse to work when controller is idle
	bool controllerGivingInput = false;
	if (controllerState->buttonSouth || controllerState->buttonEast || controllerState->buttonWest || 
	    controllerState->buttonNorth || controllerState->buttonStart || controllerState->buttonBack ||
	    controllerState->buttonLeftShoulder || controllerState->buttonRightShoulder ||
	    controllerState->buttonLeftStick || controllerState->buttonRightStick ||
	    controllerState->buttonDpadUp || controllerState->buttonDpadDown ||
	    controllerState->buttonDpadLeft || controllerState->buttonDpadRight)
	{
		controllerGivingInput = true;
	}
	
	// Check stick movement (with deadzone)
	float leftStickX = static_cast<float>(controllerState->leftStickX) / 32767.0f;
	float leftStickY = static_cast<float>(controllerState->leftStickY) / 32767.0f;
	float rightStickX = static_cast<float>(controllerState->rightStickX) / 32767.0f;
	float rightStickY = static_cast<float>(controllerState->rightStickY) / 32767.0f;
	
	if (std::abs(leftStickX) > 0.1f || std::abs(leftStickY) > 0.1f ||
	    std::abs(rightStickX) > 0.1f || std::abs(rightStickY) > 0.1f)
	{
		controllerGivingInput = true;
	}
	
	// Only use controller input if controller is actively being used
	// This allows keyboard/mouse to work when controller is connected but idle
	return controllerGivingInput;
}

void LocalPlayer::addAdditionalSaveData(CompoundTag &tag)
{
	Player::addAdditionalSaveData(tag);
	tag.putInt(u"Dimension", dimension);
}

void LocalPlayer::readAdditionalSaveData(CompoundTag &tag)
{
	Player::readAdditionalSaveData(tag);
	if (tag.contains(u"Dimension"))
		dimension = tag.getInt(u"Dimension");
}

void LocalPlayer::closeContainer()
{
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
	
	Player::take(entity, count);
}

void LocalPlayer::startCrafting(int_t x, int_t y, int_t z)
{
	minecraft.setScreen(Util::make_shared<WorkbenchScreen>(minecraft, level, x, y, z));
}

void LocalPlayer::openContainer(std::shared_ptr<ChestTileEntity> chestEntity)
{
	minecraft.setScreen(Util::make_shared<ChestScreen>(minecraft, chestEntity));
}

void LocalPlayer::openContainer(std::shared_ptr<CompoundContainer> compoundContainer)
{
	minecraft.setScreen(Util::make_shared<ChestScreen>(minecraft, compoundContainer));
}

void LocalPlayer::openFurnace(std::shared_ptr<FurnaceTileEntity> furnaceEntity)
{
	minecraft.setScreen(Util::make_shared<FurnaceScreen>(minecraft, furnaceEntity));
}

void LocalPlayer::openTextEdit(std::shared_ptr<SignTileEntity> signEntity)
{
	minecraft.setScreen(Util::make_shared<TextEditScreen>(minecraft, signEntity));
}

void LocalPlayer::prepareForTick()
{
	// Beta: prepareForTick() - reset movement (LocalPlayer.java:86-88)
	xxa = 0.0f;
	yya = 0.0f;
}

bool LocalPlayer::isSneaking()
{
	return input->sneaking;
}

void LocalPlayer::handleInsidePortal()
{
	// Beta 1.2: handleInsidePortal() (LocalPlayer.java:158-165)
	isInsidePortal = true;
	portalTime += 0.0125f;
	if (portalTime >= 1.0f)
	{
		portalTime = 1.0f;
		changingDimensionDelay = 10;
		minecraft.toggleDimension();
	}
}

void LocalPlayer::hurtTo(int_t newHealth)
{
	// Beta 1.2: hurtTo() (LocalPlayer.java:167-178)
	int_t dmg = health - newHealth;
	if (dmg <= 0)
		return;
	
	hurt(nullptr, dmg);
	if (health <= 0)
	{
		// Death handling - set screen to death screen
		minecraft.setScreen(Util::make_shared<DeathScreen>(minecraft));
	}
}

void LocalPlayer::chat(const jstring& message)
{
	// Beta 1.2: chat() - empty for singleplayer (LocalPlayer.java:147-148)
	// Overridden in MultiplayerLocalPlayer
}

void LocalPlayer::respawn()
{
	Player::respawn();
	minecraft.respawnPlayer();
}

void LocalPlayer::actuallyHurt(int_t dmg)
{
	// Store health before damage to detect if we actually took damage
	int_t healthBefore = health;
	
	// Call parent to handle damage normally
	Player::actuallyHurt(dmg);
	
	// Trigger damage rumble if controller input is active (Controlify-style)
	// Controlify: BasicRumbleEffect.constant(0.8f, 0.5f, 5) - strong=0.8, weak=0.5, 5 ticks
	// Check if health actually decreased (damage was taken)
	if (dmg > 0 && health < healthBefore)
	{
		ControllerInput* controllerInput = dynamic_cast<ControllerInput*>(input.get());
		if (controllerInput != nullptr)
		{
			// Controlify damage rumble: 0.8f strong (low freq), 0.5f weak (high freq), 5 ticks
			// At 20 ticks per second, 5 ticks = 250ms
			// Use a longer duration to ensure it's felt - Controlify uses 5 ticks but we'll use 10 ticks (500ms) for better feel
			controllerInput->playRumble(0.8f, 0.5f, 10);  // 10 ticks = 500ms for better feel
		}
	}
}