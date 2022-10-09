/* MIT License
 *
 *  Copyright (c) 2022 edgecraft.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  SOFTWARE.
 */

#include <iostream>
#include <fstream>
#include <algorithm>
#include <string>

#include <chrono>
#include <mutex>
#include <thread>
#include <signal.h>

#if defined(USE_EXPERIMENTAL_FS)
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#if defined(__APPLE__)
#include <unistd.h>
#endif
#endif

#include <srt.h>
#include "VirtualGamepad.h"

auto Usleep = [](uint64_t t) -> void {
	std::this_thread::sleep_for(std::chrono::microseconds(t));
};

static bool signal_recieved = false;

static void sig_handler(int signo)
{
	if( signo == SIGINT )
	{
		LogVerbose("received SIGINT\n");
		signal_recieved = true;
	}
}

std::tuple<std::string, std::string> get_name_service(const char *str)
{
	std::string name = "";
	std::string service = "";

	std::string s(str);
	auto itr = std::find(s.begin(), s.end(), ':');
	if (itr != s.end()) {
		name = { s.begin(), itr };
		service = { itr + 1, s.end() };
	}

	return { name, service };
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		LogInfo("usage: vgmpad_recv [host_name]:port\n");
		exit(EXIT_FAILURE);
	}

	auto [ name, service ] = get_name_service(argv[1]);

	if (SDL_Init(SDL_INIT_GAMECONTROLLER | SDL_INIT_VIDEO) != 0) {
		LogError("Unable to initialize SDL: %s", SDL_GetError());
		return EXIT_FAILURE;
	}
	atexit(SDL_Quit);

	if (srt_startup() < 0) {
		LogError("Unable to initialize SRT: %s", srt_getlasterror_str());
		return EXIT_FAILURE;
	}

	/*
	 * attach signal handler
	 */
	if( signal(SIGINT, sig_handler) == SIG_ERR )
		LogError("can't catch SIGINT\n");

	std::unique_ptr<GamepadDevice> gamepad;

	// create gamepad.
	gamepad = GamepadDevice::Create();

	VirtualGampad vgmpad;
	njson js;
	to_json(js, vgmpad);
	from_json(js, vgmpad);
	if (!vgmpad.open(name, service, VirtualGampad::em_Mode::SEND)) {
		LogError("ERROR!! open virtual gamepad.\n");
		return EXIT_FAILURE;
	}
	while (!signal_recieved) {
		uint32_t timeout = 1000;	// dummy.
		gamepad->Poll(timeout);

		if (gamepad->IsAttached()) {
			vgmpad.update(gamepad);
		}

		// std::cout << vgmpad << std::endl;
		to_json(js, vgmpad);
		std::cout << std::setw(4) << js << std::endl;

		vgmpad.send(33);

		auto fps = 10.0;
		Usleep((1.0 / fps) * 1'000'000.0);
	}

	if (srt_cleanup() != 0) {
		LogError("Unable to cleanup SRT: %s", srt_getlasterror_str());
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}