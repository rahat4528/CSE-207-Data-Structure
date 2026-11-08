#include "CLI.h"

int main(int argc, char* argv[]) {
    DepResolver::CLI app;
    return app.run(argc, argv);
}

