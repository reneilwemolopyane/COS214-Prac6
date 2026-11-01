#include "ApiServer.h"
#include "tividy/scenarios/Session.h"
#include <iostream>

int main(int argc, char** argv) {
    int port = 8080;
    std::string webRoot = "web";
    if (argc > 1) port = std::atoi(argv[1]);
    if (argc > 2) webRoot = argv[2];

    // Boot up the two industries using Session
    Session session;
    
    api::ApiServer server(session, webRoot, port);
    server.listen();

    return 0;
}