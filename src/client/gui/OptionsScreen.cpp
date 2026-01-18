#include "client/gui/OptionsScreen.h"

#include "client/Minecraft.h"
#include "client/Options.h"
#include "client/gui/ScreenSizeCalculator.h"

#include "client/gui/ControlsScreen.h"
#include "client/gui/ClientOptionsScreen.h"

#include "client/gui/SmallButton.h"
#include "client/gui/SlideButton.h"

OptionsScreen::OptionsScreen(Minecraft &minecraft, std::shared_ptr<Screen> lastScreen, Options &options) : Screen(minecraft), options(options)
{
	this->lastScreen = lastScreen;
}


void OptionsScreen::init()
{
	title = u"Options";

	for (int_t i = 0; i < Util::size(Options::Option::values); i++)
	{
		auto &item = *Options::Option::values[i];
		if (!item.isProgress)
			buttons.push_back(Util::make_shared<SmallButton>(i, width / 2 - 155 + i % 2 * 160, height / 6 + 24 * (i >> 1), &item, options.getMessage(item)));
		else
			buttons.push_back(Util::make_shared<SlideButton>(i, width / 2 - 155 + i % 2 * 160, height / 6 + 24 * (i >> 1), &item, options.getMessage(item), options.getProgressValue(item)));
	}

	buttons.push_back(Util::make_shared<Button>(100, width / 2 - 100, height / 6 + 144, u"Controls..."));
	buttons.push_back(Util::make_shared<Button>(101, width / 2 - 100, height / 6 + 168, u"Client Options..."));
	buttons.push_back(Util::make_shared<Button>(200, width / 2 - 100, height / 6 + 192, u"Done"));
}

void OptionsScreen::buttonClicked(Button &button)
{
	if (!button.active)
		return;

	if (button.id < 100 && button.isSmallButton())
	{
		auto &smallButton = reinterpret_cast<SmallButton &>(button);
		Options::Option::Element *option = smallButton.getOption();
		
		// Check if this is GUI_SCALE before doing anything that might invalidate the reference
		bool isGuiScale = (option == &Options::Option::GUI_SCALE);
		
		options.toggle(*option, 1);
		
		if (isGuiScale)
		{
			// Re-layout screen if GUI scale changed (like Java GuiOptions does for non-sliders)
			// Do this BEFORE accessing button again, since init() will clear buttons
			ScreenSizeCalculator ssc(minecraft.width, minecraft.height, minecraft.options);
			int_t w = ssc.getWidth();
			int_t h = ssc.getHeight();
			Screen::init(w, h);
			init(); // Reinitialize buttons with new size - button reference is now invalid
		}
		else
		{
			// For other options, just update the button message (button is still valid)
			smallButton.msg = options.getMessage(*option);
		}
	}

	if (button.id == 100)
	{
		minecraft.options.save();
		minecraft.setScreen(Util::make_shared<ControlsScreen>(minecraft, minecraft.screen, options));
	}
	if (button.id == 101)
	{
		minecraft.options.save();
		minecraft.setScreen(Util::make_shared<ClientOptionsScreen>(minecraft, minecraft.screen, options));
	}
	if (button.id == 200)
	{
		minecraft.options.save();
		minecraft.setScreen(lastScreen);
	}
}

void OptionsScreen::render(int_t xm, int_t ym, float a)
{
	renderBackground();
	drawCenteredString(font, title, width / 2, 20, 0xFFFFFF);

	Screen::render(xm, ym, a);
}
