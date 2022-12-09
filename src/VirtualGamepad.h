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
#include <sstream>
#include <algorithm>
#include <string>
#include <vector>
#include <list>
#include <deque>
#include <tuple>

#include <chrono>
#include <mutex>
#include <thread>

#ifdef USE_SRT
#include <srt.h>
#else
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#endif

#include "devGamepad.h"

#include "json.hpp"
using njson = nlohmann::json;



// base class for derived VirtualGamepad### class.
class VirtualGamepad
{
public:
	enum class em_Mode : int {
		SEND,
		RECEIVE,
		NONE,
	};

private:

protected:
	em_Mode m_mode = em_Mode::NONE;
	std::string m_name;
	std::string m_service;
	addrinfo *m_ai = nullptr;
	bool m_connected = false;

	njson m_js;
	// std::mutex m_mtx;

	// axis, button status.
	bool axis_motion = false;
	bool button_down = false;
	bool button_up = false;

	// axis.
	int16_t axis_Left_X = 0;
	int16_t axis_Left_Y = 0;
	int16_t axis_Right_X = 0;
	int16_t axis_Right_Y = 0;
	int16_t axis_Trigger_L = 0;
	int16_t axis_Trigger_R = 0;

	// button.
	uint8_t button_A = 0;
	uint8_t button_B = 0;
	uint8_t button_X = 0;
	uint8_t button_Y = 0;
	uint8_t button_Back = 0;
	uint8_t button_Guide = 0;
	uint8_t button_Start = 0;
	uint8_t button_Stick_L = 0;
	uint8_t button_Stick_R = 0;
	uint8_t button_Shoulder_L = 0;
	uint8_t button_Shoulder_R = 0;
	uint8_t button_Dpad_U = 0;
	uint8_t button_Dpad_D = 0;
	uint8_t button_Dpad_L = 0;
	uint8_t button_Dpad_R = 0;

	void clear_axis_button_status()
	{
		if (!m_js.empty()) {
			m_js["axis_motion"] = false;
			m_js["button_down"] = false;
			m_js["button_up"] = false;
		}
	}

public:
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(
		VirtualGamepad,

		axis_motion,
		button_down,
		button_up,

		axis_Left_X,
		axis_Left_Y,
		axis_Right_X,
		axis_Right_Y,
		axis_Trigger_L,
		axis_Trigger_R,

		button_A,
		button_B,
		button_X,
		button_Y,
		button_Back,
		button_Guide,
		button_Start,
		button_Stick_L,
		button_Stick_R,
		button_Shoulder_L,
		button_Shoulder_R,
		button_Dpad_U,
		button_Dpad_D,
		button_Dpad_L,
		button_Dpad_R
	)

	static void Create(const std::string &name, const std::string &service, em_Mode mode)
	{
		/* NEED IMPLEMENTATION in sub-class. */
	}

	static std::tuple<std::string, std::string, std::string> get_name_service(const char *str)
	{
		std::string name = "";
		std::string service = "";
		std::string protocol = "";

		std::string s(str);
		auto itr_s = std::find(s.begin(), s.end(), ':');
		auto itr_p = std::find(s.begin(), s.end(), '/');

		if (itr_s >= itr_p) {
			return { std::string{""}, std::string{""}, std::string{""} };
		}

		if (itr_s != s.end()) {
			name = { s.begin(), itr_s };
			service = { itr_s + 1, itr_p };
		}

		if (itr_p != s.end()) {
			protocol = { itr_p + 1, s.end() };
		}

		std::transform(protocol.cbegin(), protocol.cend(), protocol.begin(), ::tolower);
		if (protocol == "") protocol = "srt";
		if (protocol != "udp" && protocol != "srt") {
			return { std::string{""}, std::string{""}, std::string{""} };
		}

		return { name, service, protocol };
	}

	// Is Gamepad Attached.
	virtual bool IsAttached() const = 0;

	virtual bool Poll( uint32_t timeout=0 ) = 0;

	bool IsAxisMotion() const { return axis_motion; }
	bool IsButtonDown() const { return button_down; }
	bool IsButtonUp() const { return button_up; }

