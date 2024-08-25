#include <iostream>
#include <vector>
#include <tuple>
#include <cmath>
#include <thread>
#include <atomic>
#include <algorithm>
#include <fstream>
#include <chrono>
#include <mutex>

class Program {
private:
    const long long _worldSeed = -8978430309572744422L;
    const int _threshold = 45;
    const int _threadCount = 8;
    const int _length = 200000;
    int _chunkHalfLength = _length / 32;
    std::vector<std::pair<int, int>> _offsets;
    std::vector<std::tuple<int, int, int>> Candidates;
    std::mutex mtx;

public:
    Program() {
        _offsets = CreateOffsets();
    }

    bool isSlimeChunk(int x, int z) {
        long long seed = ((_worldSeed + (long long)(x * x * 4987142LL) + (long long)(x * 5947611LL) + (long long)(z * z) * 4392871LL + (long long)(z * 389711LL) ^ 987234911LL) ^ 0x5DEECE66DLL) & ((1LL << 48) - 1);
        int bits, val;
        do {
            seed = (seed * 0x5DEECE66DLL + 0xBLL) & ((1LL << 48) - 1);
            bits = (int)(seed >> 17);
            val = bits % 10;
        } while (bits - val + 9 < 0);
        return val == 0;
    }

    std::vector<std::pair<int, int>> CreateOffsets() {
        std::vector<std::pair<int, int>> offsets;
        for (int i = -8; i <= 8; ++i) {
            for (int j = -8; j <= 8; ++j) {
                if (std::sqrt(i * i + j * j) <= 8.0) {
                    offsets.emplace_back(i, j);
                }
            }
        }
        return offsets;
    }

    void WorkerThread(int startX, int stopX) {
        int chunkHalfLength = _chunkHalfLength;
        for (int i = startX; i < stopX; ++i) {
            for (int j = -chunkHalfLength + 8; j < chunkHalfLength / 2 - 7; ++j) {
                int slimeRadiusCounter = 0;
                for (const auto& offset : _offsets) {
                    if (isSlimeChunk(i + offset.first, j + offset.second)) {
                        ++slimeRadiusCounter;
                    }
                }
                if (slimeRadiusCounter >= _threshold) {
                    std::lock_guard<std::mutex> lock(mtx);
                    Candidates.emplace_back(i, j, slimeRadiusCounter);
                }
            }
        }
    }

    void BruteForce() {
        std::cout << "Starting on " << _threadCount << " threads" << std::endl;

        int sectionLength = (_chunkHalfLength * 2 - 16) / _threadCount;
        std::vector<std::thread> threads;

        for (int i = 0; i < _threadCount; ++i) {
            int startX = -_chunkHalfLength + 8 + i * sectionLength;
            int stopX = (i == _threadCount - 1) ? _chunkHalfLength - 8 : -_chunkHalfLength + 8 + ((i + 1) * sectionLength);
            threads.emplace_back(&Program::WorkerThread, this, startX, stopX);
        }

        for (auto& t : threads) {
            if (t.joinable()) {
                t.join();
            }
        }

        std::cout << "\nSearch complete" << std::endl;
    }

    void SaveAndPrintOutput() {
        if (Candidates.empty()) {
            std::cout << "No candidates found\n";
            return;
        }

        std::string output = "Found " + std::to_string(Candidates.size()) + " candidates, max of " +
                            std::to_string(std::get<2>(*std::max_element(Candidates.begin(), Candidates.end(),
                            [](const auto &a, const auto &b) { return std::get<2>(a) < std::get<2>(b); }))) + " slime chunks\n";

        std::ofstream file("candidates.txt");
        file << output;
        for (const auto& candidate : Candidates) {
            file << std::get<0>(candidate) << ", " << std::get<1>(candidate) << ", " << std::get<2>(candidate) << "\n";
        }
        file.close();

        std::cout << output << "Output saved\n";
    }

    void Run() {
        auto start = std::chrono::high_resolution_clock::now();
        BruteForce();
        SaveAndPrintOutput();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        std::cout << "Runtime: " << elapsed.count() << " seconds." << std::endl;
    }
};

int main() { //T(2.n²) n = _length
    Program program;
    program.Run();
    return 0;
}
