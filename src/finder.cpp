#include <iostream>
#include <vector>
#include <atomic>
#include <cmath>
#include <tuple>
#include <future>
#include <algorithm>
#include <fstream>
#include <chrono>
#include <thread>
#include <mutex>

namespace Finder
{
    const long _worldSeed = -8978430309572744422L;
    const int _minim = 45;
    const int _threads = 8;

    std::vector<std::pair<int, int>> _chunkOffsets;
    std::vector<std::tuple<int, int, int>> Candidates;
    std::mutex candidatesMutex;
    std::mutex progressMutex;

    const int _length = 160000; //1600000;
    int _chunkHalf = _length / 32;

    struct ThreadParams {
        int StartX;
        int StopX;
        int ChunkHalfLength;
        std::atomic<int> PercentComplete;
        std::atomic<bool> Complete;
    };

    bool isSlimeChunk(int x, int z) {
        long seed = ((_worldSeed + (long)(x * x * 4987142) + (long)(x * 5947611) + (long)(z * z) * 4392871L + (long)(z * 389711) ^ 987234911L) ^ 0x5DEECE66DL) & ((1L << 48) - 1);
        int bits, val;
        do {
            seed = (seed * 0x5DEECE66DL + 0xBL) & ((1L << 48) - 1);
            bits = (int)(seed >> 17);
            val = bits % 10;
        } while (bits - val + 9 < 0);
        return val == 0;
    }

    std::vector<std::pair<int, int>> CreateChunkOffsets() {
        std::vector<std::pair<int, int>> offsets;
        for (int i = -8; i < 9; i++) {
            for (int j = -8; j < 9; j++) {
                if (std::sqrt(i * i + j * j) <= 8.0) {
                    offsets.emplace_back(i, j);
                }
            }
        }
        return offsets;
    }

    void ProcessChunk(ThreadParams& tParams) {
        int startX = tParams.StartX;
        int stopX = tParams.StopX;
        int chunkHalfLength = tParams.ChunkHalfLength;
        int diff = stopX - startX;

        for (int i = startX; i < stopX; i++) {
            int percentComplete = static_cast<int>(((i - startX) / static_cast<double>(diff)) * 100);
            if (percentComplete > tParams.PercentComplete.load()) {
                tParams.PercentComplete.store(percentComplete);
            }

            for (int j = -chunkHalfLength + 8; j < chunkHalfLength / 2 - 7; j++) {
                int slimeRadiusCounter = 0;
                for (const auto& delta : _chunkOffsets) {
                    if (isSlimeChunk(i + delta.first, j + delta.second)) {
                        ++slimeRadiusCounter;
                    }
                }
                if (slimeRadiusCounter >= _minim) {
                    std::lock_guard<std::mutex> lock(candidatesMutex);
                    Candidates.emplace_back(i, j, slimeRadiusCounter);
                }
            }
        }
        tParams.Complete.store(true);
    }

    void BruteForce() {
        int sectionLength = (_chunkHalf * 2 - 16) / _threads;
        std::vector<std::thread> threads;
        std::vector<ThreadParams> threadParams(_threads);

        for (int i = 0; i < _threads; ++i) {
            threadParams[i].StartX = -_chunkHalf + 8 + i * sectionLength;
            threadParams[i].StopX = (i == _threads - 1) ? _chunkHalf - 8 : -_chunkHalf + 8 + ((i + 1) * sectionLength);
            threadParams[i].ChunkHalfLength = _chunkHalf;
            threadParams[i].PercentComplete = 0;
            threadParams[i].Complete = false;

            threads.emplace_back(ProcessChunk, std::ref(threadParams[i]));
        }

        double threadWeight = 1.0 / _threads;
        long greatestTotalMemory = 0;
        while (!std::all_of(threadParams.begin(), threadParams.end(), [](const ThreadParams& tp) { return tp.Complete.load(); })) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));

            std::string threadPercentLine = "";
            double percentComplete = 0.0;
            for (const auto& tp : threadParams) {
                percentComplete += tp.PercentComplete.load() / 100.0 * threadWeight;
                threadPercentLine += "\t" + std::to_string(tp.PercentComplete.load()) + "%";
            }

            if (percentComplete > greatestTotalMemory) {
                greatestTotalMemory = percentComplete;
            }

            std::cout << "\rTotal: " << percentComplete * 100 << "%   Individual:" << threadPercentLine;
        }

        std::cout << "\nBrute force search complete using " << (greatestTotalMemory / static_cast<double>(1000000000)) << "GB\n";
    }

    void Printer() {
        std::sort(Candidates.begin(), Candidates.end(), [](const auto& lhs, const auto& rhs) {
            return std::get<2>(lhs) > std::get<2>(rhs);
        });

        std::ofstream outFile("candidates.txt");
        outFile << "Found " << Candidates.size() << " candidates with a max of " << std::get<2>(Candidates.front()) << " slime chunks\n";
        for (const auto& candidate : Candidates) {
            outFile << std::get<0>(candidate) << ", " << std::get<1>(candidate) << ", " << std::get<2>(candidate) << "\n";
        }
        outFile.close();

        std::cout << "Top 10 List:\n";
        for (size_t i = 0; i < std::min(Candidates.size(), size_t(10)); ++i) {
            std::cout << std::get<0>(Candidates[i]) << ", " << std::get<1>(Candidates[i]) << ", " << std::get<2>(Candidates[i]) << "\n";
        }
    }

    template<typename Func>
    void Time(Func func, const std::string& actionName) {
        auto start = std::chrono::high_resolution_clock::now();
        func();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        std::cout << actionName << " completed in " << elapsed.count() << " seconds\n";
    }

    void Run() {
        _chunkOffsets = CreateChunkOffsets();
        Time(BruteForce, "BruteForce");
        Printer();
    }

    int Main() {
        Run();
        return 0;
    }
}

int main() {
    return Finder::Main();
}