	int16_t GetAxis_Left_X() const { return axis_Left_X; }
	int16_t GetAxis_Left_Y() const { return axis_Left_Y; }
	int16_t GetAxis_Right_X() const { return axis_Right_X; }
	int16_t GetAxis_Right_Y() const { return axis_Right_Y; }
	int16_t GetAxis_Trigger_L() const { return axis_Trigger_L; }
	int16_t GetAxis_Trigger_R() const { return axis_Trigger_R; }

	uint8_t GetButton_A() const { return button_A; }
	uint8_t GetButton_B() const { return button_B; }
	uint8_t GetButton_X() const { return button_X; }
	uint8_t GetButton_Y() const { return button_Y; }
	uint8_t GetButton_Back() const { return button_Back; }
	uint8_t GetButton_Guide() const { return button_Guide; }
	uint8_t GetButton_Start() const { return button_Start; }
	uint8_t GetButton_Stick_L() const { return button_Stick_L; }
	uint8_t GetButton_Stick_R() const { return button_Stick_R; }
	uint8_t GetButton_Shoulder_L() const { return button_Shoulder_L; }
	uint8_t GetButton_Shoulder_R() const { return button_Shoulder_R; }
	uint8_t GetButton_Dpad_U() const { return button_Dpad_U; }
	uint8_t GetButton_Dpad_D() const { return button_Dpad_D; }
	uint8_t GetButton_Dpad_L() const { return button_Dpad_L; }
	uint8_t GetButton_Dpad_R() const { return button_Dpad_R; }

	VirtualGamepad() { clear_stat(); }

	virtual ~VirtualGamepad() {}

	void clear_stat()
	{
		axis_motion = false;
		button_down = false;
		button_up = false;

		// axis.
		int16_t axis_Left_X = 0;
		int16_t axis_Left_Y = 0;
		int16_t axis_Right_X = 0;
		int16_t axis_Right_Y = 0;
		int16_t axis_Trigger_L = 0;
		int16_t axis_Trigger_R = 0;

		// button.
		uint8_t button_A = 0;
		uint8_t button_B = 0;
		uint8_t button_X = 0;
		uint8_t button_Y = 0;
		uint8_t button_Back = 0;
		uint8_t button_Guide = 0;
		uint8_t button_Start = 0;
		uint8_t button_Stick_L = 0;
		uint8_t button_Stick_R = 0;
		uint8_t button_Shoulder_L = 0;
		uint8_t button_Shoulder_R = 0;
		uint8_t button_Dpad_U = 0;
		uint8_t button_Dpad_D = 0;
		uint8_t button_Dpad_L = 0;
		uint8_t button_Dpad_R = 0;
	}

	bool update(std::unique_ptr<GamepadDevice> &gamepad)
	{
		if (!gamepad->IsAttached()) {
			// LogInfo("Gamepad is not enable.\n");
			return false;
		}

		axis_motion = gamepad->IsAxisMotion();
		button_down = gamepad->IsButtonDown();
		button_up = gamepad->IsButtonUp();

		axis_Left_X = gamepad->GetAxis_Left_X();
		axis_Left_Y = gamepad->GetAxis_Left_Y();
		axis_Right_X = gamepad->GetAxis_Right_X();
		axis_Right_Y = gamepad->GetAxis_Right_Y();
		axis_Trigger_L = gamepad->GetAxis_Trigger_L();
		axis_Trigger_R = gamepad->GetAxis_Trigger_R();

		button_A = gamepad->GetButton_A();
		button_B = gamepad->GetButton_B();
		button_X = gamepad->GetButton_X();
		button_Y = gamepad->GetButton_Y();
		button_Back = gamepad->GetButton_Back();
		button_Guide = gamepad->GetButton_Guide();
		button_Start = gamepad->GetButton_Start();
		button_Stick_L = gamepad->GetButton_Stick_L();
		button_Stick_R = gamepad->GetButton_Stick_R();
		button_Shoulder_L = gamepad->GetButton_Shoulder_L();
		button_Shoulder_R = gamepad->GetButton_Shoulder_R();
		button_Dpad_U = gamepad->GetButton_Dpad_U();
		button_Dpad_D = gamepad->GetButton_Dpad_D();
		button_Dpad_L = gamepad->GetButton_Dpad_L();
		button_Dpad_R = gamepad->GetButton_Dpad_R();

		return true;
	}

