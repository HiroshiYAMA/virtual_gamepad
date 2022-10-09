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

#include <chrono>
#include <mutex>
#include <thread>

#include <srt.h>

#include "devGamepad.h"

#include "json.hpp"
using njson = nlohmann::json;



class VirtualGampad
{
public:
	enum class em_Mode : int {
		SEND,
		RECEIVE,
		NONE,
	};

private:
	em_Mode m_mode = em_Mode::NONE;
	std::string m_name;
	std::string m_service;
	SRTSOCKET m_sock = SRT_INVALID_SOCK;
	addrinfo m_ai;
	bool m_connected = false;

	// epoll.
	int m_pollid = -1;
	int m_srtrfdslen = 2;
	int m_srtwfdslen = 2;
	SRTSOCKET m_srtrwfds[4] = {SRT_INVALID_SOCK, SRT_INVALID_SOCK , SRT_INVALID_SOCK , SRT_INVALID_SOCK };
	// int m_sysrfdslen = 2;
	// SYSSOCKET m_sysrfds[2];

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

protected:

public:
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(
		VirtualGampad,

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

	static std::unique_ptr<VirtualGampad> Create(const std::string &name, const std::string &service, em_Mode mode)
	{
		auto vgmpad = std::make_unique<VirtualGampad>();
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
	bool IsAttached() const { return (m_sock != SRT_INVALID_SOCK); }

	bool Poll( uint32_t timeout=0 )
	{
		return receive(timeout);
	}

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

	VirtualGampad()
	{
		m_pollid = srt_epoll_create();
		if (m_pollid < 0)
		{
			std::cerr << "Can't initialize epoll" << std::endl;
			exit(EXIT_FAILURE);
		}
		LogInfo("m_pollid = %d\n", m_pollid);

		clear_stat();
	}

	virtual ~VirtualGampad()
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

	bool open(const std::string &name, const std::string &service, em_Mode mode)
	{
		m_mode = mode;
		m_name = name;
		m_service = service;

		if (mode == em_Mode::SEND) {
			m_sock = srt_create_socket();
			if ( m_sock == SRT_ERROR ) {
				LogError("srt_create_socket");
				return false;
			}

			// pre config.
			bool no = false;
			int result;
			result = srt_setsockopt(m_sock, 0, SRTO_RCVSYN, &no, sizeof no);
			if ( result == -1 ) {
				LogError("Can't set SRT option : %s(%s)\n", "SRTO_RCVSYN", no ? "true" : "false");
				srt_close(m_sock);
				return false;
			}

			// get addrinfo.
			addrinfo fo = {
				0,
				AF_UNSPEC,
				0, 0,
				0, 0,
				NULL, NULL
			};
			addrinfo* val = nullptr;
			const char *n = (name.empty() || name == "") ? nullptr : name.c_str();
			const char *s = (service.empty() || service == "") ? nullptr : service.c_str();
			int erc = getaddrinfo(n, s, &fo, &val);
			if (erc == 0)
			{
				m_ai = *val;
			}
			freeaddrinfo(val);

			// connect to the receiver, implicit bind
			if (SRT_ERROR == srt_connect(m_sock, m_ai.ai_addr, m_ai.ai_addrlen))
			{
				LogError("connect: %s\n", srt_getlasterror_str());
				srt_close(m_sock);
				return false;
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

			LogInfo("SUCCESS!! open virtual gamepad(SEND).\n");

		} else if (mode == em_Mode::RECEIVE) {
			m_sock = srt_create_socket();
			if ( m_sock == SRT_ERROR ) {
				LogError("srt_create_socket");
				return false;
			}

			// pre config.
			bool no = false;
			int result;
			result = srt_setsockopt(m_sock, 0, SRTO_RCVSYN, &no, sizeof no);
			if ( result == -1 ) {
				LogError("Can't set SRT option : %s(%s)\n", "SRTO_RCVSYN", no ? "true" : "false");
				srt_close(m_sock);
				return false;
			}

			// get addrinfo.
			addrinfo fo = {
				0,
				AF_UNSPEC,
				0, 0,
				0, 0,
				NULL, NULL
			};
			addrinfo* val = nullptr;
			const char *n = (name.empty() || name == "") ? nullptr : name.c_str();
			const char *s = (service.empty() || service == "") ? nullptr : service.c_str();
			int erc = getaddrinfo(n, s, &fo, &val);
			if (erc == 0)
			{
				m_ai = *val;
			}
			freeaddrinfo(val);

			// connect to the receiver, implicit bind
			if (SRT_ERROR == srt_connect(m_sock, m_ai.ai_addr, m_ai.ai_addrlen))
			{
				LogError("connect: %s\n", srt_getlasterror_str());
				srt_close(m_sock);
				return false;
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

			LogInfo("SUCCESS!! open virtual gamepad(RECEIVE).\n");
		}

		return true;
	}

	bool close()
	{
		bool ret = false;

		if (m_sock != SRT_INVALID_SOCK) {
			int r = srt_close(m_sock);
			m_sock = SRT_INVALID_SOCK;
			if (r != SRT_ERROR) ret = true;
		}

		return ret;
	}

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
			if (!m_js.empty()) from_json(m_js, *this);
		} else {
			// LogDebug("poll() : false\n");
		}

		return ret;
	}

	bool poll(int64_t time_out = 33)
	{
		if (m_sock == SRT_INVALID_SOCK) {
			if (!open(m_name, m_service, m_mode)) return false;
		}

		const auto str_direction = m_mode == em_Mode::RECEIVE ? "source" : "target";

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
			for (size_t i = 0; i < sizeof(m_srtrwfds) / sizeof(SRTSOCKET); i++)
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
	// 			case SRTS_LISTENING:
	// 				{
	// 					const bool res = (issource) ?
	// 						src->AcceptNewClient() : tar->AcceptNewClient();
	// 					if (!res)
	// 					{
	// 						cerr << "Failed to accept SRT connection"
	// 							<< endl;
	// 						doabort = true;
	// 						break;
	// 					}

	// 					srt_epoll_remove_usock(pollid, s);

	// 					SRTSOCKET ns = (issource) ?
	// 						src->GetSRTSocket() : tar->GetSRTSocket();
	// 					int events = SRT_EPOLL_IN | SRT_EPOLL_ERR;
	// 					if (srt_epoll_add_usock(pollid, ns, &events))
	// 					{
	// 						cerr << "Failed to add SRT client to poll, "
	// 							<< ns << endl;
	// 						doabort = true;
	// 					}
	// 					else
	// 					{
	// 						if (!cfg.quiet)
	// 						{
	// 							cerr << "Accepted SRT "
	// 								<< dirstring
	// 								<<  " connection"
	// 								<< endl;
	// 						}
	// #ifndef _WIN32
	// 						if (cfg.timeout_mode == 1 && cfg.timeout > 0)
	// 						{
	// 							if (!cfg.quiet)
	// 								cerr << "TIMEOUT: cancel\n";
	// 							alarm(0);
	// 						}
	// #endif
	// 						if (issource)
	// 							srcConnected = true;
	// 						else
	// 							tarConnected = true;
	// 					}
	// 				}
	// 				break;
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
						pkt.clear();
						return false;
					}
					if (stat < pkt.size()) pkt.resize(stat);
					dataqueue.push_back(pkt);

				} else {
					// LogInfo("Empty packets\n");
					if (!m_js.empty()) {
						m_js["axis_motion"] = false;
						m_js["button_down"] = false;
						m_js["button_up"] = false;
					}
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

	friend std::ostream &operator<<(std::ostream &ostr, const VirtualGampad &vg);
};

std::ostream &operator<<(std::ostream &ostr, const VirtualGampad &vg)
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
