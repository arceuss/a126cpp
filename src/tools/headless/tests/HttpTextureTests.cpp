#include "network/SocketStreams.h"
#ifndef _WIN32
#include <netinet/in.h>
#endif
#include <chrono>
#include <future>
#include <thread>
#include "client/renderer/HttpTexture.h"
#include "java/Resource.h"
#include "tools/headless/TestFramework.h"
#include "util/BackgroundTask.h"

static void closeTestSocket(SocketHandle socket)
{
#ifdef _WIN32
	closesocket(socket);
#else
	close(socket);
#endif
}

HEADLESS_TEST(http_texture, removal_during_decode_cannot_publish_into_replacement)
{
#ifdef _WIN32
	WSADATA winsock;
	if (!ctx.check(WSAStartup(MAKEWORD(2, 2), &winsock) == 0, "initialize loopback sockets"))
		return;
#endif
	std::unique_ptr<std::istream> imageFile(Resource::getResource(u"/font/default.png"));
	const std::string png((std::istreambuf_iterator<char>(*imageFile)), std::istreambuf_iterator<char>());
	const std::string response = "HTTP/1.1 200 OK\r\nContent-Length: " + std::to_string(png.size()) +
		"\r\nConnection: close\r\n\r\n" + png;
	SocketHandle listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	sockaddr_in address = {};
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
#ifdef _WIN32
	int addressSize = sizeof(address);
#else
	socklen_t addressSize = sizeof(address);
#endif
	if (!ctx.check(listener != INVALID_SOCKET_HANDLE &&
		bind(listener, reinterpret_cast<sockaddr *>(&address), sizeof(address)) == 0 &&
		listen(listener, 2) == 0 &&
		getsockname(listener, reinterpret_cast<sockaddr *>(&address), &addressSize) == 0,
		"bind ephemeral loopback HTTP server"))
	{
		closeTestSocket(listener);
#ifdef _WIN32
		WSACleanup();
#endif
		return;
	}
	std::thread server([&]() {
		for (int i = 0; i < 2; ++i)
		{
			fd_set ready;
			FD_ZERO(&ready);
			FD_SET(listener, &ready);
			timeval timeout = {5, 0};
			if (select(static_cast<int>(listener + 1), &ready, nullptr, nullptr, &timeout) <= 0)
				return;
			SocketHandle client = accept(listener, nullptr, nullptr);
			if (client == INVALID_SOCKET_HANDLE)
				return;
			std::string request;
			while (request.find("\r\n\r\n") == std::string::npos && request.size() < 8192)
			{
				FD_ZERO(&ready);
				FD_SET(client, &ready);
				timeout = {5, 0};
				if (select(static_cast<int>(client + 1), &ready, nullptr, nullptr, &timeout) <= 0)
					break;
				char buffer[1024];
				const int count = recv(client, buffer, sizeof(buffer), 0);
				if (count <= 0)
					break;
				request.append(buffer, count);
			}
			std::size_t sent = 0;
			while (sent < response.size())
			{
				const int count = send(client, response.data() + sent, static_cast<int>(response.size() - sent), 0);
				if (count <= 0)
					break;
				sent += count;
			}
			closeTestSocket(client);
		}
	});

	struct State
	{
		std::promise<void> entered;
		std::promise<void> release;
		std::atomic<bool> destroyed{false};
	};
	class Processor : public HttpTextureProcessor
	{
	public:
		std::shared_ptr<State> state;
		explicit Processor(const std::shared_ptr<State> &state) : state(state) {}
		~Processor() override { state->destroyed.store(true); }
		BufferedImage process(BufferedImage &input) override
		{
			state->entered.set_value();
			state->release.get_future().wait();
			return std::move(input);
		}
	};
	const jstring url = u"http://127.0.0.1:" + String::toString(ntohs(address.sin_port)) + u"/test.png";
	const std::shared_ptr<State> state = std::make_shared<State>();
	std::unique_ptr<HttpTexture> old = std::make_unique<HttpTexture>(url, std::make_unique<Processor>(state));
	const bool entered = state->entered.get_future().wait_for(std::chrono::seconds(5)) == std::future_status::ready;
	ctx.check(entered, "old request reaches its blocking decoder");
	ctx.check(old->image() == nullptr, "partially processed image is not published");
	old.reset();
	ctx.check(!state->destroyed.load(), "processor survives texture removal while still in use");
	HttpTexture replacement(url, nullptr);
	state->release.set_value();
	BackgroundTask::joinAll();
	server.join();
	closeTestSocket(listener);
	ctx.check(state->destroyed.load(), "late request releases its owned processor");
	BufferedImage *image = replacement.image();
	ctx.check(image != nullptr && image->getWidth() == 128 && image->getHeight() == 128,
		"replacement publishes its own fully decoded font image");
#ifdef _WIN32
	WSACleanup();
#endif
}
