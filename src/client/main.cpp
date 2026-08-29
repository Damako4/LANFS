#include <ClientApplication.hpp>
#include <iostream>

int main(int, char**){
    ApplicationConfig config;
    config.hostname = "127.0.0.1";
    config.hostport = "54458";
    config.readBufferSize = 1024;

    try {
        ClientApplication app(config);
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}