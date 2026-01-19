#include "client/gui/TextEditScreen.h"

#include "client/Minecraft.h"
#include "client/gui/Button.h"
#include "client/renderer/tileentity/TileEntityRenderDispatcher.h"
#include "world/level/tile/Tile.h"
#include "world/level/tile/SignTile.h"
#include "network/Packet130UpdateSign.h"
#include "client/multiplayer/MultiPlayerLevel.h"
#include "network/NetClientHandler.h"
#include "SharedConstants.h"
#include "lwjgl/Keyboard.h"
#include "lwjgl/GLContext.h"
#include "OpenGL.h"
#include "util/Memory.h"
#include <SDL3/SDL.h>

// Beta: TextEditScreen(SignTileEntity sign) (TextEditScreen.java:20-22)
TextEditScreen::TextEditScreen(Minecraft &minecraft, std::shared_ptr<SignTileEntity> sign)
	: Screen(minecraft), sign(sign)
{
}

// Beta: TextEditScreen.init() (TextEditScreen.java:24-29)
void TextEditScreen::init()
{
	buttons.clear();
	lwjgl::Keyboard::enableRepeatEvents(true);
	buttons.push_back(std::make_shared<Button>(0, width / 2 - 100, height / 4 + 120, u"Done"));
	
	// SDL3: Start text input when sign editing screen opens
	SDL_Window *window = lwjgl::GLContext::detail::getWindow();
	if (window)
		SDL_StartTextInput(window);
}

// Beta: TextEditScreen.removed() (TextEditScreen.java:31-37)
void TextEditScreen::removed()
{
	lwjgl::Keyboard::enableRepeatEvents(false);
	
	// SDL3: Stop text input when sign editing screen closes
	SDL_Window *window = lwjgl::GLContext::detail::getWindow();
	if (window)
		SDL_StopTextInput(window);
	
	// Beta: if (this.minecraft.level.isOnline) { send SignUpdatePacket } (TextEditScreen.java:34-35)
	if (minecraft.level != nullptr && minecraft.level->isOnline)
	{
		// Get connection from MultiPlayerLevel
		MultiPlayerLevel *mpLevel = dynamic_cast<MultiPlayerLevel *>(minecraft.level.get());
		if (mpLevel != nullptr)
		{
			NetClientHandler *connection = mpLevel->getConnection();
			if (connection != nullptr)
			{
				// Beta: this.minecraft.getConnection().send(new SignUpdatePacket(...)) (TextEditScreen.java:35)
				Packet130UpdateSign *packet = new Packet130UpdateSign();
				packet->xPosition = sign->x;
				packet->yPosition = sign->y;
				packet->zPosition = sign->z;
				for (int i = 0; i < 4; i++)
				{
					packet->signLines[i] = sign->messages[i];
				}
				connection->addToSendQueue(packet);
			}
		}
	}
}

// Beta: TextEditScreen.tick() (TextEditScreen.java:39-42)
void TextEditScreen::tick()
{
	frame++;
}

// Beta: TextEditScreen.buttonClicked() (TextEditScreen.java:44-52)
void TextEditScreen::buttonClicked(Button &button)
{
	if (button.active)
	{
		if (button.id == 0)
		{
			// Beta: this.sign.setChanged() (TextEditScreen.java:48)
			sign->setChanged();
			// Beta: this.minecraft.setScreen(null) (TextEditScreen.java:49)
			minecraft.setScreen(nullptr);
		}
	}
}

// Beta: TextEditScreen.keyPressed() (TextEditScreen.java:54-71)
void TextEditScreen::keyPressed(char_t eventCharacter, int_t eventKey)
{
	// Beta: if (eventKey == 200) { this.line = this.line - 1 & 3; } (TextEditScreen.java:56-58)
	if (eventKey == 200)  // Up arrow
	{
		line = (line - 1) & 3;
	}
	
	// Beta: if (eventKey == 208 || eventKey == 28) { this.line = this.line + 1 & 3; } (TextEditScreen.java:60-62)
	if (eventKey == 208 || eventKey == 28)  // Down arrow or Enter
	{
		line = (line + 1) & 3;
	}
	
	// Beta: if (eventKey == 14 && this.sign.messages[this.line].length() > 0) { ... } (TextEditScreen.java:64-66)
	if (eventKey == 14 && sign->messages[line].length() > 0)  // Backspace
	{
		sign->messages[line] = sign->messages[line].substr(0, sign->messages[line].length() - 1);
		// Invalidate texture cache when text changes (so it updates in real-time while editing)
		sign->invalidateWidthCache();
		sign->invalidateTextDisplayList();
	}
	
	// Beta: if (allowedChars.indexOf(ch) >= 0 && this.sign.messages[this.line].length() < 15) { ... } (TextEditScreen.java:68-70)
	jstring allowedChars = SharedConstants::acceptableLetters;
	if (allowedChars.find(static_cast<char16_t>(eventCharacter)) != jstring::npos && sign->messages[line].length() < 15)
	{
		sign->messages[line] += static_cast<char16_t>(eventCharacter);
		// Invalidate texture cache when text changes (so it updates in real-time while editing)
		sign->invalidateWidthCache();
		sign->invalidateTextDisplayList();
	}
}

// Beta: TextEditScreen.render() (TextEditScreen.java:73-114)
void TextEditScreen::render(int_t xm, int_t ym, float a)
{
	renderBackground();
	drawCenteredString(font, title, width / 2, 40, 16777215);
	
	glPushMatrix();
	glTranslatef(static_cast<float>(width) / 2.0f, static_cast<float>(height) / 2.0f, 50.0f);
	float ss = 93.75f;
	glScalef(-ss, -ss, -ss);
	glRotatef(180.0f, 0.0f, 1.0f, 0.0f);
	
	// Beta: Tile tile = this.sign.getTile() (TextEditScreen.java:82)
	Tile &tile = sign->getTile();
	if (&tile == &Tile::sign)
	{
		// Beta: float rot = this.sign.getData() * 360 / 16.0F (TextEditScreen.java:84)
		float rot = sign->getData() * 360.0f / 16.0f;
		glRotatef(rot, 0.0f, 1.0f, 0.0f);
		glTranslatef(0.0f, 0.3125f, 0.0f);
	}
	else
	{
		// Beta: int face = this.sign.getData() (TextEditScreen.java:88)
		int_t face = sign->getData();
		float rot = 0.0f;
		if (face == 2)  // North
		{
			rot = 180.0f;
		}
		if (face == 4)  // West
		{
			rot = 90.0f;
		}
		if (face == 5)  // East
		{
			rot = -90.0f;
		}
		glRotatef(rot, 0.0f, 1.0f, 0.0f);
		glTranslatef(0.0f, 0.3125f, 0.0f);
	}
	
	// Beta: if (this.frame / 6 % 2 == 0) { this.sign.selectedLine = this.line; } (TextEditScreen.java:106-108)
	if ((frame / 6) % 2 == 0)
	{
		sign->selectedLine = line;
	}
	
	// Beta: TileEntityRenderDispatcher.instance.render(this.sign, -0.5, -0.75, -0.5, 0.0F) (TextEditScreen.java:110)
	TileEntityRenderDispatcher::instance.render(sign.get(), -0.5, -0.75, -0.5, 0.0f);
	sign->selectedLine = -1;
	
	glPopMatrix();
	Screen::render(xm, ym, a);
}
