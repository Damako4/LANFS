#include <iostream>
#include <openssl/ssl.h>
#include <ServerApplication.hpp>
#include <ProtocolHandler.hpp>

int main(int, char**){
    ApplicationConfig config;
    config.cacheSize = 1024;
    config.cacheTimeout = 3600; // 1 hour
    config.hostport = "54458";
    config.readBufferSize = 1024;
    config.sharedFolderPath = "./build/shared/";

    try {
        ServerApplication app(config);
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