	virtual bool open(const std::string &name, const std::string &service, em_Mode mode) = 0;
	virtual bool close() = 0;
	virtual bool poll(int64_t time_out = 33) = 0;

	bool send(int64_t time_out = 33)
	{
		to_json(m_js, *this);

		return poll(time_out);
	}

	bool receive(int64_t time_out = 33)
	{
		bool ret = poll(time_out);
		if (ret) {
			// LogDebug("poll() : true\n");
		} else {
			// LogDebug("poll() : false\n");
		}
		if (!m_js.empty()) from_json(m_js, *this);

		return ret;
	}

	friend std::ostream &operator<<(std::ostream &ostr, const VirtualGamepad &vg);
};

#ifdef USE_SRT
class VirtualGamepadSRT : public VirtualGamepad
{
private:
	SRTSOCKET m_sock = SRT_INVALID_SOCK;
	SRTSOCKET m_sock_listen = SRT_INVALID_SOCK;

	// epoll.
	int m_pollid = -1;
	int m_srtrfdslen = 2;
	int m_srtwfdslen = 2;
	SRTSOCKET m_srtrwfds[4] = {SRT_INVALID_SOCK, SRT_INVALID_SOCK , SRT_INVALID_SOCK , SRT_INVALID_SOCK };
	// int m_sysrfdslen = 2;
	// SYSSOCKET m_sysrfds[2];

protected:

public:
	static std::unique_ptr<VirtualGamepadSRT> Create(const std::string &name, const std::string &service, em_Mode mode)
	{
		auto vgmpad = std::make_unique<VirtualGamepadSRT>();
		auto ret = vgmpad->open(name, service, mode);

		LogInfo("======== Virtual Gamepad ========\n");

		if (!ret) {
			LogError("virtual gamepad -- can't opened\n");
			// vgmpad.reset();
		} else {
			std::string mode_str;
			if (mode == em_Mode::RECEIVE) {
				mode_str = "Receive";
			} else if (mode == em_Mode::SEND) {
				mode_str = "Send";
			} else {
				mode_str = "None";
			}
			LogSuccess("virtual gamepad -- opened(%s) %s:%s\n", mode_str.c_str(), name.c_str(), service.c_str());
		}

		return std::move(vgmpad);
	}

	// Is Gamepad Attached.
	bool IsAttached() const override { return (m_sock != SRT_INVALID_SOCK); }

	bool Poll( uint32_t timeout=0 ) override
	{
		return receive(timeout);
	}

	VirtualGamepadSRT()
	{
		m_pollid = srt_epoll_create();
		if (m_pollid < 0)
		{
			std::cerr << "Can't initialize epoll" << std::endl;
			exit(EXIT_FAILURE);
		}
		LogInfo("m_pollid = %d\n", m_pollid);
	}

	virtual ~VirtualGamepadSRT()
	{
		if (!close()) {
			LogError("ERROR!! VirtualGamepad close.\n");
			// exit(EXIT_FAILURE);
		}

		int ret = srt_epoll_release(m_pollid);
		if (ret < 0)
		{
			std::cerr << "Can't release epoll" << " : " << m_pollid << std::endl;
			// exit(EXIT_FAILURE);
		}
	}

