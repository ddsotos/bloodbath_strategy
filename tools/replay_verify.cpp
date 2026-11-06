#include "engine/core.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>

int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string(argv[1]) == "--json") {
            std::cout << bloodbath::serialize_result(bloodbath::run_toy(42, 12)) << '\n';
            return 0;
        }
        auto log = bloodbath::run_toy(42, 12).replay;
        if (argc == 2) {
            std::ifstream file(argv[1]);
            if (!file) throw std::runtime_error("cannot open replay file");
            log = bloodbath::deserialize(std::string(std::istreambuf_iterator<char>(file), {}));
        } else if (argc != 1) {
            throw std::runtime_error("usage: replay_verify [toy-replay-file | --json]");
        }
        const auto replayed = bloodbath::replay_toy(bloodbath::deserialize(bloodbath::serialize(log)));
        if (replayed.replay != log) throw std::runtime_error("replay mismatch");
        std::cout << bloodbath::serialize(log);
        std::cout << "verified toy replay; no official game rules implemented\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
