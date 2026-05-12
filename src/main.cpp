#include "router.hpp"
#include <iostream>

int main(int argc, char **argv) {
    if (argc < 4) {
        std::cout << "Usage: ./router-basic <command> --input-dir <dir>\n";
        return 1;
    }

    std::string command = argv[1];
    std::string inputDir = argv[3];

    Router router;
    router.load(inputDir);

    if (command == "show-routes") {
        router.showRoutes();
    }
    else if (command == "show-interfaces") {
        router.showInterfaces();
    }
    else if (command == "lookup") {
        if (argc < 5) {
            std::cout << "lookup requires IP\n";
            return 1;
        }
        router.lookup(argv[2]);
    }
    else if (command == "explain-lookup") {
        router.explainLookup(argv[2]);
    }
    else if (command == "replay-events") {
        router.replayEvents(inputDir);
    }
    else {
        std::cout << "Unknown command\n";
    }

    return 0;
}