	bool open(const std::string &name, const std::string &service, em_Mode mode) override
	{
		m_mode = mode;
		m_name = name;
		m_service = service;

		bool is_caller = (name != "") ? true : false;

		// get addrinfo.
		{
			addrinfo fo = {
				AI_PASSIVE,
				AF_UNSPEC,
				SOCK_DGRAM, IPPROTO_UDP,
				0, 0,
				NULL, NULL
			};
			const char *n = (name.empty() || name == "") ? nullptr : name.c_str();
			const char *s = (service.empty() || service == "") ? nullptr : service.c_str();
			int erc = getaddrinfo(n, s, &fo, &m_ai);
			if (erc != 0)
			{
				LogError("ERROR!! getaddrinfo(errno=%d): name=%s, service=%s.\n", erc, name.c_str(), service.c_str());
				srt_close(m_sock);
				return false;
			}
		}

		m_sock = srt_create_socket();
		if ( m_sock == SRT_ERROR ) {
			LogError("ERROR!! srt_create_socket\n");
			return false;
		}

		auto open_socket = [&]() -> bool {
			// pre config.
			bool no = false;
			int result;
			result = srt_setsockopt(m_sock, 0, SRTO_RCVSYN, &no, sizeof no);
			if ( result == -1 ) {
				LogError("Can't set SRT option : %s(%s)\n", "SRTO_RCVSYN", no ? "true" : "false");
				srt_close(m_sock);
				return false;
			}

			// caller.
			if (is_caller) {
				// connect to the receiver, implicit bind
				if (SRT_ERROR == srt_connect(m_sock, m_ai->ai_addr, m_ai->ai_addrlen))
				{
					LogError("ERROR!! srt_connect: %s\n", srt_getlasterror_str());
					srt_close(m_sock);
					return false;
				}

			// listener.
			} else {
				if (srt_bind(m_sock, m_ai->ai_addr, m_ai->ai_addrlen) == SRT_ERROR)
				{
					LogError("ERROR!! srt_bind: %s\n", srt_getlasterror_str());
					srt_close(m_sock);
					return false;
				}

				LogVerbose(" listen...\n");

				if (srt_listen(m_sock, 1) == SRT_ERROR)
				{
					LogError("ERROR!! srt_listen: %s\n", srt_getlasterror_str());
					srt_close(m_sock);
					return false;
				}
			}

			// post config.
			result = srt_setsockopt(m_sock, 0, SRTO_SNDSYN, &no, sizeof no);
			if ( result == -1 ) {
				LogError("Can't set SRT option : %s(%s)\n", "SRTO_SNDSYN", no ? "true" : "false");
				srt_close(m_sock);
				return false;
			}

			int events = SRT_EPOLL_IN | SRT_EPOLL_OUT | SRT_EPOLL_ERR;
			if (srt_epoll_add_usock(m_pollid, m_sock, &events))
			{
				LogError("Failed to add SRT destination to poll, %d\n", m_sock);
				srt_close(m_sock);
				return false;
			}

			return true;
		};

		if (mode == em_Mode::SEND) {
			if (!open_socket()) return false;
			else LogInfo("SUCCESS!! open virtual gamepad(SEND).\n");

		} else if (mode == em_Mode::RECEIVE) {
			if (!open_socket()) return false;
			else LogInfo("SUCCESS!! open virtual gamepad(RECEIVE).\n");
		}

		return true;
	}

	bool close() override
	{
		bool ret = false;

		if (!m_ai) {
			freeaddrinfo(m_ai);
			m_ai = nullptr;
		}

		if (m_sock != SRT_INVALID_SOCK) {
			int r = srt_close(m_sock);
			m_sock = SRT_INVALID_SOCK;
			if (r != SRT_ERROR) ret = true;
		}

		return ret;
	}

