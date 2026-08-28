#include <iostream>
#include <openssl/ssl.h>
#include <Application.hpp>
#include <ProtocolHandler.hpp>

int main(int, char**){
    const long TLS_SESSION_CACHE_SIZE = 1024;
    const long SESSION_CACHE_TIMEOUT = 3600; // 1 hour
    std::string HOSTPORT("54458");
    size_t READ_BUFFER_SIZE = 1024;
    ApplicationConfig config = { HOSTPORT, TLS_SESSION_CACHE_SIZE, SESSION_CACHE_TIMEOUT, READ_BUFFER_SIZE };

    try {
        Application app(config);
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
