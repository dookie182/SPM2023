#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <atomic>
#include <thread>
#include <queue>
#include <chrono>
#include "../include/utimer.cpp"


const int kNumThreads = 64;

struct HuffmanNode {
    char character;
    int frequency;
    HuffmanNode* left;
    HuffmanNode* right;

    HuffmanNode(char c, int freq) : character(c), frequency(freq), left(nullptr), right(nullptr) {}
};

struct CompareNodes {
    bool operator()(HuffmanNode* a, HuffmanNode* b) {
        return a->frequency > b->frequency;
    }
};

void MapFrequencies(const std::string& text, std::atomic<int> frequencies[]) {
    for (char c : text) {
        ++frequencies[static_cast<unsigned char>(c)];
    }

    int count = 0;
    for (int i = 0; i < 256; ++i) {
        if (frequencies[i] > 0) {
            ++count;
            if (count == 2) {
                return;
            }
        }
    }

    std::cerr << "Errore: Meno di due caratteri con frequenza maggiore di zero." << std::endl;
    exit(1);
}


void ReduceFrequencies(const std::atomic<int> mappedFrequencies[], std::atomic<int> reducedFrequencies[]) {
    for (int i = 0; i < 256; ++i) {
        for (int j = 0; j < kNumThreads; ++j) {
            reducedFrequencies[i] += mappedFrequencies[j * 256 + i].load();
        }
    }
}

HuffmanNode* BuildHuffmanTree(const std::unordered_map<char, int>& frequencies) {
    if (frequencies.size() < 2) {
        // Gestisci il caso in cui ci sono meno di due caratteri con frequenza maggiore di zero
        // Restituisci nullptr o un valore di default, a seconda del tuo caso d'uso
        return nullptr;
    }

    std::priority_queue<HuffmanNode*, std::vector<HuffmanNode*>, CompareNodes> pq;

    for (const auto& pair : frequencies) {
        pq.push(new HuffmanNode(pair.first, pair.second));
    }

    while (pq.size() > 1) {
        HuffmanNode* left = pq.top();
        pq.pop();
        HuffmanNode* right = pq.top();
        pq.pop();

        HuffmanNode* newNode = new HuffmanNode('$', left->frequency + right->frequency);
        newNode->left = left;
        newNode->right = right;

        pq.push(newNode);
    }

    return pq.top();
}

void GenerateHuffmanCodes(HuffmanNode* root, const std::string& prefix, std::unordered_map<char, std::string>& codes) {
    if (root->left == nullptr && root->right == nullptr) {
        codes[root->character] = prefix;
        return;
    }

    if (root->left != nullptr) {
        GenerateHuffmanCodes(root->left, prefix + "0", codes);
    }
    if (root->right != nullptr) {
        GenerateHuffmanCodes(root->right, prefix + "1", codes);
    }
}

void EncodeText(const std::string& text, const std::unordered_map<char, std::string>& codes, std::string& encodedText) {
    for (char c : text) {
        encodedText += codes.at(c);
    }
}

void CompressText(const std::string& inputFilename, const std::string& outputFilename, int numThreads) {
    // Leggi il file di input
    std::ifstream inputFile(inputFilename);
    std::string text((std::istreambuf_iterator<char>(inputFile)), std::istreambuf_iterator<char>());

    // Frequenze mappate
    std::atomic<int> mappedFrequencies[kNumThreads * 256]{};  // Aggiunta delle parentesi graffe per inizializzare a zero

    // Crea i thread per la mappatura delle frequenze
    std::thread threads[kNumThreads];
    for (int i = 0; i < numThreads; ++i) {
        std::string threadText = text.substr(i * text.size() / numThreads, text.size() / numThreads);
        threads[i] = std::thread(MapFrequencies, threadText, mappedFrequencies + i * 256);
    }

    // Attendi la fine di tutti i thread
    for (int i = 0; i < numThreads; ++i) {
        threads[i].join();
    }

    // Frequenze ridotte
    std::atomic<int> reducedFrequencies[256]{};

    // Riduci le frequenze
    ReduceFrequencies(mappedFrequencies, reducedFrequencies);

    // Costruisci l'albero di Huffman
    std::unordered_map<char, int> frequencies;
    for (int i = 0; i < 256; ++i) {
        if (reducedFrequencies[i] > 0) {
            frequencies[static_cast<char>(i)] = reducedFrequencies[i];
        }
    }

    HuffmanNode* root = BuildHuffmanTree(frequencies);

    if (root == nullptr) {
        std::cerr << "Errore: Meno di due caratteri con frequenza maggiore di zero." << std::endl;
        return;
    }

    // Genera i codici di Huffman
    std::unordered_map<char, std::string> codes;
    GenerateHuffmanCodes(root, "", codes);

    // Codifica il testo
    std::string encodedText;
    EncodeText(text, codes, encodedText);

    // Scrivi il file compresso
    std::ofstream outputFile(outputFilename);
    if (outputFile.is_open()) {
        // Scrivi le frequenze ridotte
        for (int i = 0; i < 256; ++i) {
            outputFile << reducedFrequencies[i] << "\n";
        }
        outputFile << "\n";

        // Scrivi il testo codificato
        outputFile << encodedText;

        outputFile.close();
        std::cout << "Compressione completata. File compresso creato: " << outputFilename << std::endl;
    } else {
        std::cerr << "Impossibile aprire il file di output." << std::endl;
    }

    // Dealloca la memoria dell'albero di Huffman
    delete root;
}

int main() {
    std::string inputFilename = "../data/dataset.txt";
    std::string outputFilename = "../data/compressed_dataset.txt";

    // Esegui la codifica utilizzando un numero crescente di thread
   for (int numThreads = 1; numThreads <= kNumThreads; numThreads *= 2) {
        std::cout << "Esecuzione con " << numThreads << " thread." << std::endl;

        long sequentialTime;
        {
            utimer sequentialTimer("Esecuzione sequenziale", &sequentialTime);
            CompressText(inputFilename, outputFilename, 1);
        }

        long parallelTime;
        {
            utimer parallelTimer("Esecuzione parallela", &parallelTime);
            CompressText(inputFilename, outputFilename, numThreads);
        }

        std::cout << "Tempo sequenziale: " << sequentialTime << " microsecondi." << std::endl;
        std::cout << "Tempo parallelo: " << parallelTime << " microsecondi." << std::endl;

        if (numThreads == 1) {
            std::cout << "Speedup rispetto all'esecuzione sequenziale: 1.0" << std::endl;
        } else {
            double speedup = static_cast<double>(sequentialTime) / parallelTime;
            std::cout << "Speedup rispetto all'esecuzione sequenziale: " << speedup << std::endl;
        }

        std::cout << std::endl;
    }
    return 0;
}