	bool poll(int64_t time_out = 33) override
	{
		if (m_sock == SRT_INVALID_SOCK)
		{
			if (!open(m_name, m_service, m_mode)) return false;
		}

		const auto str_direction = (m_mode == em_Mode::RECEIVE) ? "source" : "target";

		m_srtrfdslen = 2;
		m_srtwfdslen = 2;
		for (auto &e : m_srtrwfds) e = SRT_INVALID_SOCK;
		// m_sysrfdslen = 2;
		// for (auto &e : m_sysrfds) e = SRT_INVALID_SOCK;
		auto ret_epoll_wait = srt_epoll_wait(m_pollid,
			&m_srtrwfds[0], &m_srtrfdslen, &m_srtrwfds[2], &m_srtwfdslen,
			time_out,
			0, 0, 0, 0);
			// &m_sysrfds[0], &m_sysrfdslen, 0, 0);
		// LogDebug("srt_epoll_wait = %d (%s)\n", ret_epoll_wait, srt_getlasterror_str());
		if (ret_epoll_wait >= 0
			// || m_mode == em_Mode::SEND
			)
		{
			bool doabort = false;
			for (size_t i = 0; i < sizeof(m_srtrwfds) / sizeof(SRTSOCKET) && !doabort; i++)
			{
				SRTSOCKET s = m_srtrwfds[i];
				if (s == SRT_INVALID_SOCK)
					continue;

				// Remove duplicated sockets
				for (size_t j = i + 1; j < sizeof(m_srtrwfds) / sizeof(SRTSOCKET); j++)
				{
					const SRTSOCKET next_s = m_srtrwfds[j];
					if (next_s == s)
						m_srtrwfds[j] = SRT_INVALID_SOCK;
				}

				if (m_sock != s)
				{
					continue;
				}

				SRT_SOCKSTATUS status = srt_getsockstate(s);
				switch (status)
				{
				case SRTS_LISTENING:
					{
						LogVerbose(" accept... \n");

						m_sock_listen = m_sock;
						sockaddr sa = {};
						int sa_len = sizeof(sa);
						m_sock = srt_accept(m_sock_listen, &sa, &sa_len);

						// we do one client connection at a time,
						// so close the listener.
						srt_close(m_sock_listen);
						m_sock_listen = SRT_INVALID_SOCK;

						if ( m_sock == SRT_INVALID_SOCK )
						{
							LogError("Failed to accept SRT connection\n");
							doabort = true;
							break;
						}

						LogVerbose(" connected.\n");

						// pre config.
						bool no = false;
						int result;
						result = srt_setsockopt(m_sock, 0, SRTO_RCVSYN, &no, sizeof no);
						if ( result == -1 ) {
							LogError("Can't set SRT option : %s(%s)\n", "SRTO_RCVSYN", no ? "true" : "false");
							srt_close(m_sock);
							doabort = true;
							break;
						}

						// post config.
						result = srt_setsockopt(m_sock, 0, SRTO_SNDSYN, &no, sizeof no);
						if ( result == -1 ) {
							LogError("Can't set SRT option : %s(%s)\n", "SRTO_SNDSYN", no ? "true" : "false");
							srt_close(m_sock);
							doabort = true;
							break;
						}

						srt_epoll_remove_usock(m_pollid, s);	// remove listen event.

						int events = SRT_EPOLL_IN | SRT_EPOLL_OUT | SRT_EPOLL_ERR;
						if (srt_epoll_add_usock(m_pollid, m_sock, &events))
						{
							LogError("Failed to add SRT client to poll, %d\n", m_sock);
							doabort = true;
						}
						else
						{
							LogVerbose("Accepted SRT %s connection\n", str_direction);
							m_connected = true;
						}
					}
					break;
				case SRTS_BROKEN:
				case SRTS_NONEXIST:
				case SRTS_CLOSED:
					{
						if (m_connected)
						{
							LogInfo("SRT %s disconnected\n", str_direction);
							m_connected = false;
						}

						srt_epoll_remove_usock(m_pollid, s);
						close();
					}
					break;
				case SRTS_CONNECTED:
					{
						if (!m_connected)
						{
							LogInfo("SRT %s connected\n", str_direction);
							m_connected = true;

							// const int events = SRT_EPOLL_IN | SRT_EPOLL_ERR;
							// // Disable OUT event polling when connected
							// if (srt_epoll_update_usock(m_pollid, m_sock, &events))
							// {
							// 	LogError("Failed to add SRT destination to poll, %d\n", m_sock);
							// 	return false;
							// }
						}
					}
					break;

				default:
					{
						// No-Op
					}
					break;
				}
			}

			if (m_mode == em_Mode::RECEIVE) {
				std::list<std::vector<char>> dataqueue;
				if (m_sock != SRT_INVALID_SOCK && (m_srtrfdslen /* || m_sysrfdslen */))
				{
					std::vector<char> pkt(SRT_LIVE_MAX_PLSIZE);
					SRT_MSGCTRL ctrl;
					const int stat = srt_recvmsg2(m_sock, pkt.data(), (int)pkt.size(), &ctrl);
					// LogDebug("ctrl.srctime(recv) = %ld\n", ctrl.srctime);
					if (stat <= 0)
					{
						// LogInfo("Empty packets\n");
						clear_axis_button_status();
						return false;
					}
					if (stat < pkt.size()) pkt.resize(stat);
					dataqueue.push_back(pkt);

				} else {
					// LogInfo("Empty packets\n");
					clear_axis_button_status();
					return false;
				}

				// LogDebug("dataqueue size = %ld\n", dataqueue.size());
				while (!dataqueue.empty())
				{
					std::vector<char> pkt = dataqueue.front();
					// m_js = njson::from_cbor(pkt);
					std::string str = pkt.data();
					m_js = njson::parse(str);
					dataqueue.pop_front();
				}

			} else if (m_mode == em_Mode::SEND) {
				std::list<std::vector<char>> dataqueue;
				std::vector<char> pkt(SRT_LIVE_MAX_PLSIZE);
				std::stringstream ss;
				ss /* << std::setw(4) */ << m_js << std::endl;
				std::string ss_str = ss.str();
				// auto cbor = njson::to_cbor(m_js);
				// auto json_size = ss_str.size();
				// auto cbor_size = cbor.size();
				if (ss_str.size() + 1 < pkt.size()) {
					pkt.assign(ss_str.begin(), ss_str.end());
					pkt.resize(ss_str.size() + 1);
					pkt.back() = '\0';
					dataqueue.push_back(pkt);
				} else {
					LogError("ERROR!! pkt size is not enough.\n");
					return false;
				}

				while (!dataqueue.empty())
				{
					std::vector<char> pkt = dataqueue.front();
					SRT_MSGCTRL ctrl = srt_msgctrl_default;
					int stat = srt_sendmsg2(m_sock, pkt.data(), (int)pkt.size(), &ctrl);
					// LogDebug("ctrl.srctime(send) = %ld\n", ctrl.srctime);
					if (stat == SRT_ERROR)
					{
						LogError("ERROR!! send SRT packet.\n");
						return false;
					}
					dataqueue.pop_front();
				}
			}
		}

		return true;
	}
};
#else
class VirtualGamepadSRT : public VirtualGamepad
{
private:

protected:

public:
	static std::unique_ptr<VirtualGamepadSRT> Create(const std::string &name, const std::string &service, em_Mode mode)
	{
		auto vgmpad = std::make_unique<VirtualGamepadSRT>();
		vgmpad.reset();

		return std::move(vgmpad);
	}

