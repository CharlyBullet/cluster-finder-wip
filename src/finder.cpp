#include <iostream>
#include <vector>
#include <atomic>
#include <cmath>
#include <tuple>
#include <future>
#include <algorithm>
#include <fstream>
#include <chrono>

namespace Finder
{
    const long _worldSeed = -8978430309572744422L;
    const int _minim = 45;
    const int _threads = 8;

    std::vector<std::pair<int, int>> _chunkOffsets;
    std::vector<std::tuple<int, int, int>> Candidates;

    int _length;
    int _chunkHalf;

    bool isSlimeChunk(int x, int z) {
        //check long vs long long for extend biomes
        // new Random(seed + (long) (i * i * 4987142) + (long) (i * 5947611) + (long) (j * j) * 4392871L + (long) (j * 389711) ^ 987234911L).nextInt(10) == 0
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
                //player radius (16 chunks ≡ 128 blocks /// 8 render distance)
                if (std::sqrt(i * i + j * j) <= 8.0) {
                    offsets.emplace_back(i, j);
                }
            }
        }
        return offsets;
    }

    std::vector<std::tuple<int, int, int>> ProcessChunk(int startX, int stopX, int chunkHalf) {
        std::vector<std::tuple<int, int, int>> localCandidates;

        for (int i = startX; i < stopX; i++) {
            for (int j = -chunkHalf + 8; j < chunkHalf / 2 - 7; j++) {
                int slimeRadiusCounter = 0;
                for (const auto& offset : _chunkOffsets) {
                    if (isSlimeChunk(i + offset.first, j + offset.second)) {
                        slimeRadiusCounter++;
                    }
                }
                if (slimeRadiusCounter >= _minim) {
                    localCandidates.emplace_back(i, j, slimeRadiusCounter);
                }
            }
        }

        return localCandidates;
    }

    void BruteForce() {
        int sectionLength = (_chunkHalf * 2 - 16) / _threads;
        std::vector<std::future<std::vector<std::tuple<int, int, int>>>> futures;
        for (int i = 0; i < _threads; i++) {
            int startX = -_chunkHalf + 8 + i * sectionLength;
            int stopX = i == _threads - 1 ? _chunkHalf - 8 : -_chunkHalf + 8 + ((i + 1) * sectionLength);
            //Chequear async!!!
            futures.push_back(std::async(std::launch::async, ProcessChunk, startX, stopX, _chunkHalf));
        }

        for (auto& future : futures) {
            auto result = future.get();
            Candidates.insert(Candidates.end(), result.begin(), result.end());
        }

        std::cout << "search complete\n";
    }

 void Printer() {
        //Sort and print top 10
        std::sort(Candidates.begin(), Candidates.end(), [](const auto& lhs, const auto& rhs) {
            return std::get<2>(lhs) > std::get<2>(rhs);
        });

        std::ofstream outFile("candidates.txt");
        outFile << "found " << Candidates.size() << " candidates with max of " << std::get<2>(Candidates.front()) << " slime chunks\n";
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