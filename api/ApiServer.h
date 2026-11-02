#pragma once
#include "httplib.h"
#include "json.hpp"
#include "tividy/scenarios/Session.h"
#include <string>

namespace api {
    using json = nlohmann::json;

    class ApiServer {
    public:
        ApiServer(Session& session, const std::string& webRoot, int port = 8080);
        void listen();

    private:
        void setupRoutes();
        
        Session& session;
        httplib::Server svr;
        int port;
    };
}