	// Is Gamepad Attached.
	bool IsAttached() const override { return false; }

	bool Poll( uint32_t timeout=0 ) override { return false; }

	VirtualGamepadSRT()
	{
		LogError("This execution binary is not support SRT.\n");
		LogError("If you need SRT, re-build with 'USE_SRT' option enabled.\n");
	}

	virtual ~VirtualGamepadSRT() {}

	bool open(const std::string &name, const std::string &service, em_Mode mode) override { return false; }
	bool close() override { return false; }
	bool poll(int64_t time_out = 33) override { return false; }
};
#endif

class VirtualGamepadUDP : public VirtualGamepad
{
private:
	static constexpr auto WAIT_FOR_RECONNECT = 500;	// [msec].

	int m_sock = -1;

protected:

public:
	static std::unique_ptr<VirtualGamepadUDP> Create(const std::string &name, const std::string &service, em_Mode mode)
	{
		auto vgmpad = std::make_unique<VirtualGamepadUDP>();
		auto ret = vgmpad->open(name, service, mode);

		LogInfo("======== Virtual Gamepad ========\n");

		if (!ret) {
			LogError("virtual gamepad -- can't opened\n");
			// vgmpad.reset();
		} else {
			std::string mode_str;
			if (mode == em_Mode::RECEIVE) {
				mode_str = "Receive";
			} else if (mode == em_Mode::SEND) {
				mode_str = "Send";
			} else {
				mode_str = "None";
			}
			LogSuccess("virtual gamepad -- opened(%s) %s:%s\n", mode_str.c_str(), name.c_str(), service.c_str());
		}

		return std::move(vgmpad);
	}

