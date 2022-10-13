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
#include <tuple>

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

#ifdef USE_SRT
#include <srt.h>
#endif
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

static void print_usage()
{
	LogInfo("usage: vgmpad_recv [host_name]:port[/protocol]\n");
	LogInfo("  protocol: srt, udp. If not specified, it is 'srt'.\n");
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		print_usage();
		exit(EXIT_FAILURE);
	}

	auto [ name, service, protocol ] = VirtualGamepad::get_name_service(argv[1]);
	if (name == "" && service == "" && protocol == "") {
		print_usage();
		exit(EXIT_FAILURE);
	}

	if (SDL_Init(SDL_INIT_GAMECONTROLLER | SDL_INIT_VIDEO) != 0) {
		LogError("Unable to initialize SDL: %s\n", SDL_GetError());
		return EXIT_FAILURE;
	}
	atexit(SDL_Quit);

#ifdef USE_SRT
	if (srt_startup() < 0) {
		LogError("Unable to initialize SRT: %s\n", srt_getlasterror_str());
		return EXIT_FAILURE;
	}
#endif

	/*
	 * attach signal handler
	 */
	if( signal(SIGINT, sig_handler) == SIG_ERR )
		LogError("can't catch SIGINT\n");

	std::unique_ptr<VirtualGamepad> vgmpad;
	if (protocol == "udp") {
		vgmpad = VirtualGamepadUDP::Create(name, service, VirtualGamepad::em_Mode::RECEIVE);
	} else {
		vgmpad = VirtualGamepadSRT::Create(name, service, VirtualGamepad::em_Mode::RECEIVE);
	}
	if (!vgmpad) {
		LogError("ERROR!! create virtual gamepad.\n");
		return EXIT_FAILURE;
	}
    njson js;
	while (!signal_recieved) {
		int64_t time_out = 33; // [msec].
		vgmpad->Poll(time_out);

		// std::cout << vgmpad << std::endl;
		to_json(js, *vgmpad);
		std::cout << std::setw(2) << js << std::endl;

		auto fps = 30.0;
		Usleep((1.0 / fps) * 1'000'000.0);
	}

#ifdef USE_SRT
	if (srt_cleanup() != 0) {
		LogError("Unable to cleanup SRT: %s\n", srt_getlasterror_str());
		return EXIT_FAILURE;
	}
#endif

	return EXIT_SUCCESS;
}
