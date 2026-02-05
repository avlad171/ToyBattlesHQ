#include "../include/AuthServer.h"
#include "../include/AuthSession.h"
#include "../include/Handlers/AuthAuthorizationHandler.h"
#include "../include/Handlers/AuthChannelsHandler.h"


namespace Auth
{
	AuthServer::AuthServer(ioContext& io_context, const std::string& ip, std::uint16_t port)
		: m_io_context(io_context)
		, m_acceptor{ io_context, tcp::endpoint(asio::ip::make_address(ip), port) }
		, m_database()
		, m_authService{m_database}
	{
		Common::Network::Session::addCallback<Common::Network::PacketType::ENCRYPTED, Auth::Network::Session>(22, [&](const Common::Network::Packet& request,
			std::shared_ptr<Auth::Network::Session> session) { Auth::Handlers::handleAuthUserInformation(request, session, m_authService); });

		Common::Network::Session::addCallback<Common::Network::PacketType::ENCRYPTED, Auth::Network::Session>(23, Auth::Handlers::handleServerChannelsInfo);

		Common::Network::Session::addCallback<Common::Network::PacketType::ENCRYPTED, Auth::Network::Session>(81, Auth::Handlers::handleHwidRetrieval);
	}

	void AuthServer::asyncAccept()
	{
		m_socket.emplace(m_io_context);
		m_acceptor.async_accept(*m_socket, [&](asio::error_code error)
			{
				auto client = std::make_shared<Auth::Network::Session>(std::move(*AuthServer::m_socket), nullptr);
				client->m_checkValidSession = false;
				client->sendConnectionACK(Common::Enums::ServerType::AUTH_SERVER);
				asyncAccept();
			});
	}
}
	