	// Is Gamepad Attached.
	bool IsAttached() const override { return (m_sock >= 0); }

	bool Poll( uint32_t timeout=0 ) override
	{
		return receive(timeout);
	}

	VirtualGamepadUDP() {}

	virtual ~VirtualGamepadUDP()
	{
		if (!close()) {
			LogError("ERROR!! VirtualGamepad close.\n");
			// exit(EXIT_FAILURE);
		}
	}

	bool open(const std::string &name, const std::string &service, em_Mode mode) override
	{
		m_mode = mode;
		m_name = name;
		m_service = service;

		// get addrinfo.
		{
			addrinfo fo = {
				AI_PASSIVE,
				AF_UNSPEC,
				SOCK_DGRAM, IPPROTO_UDP,
				0, 0,
				NULL, NULL
			};
			const char *n = (name.empty() || name == "") ? nullptr : name.c_str();
			const char *s = (service.empty() || service == "") ? nullptr : service.c_str();
			int erc = getaddrinfo(n, s, &fo, &m_ai);
			if (erc != 0)
			{
				LogError("ERROR!! getaddrinfo(errno=%d): name=%s, service=%s.\n", erc, name.c_str(), service.c_str());
				return false;
			}
		}

		m_sock = socket(m_ai->ai_family, m_ai->ai_socktype, 0);
		if ( m_sock < 0 ) {
			LogError("ERROR!! create socket\n");
			return false;
		}

		if (mode == em_Mode::SEND) {
			// pre config.
			int yes = 1;
			setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof yes);
#if defined(_WIN32)
			unsigned long ulyes = 1;
			if (ioctlsocket(m_sock, FIONBIO, &ulyes) == SOCKET_ERROR)
#else
			if (ioctl(m_sock, FIONBIO, (const char *)&yes) < 0)
#endif
			{
				LogError("ERROR!! VirtualGamepadUDP ioctl FIONBIO\n");
				return false;
			}

			LogInfo("SUCCESS!! open virtual gamepad(SEND).\n");

		} else if (mode == em_Mode::RECEIVE) {
			// pre config.
			if (::bind(m_sock, m_ai->ai_addr, m_ai->ai_addrlen) != 0) {
				LogError("ERROR!! bind\n");
				return false;
			}

			int yes = 1;
#if defined(_WIN32)
			unsigned long ulyes = 1;
			if (ioctlsocket(m_sock, FIONBIO, &ulyes) == SOCKET_ERROR)
#else
			if (ioctl(m_sock, FIONBIO, (const char *)&yes) < 0)
#endif
			{
				LogError("ERROR!! VirtualGamepadUDP ioctl FIONBIO\n");
				return false;
			}

			LogInfo("SUCCESS!! open virtual gamepad(RECEIVE).\n");
		}

