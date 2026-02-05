#include "Utils/Logger.h"
#include <chrono>
#include <format>
#include <asio/execution_context.hpp>
#include "../include/AuthServer.h"

#include <iostream>
#include <Utils/SetupParser.h>
#include "Utils/Utils.h"


int main()
{
	Common::Utils::setConsoleTitle(L"Microvolts Auth Server");

	auto now = std::chrono::system_clock::now();
	std::time_t t = std::chrono::system_clock::to_time_t(now);

	std::tm tm{};
#if defined(_WIN32)
	localtime_s(&tm, &t);
#else
	localtime_r(&t, &tm);
#endif

	std::cout << "[Info] Auth server initialized on " << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "\n\n";
	auto parsedServerInfo = Common::Utils::SetupParser::getInstance().getAuthSetup();

	Utils::Logger::log(std::format("Server Information: IP: {},  Port: {}",
		parsedServerInfo.ip, parsedServerInfo.port), Utils::LogType::Normal);

	const std::string banner = R"(

   _____   __          __  ____             _         ______                 _       _             
  / ____|  \ \        / / |  _ \           (_)       |  ____|               | |     | |            
 | (___   __\ \  /\  / /__| |_) | ___  __ _ _ _ __   | |__   _ __ ___  _   _| | __ _| |_ ___  _ __ 
  \___ \ / _ \ \/  \/ / _ \  _ < / _ \/ _` | | '_ \  |  __| | '_ ` _ \| | | | |/ _` | __/ _ \| '__|
  ____) | (_) \  /\  /  __/ |_) |  __/ (_| | | | | | | |____| | | | | | |_| | | (_| | || (_) | |   
 |_____/ \___/ \/  \/ \___|____/ \___|\__, |_|_| |_| |______|_| |_| |_|\__,_|_|\__,_|\__\___/|_|   
                                       __/ |                                                       
                                      |___/                                                       

    GitHub: https://github.com/SoWeBegin/MicrovoltsEmulator

)";

	Utils::Logger::log(banner, Utils::LogType::Info);



	asio::io_context io_context;
	Auth::AuthServer srv(io_context, parsedServerInfo.ip, parsedServerInfo.port);
	srv.asyncAccept();
	io_context.run();
}
