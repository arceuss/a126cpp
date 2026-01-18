#include "network/ThreadConnectToServer.h"
#include "client/gui/GuiConnecting.h"
#include "client/gui/GuiConnectFailed.h"
#include "client/Minecraft.h"
#include "network/NetClientHandler.h"
#include "network/Packet2Handshake.h"
#include <stdexcept>
#include <iostream>

ThreadConnectToServer::ThreadConnectToServer(GuiConnecting* gui, Minecraft* mc, const jstring& hostName, int_t port)
	: connectingGui(gui)
	, mc(mc)
	, hostName(hostName)
	, port(port)
{
}

void ThreadConnectToServer::run()
{
	try
	{
		// Java: GuiConnecting.setNetClientHandler(this.connectingGui, new NetClientHandler(this.mc, this.hostName, this.port));
		NetClientHandler* handler = new NetClientHandler(mc, hostName, port);
		GuiConnecting::setNetClientHandler(connectingGui, handler);
		
		// Java: if(GuiConnecting.isCancelled(this.connectingGui)) {
		//           return;
		//       }
		if (GuiConnecting::isCancelled(connectingGui))
		{
			return;
		}
		
		// Java: GuiConnecting.getNetClientHandler(this.connectingGui).addToSendQueue(new Packet2Handshake(this.mc.field_6320_i.inventory));
		// Note: In Alpha 1.2.6 Packet2Handshake takes username string
		// Get username from mc->options.username, or use "-" as default (offline mode)
		jstring username = mc ? mc->options.username : u"-";
		GuiConnecting::getNetClientHandler(connectingGui)->addToSendQueue(new Packet2Handshake(username));
	}
	catch (const std::runtime_error& e)
	{
		// Java: } catch (UnknownHostException var2) {
		//           if(GuiConnecting.isCancelled(this.connectingGui)) {
		//               return;
		//           }
		//           this.mc.displayGuiScreen(new GuiConnectFailed("Failed to connect to the server", "Unknown host \'" + this.hostName + "\'"));
		//       } catch (ConnectException var3) {
		//           if(GuiConnecting.isCancelled(this.connectingGui)) {
		//               return;
		//           }
		//           this.mc.displayGuiScreen(new GuiConnectFailed("Failed to connect to the server", var3.getMessage()));
		//       } catch (Exception var4) {
		
		// C++ doesn't have separate exception types for network errors, so we catch runtime_error
		// and check the error message or error code to determine the type
		if (GuiConnecting::isCancelled(connectingGui))
		{
			return;
		}
		
		std::string errorMsg = e.what();
		jstring errorDetail;
		
		// Try to distinguish between UnknownHost and ConnectException based on error message
		if (errorMsg.find("resolve") != std::string::npos || errorMsg.find("Unknown host") != std::string::npos)
		{
			// UnknownHostException equivalent
			std::string detailStr = "Unknown host '" + String::toUTF8(hostName) + "'";
			errorDetail = String::fromUTF8(detailStr);
		}
		else
		{
			// ConnectException or general Exception
			errorDetail = String::fromUTF8(errorMsg);
		}
		
		// Java: this.mc.displayGuiScreen(new GuiConnectFailed("Failed to connect to the server", var4.toString()));
		mc->setScreen(std::make_shared<GuiConnectFailed>(*mc, u"Failed to connect to the server", errorDetail));
	}
	catch (const std::exception& e)
	{
		// Java: } catch (Exception var4) {
		//           if(GuiConnecting.isCancelled(this.connectingGui)) {
		//               return;
		//           }
		//           var4.printStackTrace();
		//           this.mc.displayGuiScreen(new GuiConnectFailed("Failed to connect to the server", var4.toString()));
		//       }
		if (GuiConnecting::isCancelled(connectingGui))
		{
			return;
		}
		
		// Java: var4.printStackTrace();
		std::cerr << "ThreadConnectToServer exception: " << e.what() << std::endl;
		
		// Java: this.mc.displayGuiScreen(new GuiConnectFailed("Failed to connect to the server", var4.toString()));
		jstring errorDetail = String::fromUTF8(e.what());
		mc->setScreen(std::make_shared<GuiConnectFailed>(*mc, u"Failed to connect to the server", errorDetail));
	}
	catch (...)
	{
		// Catch any other exceptions
		if (GuiConnecting::isCancelled(connectingGui))
		{
			return;
		}
		
		std::cerr << "ThreadConnectToServer unknown exception" << std::endl;
		jstring errorDetail = u"Unknown error";
		mc->setScreen(std::make_shared<GuiConnectFailed>(*mc, u"Failed to connect to the server", errorDetail));
	}
}
