#include "../Hosting.h"
extern Hosting g_hosting;
#include "WebSocket.h"
#include "OverlayService.h"

WebSocket WebSocket::instance;

WebSocket::WebSocket() {
    // Constructor implementation (if needed)
}

void WebSocket::createServer(uint16_t port) {
    // Phoenix: si ya hay uno funcionando (o arrancando) no se crea otro. Si el anterior terminó solo
    // (por ejemplo, el puerto estaba ocupado) se limpia antes de crear el nuevo: reemplazar un hilo
    // sin cerrar haría que el programa se cierre.
    if (serverThread_.joinable()) {
        if (isRunning_ || starting_) {
            return;
        }
        serverThread_.join();
    }
    starting_ = true;

    if (!_asioInited) {
        server_.init_asio();
        _asioInited = true;
    }
    else
    {
        server_.reset();
    }
    server_.set_open_handler(std::bind(&WebSocket::onOpen, this, std::placeholders::_1));
    server_.set_close_handler(std::bind(&WebSocket::onClose, this, std::placeholders::_1));
    server_.set_message_handler(std::bind(&WebSocket::onMessage, this, std::placeholders::_1, std::placeholders::_2));

    serverThread_ = std::thread([this, port]() {
        try {
            server_.listen(port);
            server_.start_accept();
            isRunning_ = true;
            starting_ = false;
            g_hosting.logMessage("WebSocket server started.");

            // Start overlay
            if (Config::cfg.overlay.enabled) {
                OverlayService::instance().start();
            }

            server_.run();
        } catch (const std::exception& e) {
            g_hosting.logMessage("Server error: " + std::string(e.what()));
        }
        starting_ = false;
        isRunning_ = false;
        g_hosting.logMessage("WebSocket server stopped.");
    });
}

void WebSocket::stopServer() {
    try { server_.stop_listening(); } catch (...) {}
    try { server_.stop(); } catch (...) {}
    if (serverThread_.joinable()) {
        serverThread_.join();
    }
    {
        std::lock_guard<std::mutex> lock(clientsMutex_);
        clients_.clear();
    }
    if (Config::cfg.overlay.enabled) {
        OverlayService::instance().stop();
    }
    starting_ = false;
    isRunning_ = false;
}

void WebSocket::sendMessageToAll(const std::string& message) {
    // If not running, return
    if (!isRunning_) {
        return;
    }

    // Phoenix: varios hilos mandan mensajes a la vez (chat, ping, mandos) mientras el overlay se conecta
    // o se cierra. La lista se copia bajo el candado y el envío se hace fuera de él; un cliente que se
    // cayó a mitad no puede tumbar el hilo que envía.
    std::vector<websocketpp::connection_hdl> destinos;
    {
        std::lock_guard<std::mutex> lock(clientsMutex_);
        if (clients_.empty()) {
            return;
        }
        destinos.assign(clients_.begin(), clients_.end());
    }

    for (auto& client : destinos) {
        try {
            websocketpp::lib::error_code ec;
            server_.send(client, message, websocketpp::frame::opcode::text, ec);
        }
        catch (...) {}
    }
}

bool WebSocket::isRunning() {
    return isRunning_;
}

bool WebSocket::hasClients() {
    std::lock_guard<std::mutex> lock(clientsMutex_);
    return !clients_.empty();
}

void WebSocket::onOpen(websocketpp::connection_hdl hdl) {
    {
        std::lock_guard<std::mutex> lock(clientsMutex_);
        clients_.insert(hdl);
    }
    g_hosting.logMessage("WebSocket client connected.");
    // Fuera del candado: esta llamada puede mandar mensajes (y sendMessageToAll usa el mismo candado)
    OverlayService::instance().onWebSocketOpen();
}

void WebSocket::onClose(websocketpp::connection_hdl hdl) {
    {
        std::lock_guard<std::mutex> lock(clientsMutex_);
        clients_.erase(hdl);
    }
    OverlayService::instance().onWebSocketClose();
}

void WebSocket::onMessage(websocketpp::connection_hdl hdl, server::message_ptr msg) {
    // Phoenix: un mensaje mal formado no debe tumbar nada
    try {
        json j = json::parse(msg->get_payload());
        if (!j.is_object() || !j.contains("event")) {
            return;
        }
        if (j["event"] == "chat:send") {
            if (!j.contains("data") || !j["data"].is_string()) {
                return;
            }
            string message = j["data"].get<string>();
            // Los mensajes vacíos no se envían al chat
            if (message.find_first_not_of(" \t\r\n") == string::npos) {
                return;
            }
            g_hosting.sendHostMessage(message.c_str());
        }
    }
    catch (...) {}
}
