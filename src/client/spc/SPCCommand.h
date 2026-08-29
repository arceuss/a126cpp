#pragma once

#include <map>
#include <vector>

#include "java/String.h"
#include "java/Type.h"

class Level;
class Player;

class SPCCommand
{
public:
	struct Result
	{
		bool handled = false;
		bool success = false;
		std::vector<jstring> messages;
	};

	// Executes one slash command against the real local Level and Player.
	// Non-command text returns handled=false. Online levels reject commands.
	static Result execute(Level &level, Player &player, const jstring &input);

	// The optional local command tool needs chat access whenever a player is
	// loaded; this is not an Alpha gameplay behavior.
	static bool shouldOpenChat(bool playerLoaded, int_t eventKey, int_t chatKey);

	// Test/world-session cleanup for process-global waypoint and repeat state.
	static void resetState();

private:
	struct Waypoint
	{
		double x;
		double y;
		double z;
	};

	static std::map<jstring, Waypoint> waypoints;
	static double previousX;
	static double previousY;
	static double previousZ;
	static bool hasPreviousPosition;
	static jstring lastCommand;
};
