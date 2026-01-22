#include "client/gui/DeathScreen.h"

#include "client/Minecraft.h"
#include "client/title/TitleScreen.h"
#include "client/gui/Button.h"
#include "client/gui/ScreenSizeCalculator.h"

#include "pc/OpenGL.h"
#include "java/String.h"

DeathScreen::DeathScreen(Minecraft &minecraft) : Screen(minecraft)
{
}

void DeathScreen::init()
{
	// Beta: DeathScreen.init() - creates buttons (DeathScreen.java:8-14)
	buttons.clear();
	buttons.push_back(Util::make_shared<Button>(1, width / 2 - 100, height / 4 + 72, u"Respawn"));  // Beta: Button(1, this.width / 2 - 100, this.height / 4 + 72, "Respawn") (DeathScreen.java:10)
	buttons.push_back(Util::make_shared<Button>(2, width / 2 - 100, height / 4 + 96, u"Title menu"));  // Beta: Button(2, this.width / 2 - 100, this.height / 4 + 96, "Title menu") (DeathScreen.java:11)
	
	// Beta: Disable respawn button if user is null (DeathScreen.java:12-14)
	// TODO: Check if user exists
	// if (minecraft.user == null) {
	//     buttons.get(1).active = false;
	// }
}

void DeathScreen::buttonClicked(Button &button)
{
	// Beta: DeathScreen.buttonClicked(Button button) - handles button clicks (DeathScreen.java:22-32)
	if (button.id == 1)  // Beta: if (button.id == 1) (DeathScreen.java:23)
	{
		minecraft.player->respawn();  // Beta: this.minecraft.player.respawn() (DeathScreen.java:24)
		minecraft.setScreen(nullptr);  // Beta: this.minecraft.setScreen(null) (DeathScreen.java:25)
	}
	
	if (button.id == 2)  // Beta: if (button.id == 2) (DeathScreen.java:28)
	{
		minecraft.setLevel(nullptr);  // Beta: this.minecraft.setLevel(null) (DeathScreen.java:29)
		minecraft.setScreen(Util::make_shared<TitleScreen>(minecraft));  // Beta: this.minecraft.setScreen(new TitleScreen()) (DeathScreen.java:30)
	}
}

void DeathScreen::render(int_t xm, int_t ym, float a)
{
	// Beta: DeathScreen.render(int xm, int ym, float a) - renders death screen (DeathScreen.java:35-43)
	fillGradient(0, 0, width, height, 0x60500000, 0xA0803030);  // Beta: this.fillGradient(0, 0, this.width, this.height, 1615855616, -1602211792) - converted Java integers to unsigned hex
	
	glPushMatrix();  // Beta: GL11.glPushMatrix() (DeathScreen.java:37)
	glScalef(2.0f, 2.0f, 2.0f);  // Beta: GL11.glScalef(2.0F, 2.0F, 2.0F) (DeathScreen.java:38)
	drawCenteredString(font, u"Game over!", width / 2 / 2, 30, 0xFFFFFF);  // Beta: this.drawCenteredString(this.font, "Game over!", this.width / 2 / 2, 30, 16777215) (DeathScreen.java:39)
	glPopMatrix();  // Beta: GL11.glPopMatrix() (DeathScreen.java:40)
	
	jstring scoreText = u"Score: &e" + String::toString(minecraft.player->getScore());  // Beta: "Score: &e" + this.minecraft.player.getScore() (DeathScreen.java:41)
	drawCenteredString(font, scoreText, width / 2, 100, 0xFFFFFF);  // Beta: this.drawCenteredString(this.font, scoreText, this.width / 2, 100, 16777215) (DeathScreen.java:41)
	
	Screen::render(xm, ym, a);  // Beta: super.render(xm, ym, a) (DeathScreen.java:42)
}

bool DeathScreen::isPauseScreen()
{
	// Beta: DeathScreen.isPauseScreen() - returns false (DeathScreen.java:46-48)
	return false;  // Beta: return false (DeathScreen.java:47)
}