		return true;
	}

	bool close() override
	{
		bool ret = false;

		if (!m_ai) {
			freeaddrinfo(m_ai);
			m_ai = nullptr;
		}

		if (m_sock >= 0) {
#ifdef _WIN32
				int r = ::closesocket(m_sock);
#else
				int r = ::close(m_sock);
#endif
				m_sock = -1;
			if (r >= 0) ret = true;
		}

		return ret;
	}

	bool poll(int64_t time_out = 33) override
	{
		if (m_sock == -1)
		{
			if (!open(m_name, m_service, m_mode)) return false;
		}

		const auto str_direction = m_mode == em_Mode::RECEIVE ? "source" : "target";

		{
			if (m_mode == em_Mode::RECEIVE) {
				std::list<std::vector<char>> dataqueue;
				if (m_sock >= 0)
				{
					std::vector<char> pkt(1500);
					socklen_t sin_size;
					struct sockaddr_in from_addr;
					const int stat = recvfrom(m_sock, pkt.data(), pkt.size(), 0, (struct sockaddr *)&from_addr, &sin_size);
					if (stat <= 0)
					{
						// LogInfo("Empty packets\n");
						clear_axis_button_status();
						return false;
					}
					if (stat < pkt.size()) pkt.resize(stat);
					dataqueue.push_back(pkt);

				} else {
					// LogInfo("Empty packets\n");
					clear_axis_button_status();
					return false;
				}

				// LogDebug("dataqueue size = %ld\n", dataqueue.size());
				while (!dataqueue.empty())
				{
					std::vector<char> pkt = dataqueue.front();
					// m_js = njson::from_cbor(pkt);
					std::string str = pkt.data();
					m_js = njson::parse(str);
					dataqueue.pop_front();
				}

			} else if (m_mode == em_Mode::SEND) {
				std::list<std::vector<char>> dataqueue;
				std::vector<char> pkt(1500);
				std::stringstream ss;
				ss /* << std::setw(4) */ << m_js << std::endl;
				std::string ss_str = ss.str();
				// auto cbor = njson::to_cbor(m_js);
				// auto json_size = ss_str.size();
				// auto cbor_size = cbor.size();
				if (ss_str.size() + 1 < pkt.size()) {
					pkt.assign(ss_str.begin(), ss_str.end());
					pkt.resize(ss_str.size() + 1);
					pkt.back() = '\0';
					dataqueue.push_back(pkt);
				} else {
					LogError("ERROR!! pkt size is not enough.\n");
					return false;
				}

				while (!dataqueue.empty())
				{
					std::vector<char> pkt = dataqueue.front();
					int stat = sendto(m_sock, pkt.data(), pkt.size(), 0, m_ai->ai_addr, m_ai->ai_addrlen);
					if (stat < 1)
					{
						LogError("ERROR!! send UDP packet.\n");
						close();	// to reconnect.
						std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_FOR_RECONNECT));
						return false;
					}
					dataqueue.pop_front();
				}
			}
		}

		return true;
	}
};

std::ostream &operator<<(std::ostream &ostr, const VirtualGamepad &vg)
{
	ostr << "axis_motion = " << std::boolalpha << vg.axis_motion << std::endl;
	ostr << "button_down = " << std::boolalpha << vg.button_down << std::endl;
	ostr << "button_up = " << std::boolalpha << vg.button_up << std::endl;

	ostr << "axis_Left_X = " << vg.axis_Left_X << std::endl;
	ostr << "axis_Left_Y = " << vg.axis_Left_Y << std::endl;
	ostr << "axis_Right_X = " << vg.axis_Right_X << std::endl;
	ostr << "axis_Right_Y = " << vg.axis_Right_Y << std::endl;
	ostr << "axis_Trigger_L = " << vg.axis_Trigger_L << std::endl;
	ostr << "axis_Trigger_R = " << vg.axis_Trigger_R << std::endl;

	ostr << "button_A = " << int(vg.button_A) << std::endl;
	ostr << "button_B = " << int(vg.button_B) << std::endl;
	ostr << "button_X = " << int(vg.button_X) << std::endl;
	ostr << "button_Y = " << int(vg.button_Y) << std::endl;
	ostr << "button_Back = " << int(vg.button_Back) << std::endl;
	ostr << "button_Guide = " << int(vg.button_Guide) << std::endl;
	ostr << "button_Start = " << int(vg.button_Start) << std::endl;
	ostr << "button_Stick_L = " << int(vg.button_Stick_L) << std::endl;
	ostr << "button_Stick_R = " << int(vg.button_Stick_R) << std::endl;
	ostr << "button_Shoulder_L = " << int(vg.button_Shoulder_L) << std::endl;
	ostr << "button_Shoulder_R = " << int(vg.button_Shoulder_R) << std::endl;
	ostr << "button_Dpad_U = " << int(vg.button_Dpad_U) << std::endl;
	ostr << "button_Dpad_D = " << int(vg.button_Dpad_D) << std::endl;
	ostr << "button_Dpad_L = " << int(vg.button_Dpad_L) << std::endl;
	ostr << "button_Dpad_R = " << int(vg.button_Dpad_R) << std::endl;

	return ostr;
}
