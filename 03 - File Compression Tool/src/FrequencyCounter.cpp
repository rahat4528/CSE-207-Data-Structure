#include "FrequencyCounter.h"

namespace HuffmanEngine {

CustomFrequencyMap::CustomFrequencyMap() {
    buckets.fill(nullptr);
}

CustomFrequencyMap::~CustomFrequencyMap() {
    clear();
}

void CustomFrequencyMap::clear() {
    for (size_t i = 0; i < BUCKET_COUNT; ++i) {
        FrequencyNode* curr = buckets[i];
        while (curr) {
            FrequencyNode* next = curr->next;
            delete curr;
            curr = next;
        }
        buckets[i] = nullptr;
    }
    uniqueCount = 0;
}

void CustomFrequencyMap::insertOrAdd(byte_t symbol, frequency_t amount) {
    size_t idx = hash(symbol);
    FrequencyNode* curr = buckets[idx];
    while (curr) {
        if (curr->symbol == symbol) {
            curr->count += amount;
            return;
        }
        curr = curr->next;
    }
    // New node insertion
    FrequencyNode* newNode = new FrequencyNode{symbol, amount, buckets[idx]};
    buckets[idx] = newNode;
    uniqueCount++;
}

frequency_t CustomFrequencyMap::get(byte_t symbol) const {
    size_t idx = hash(symbol);
    FrequencyNode* curr = buckets[idx];
    while (curr) {
        if (curr->symbol == symbol) {
            return curr->count;
        }
        curr = curr->next;
    }
    return 0;
}

bool CustomFrequencyMap::contains(byte_t symbol) const {
    size_t idx = hash(symbol);
    FrequencyNode* curr = buckets[idx];
    while (curr) {
        if (curr->symbol == symbol) return true;
        curr = curr->next;
    }
    return false;
}

FrequencyCounter::FrequencyCounter() {
    reset();
}

void FrequencyCounter::reset() {
    frequencies.fill(0);
    totalBytes = 0;
    distinctSymbols = 0;
    customMap.clear();
}

void FrequencyCounter::countByte(byte_t b, frequency_t count) {
    if (frequencies[b] == 0) {
        distinctSymbols++;
    }
    frequencies[b] += count;
    totalBytes += count;
    customMap.insertOrAdd(b, count);
}

void FrequencyCounter::countBuffer(const uint8_t* buffer, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        countByte(buffer[i], 1);
    }
}

Result<void> FrequencyCounter::countStream(IInputStream& stream, size_t bufferSize) {
    std::vector<uint8_t> buffer(bufferSize);
    while (!stream.isEof()) {
        size_t bytesRead = stream.read(buffer.data(), bufferSize);
        if (bytesRead == 0) break;
        countBuffer(buffer.data(), bytesRead);
    }
    return Result<void>::ok();
}

double FrequencyCounter::calculateEntropy() const {
    if (totalBytes == 0) return 0.0;
    double entropy = 0.0;
    double totalD = static_cast<double>(totalBytes);
    for (size_t i = 0; i < ALPHABET_SIZE; ++i) {
        if (frequencies[i] > 0) {
            double p = static_cast<double>(frequencies[i]) / totalD;
            entropy -= p * (std::log2(p));
        }
    }
    return (entropy < 0.0) ? 0.0 : entropy;
}

uint64_t FrequencyCounter::calculateTheoreticalMinimumBits() const {
    if (totalBytes == 0) return 0;
    double entropy = calculateEntropy();
    return static_cast<uint64_t>(std::ceil(entropy * static_cast<double>(totalBytes)));
}

std::optional<std::pair<byte_t, frequency_t>> FrequencyCounter::getMostFrequentSymbol() const {
    if (distinctSymbols == 0) return std::nullopt;
    byte_t bestSym = 0;
    frequency_t maxFreq = 0;
    for (size_t i = 0; i < ALPHABET_SIZE; ++i) {
        if (frequencies[i] > maxFreq) {
            maxFreq = frequencies[i];
            bestSym = static_cast<byte_t>(i);
        }
    }
    return std::make_pair(bestSym, maxFreq);
}

std::optional<std::pair<byte_t, frequency_t>> FrequencyCounter::getLeastFrequentSymbol() const {
    if (distinctSymbols == 0) return std::nullopt;
    byte_t bestSym = 0;
    frequency_t minFreq = UINT64_MAX;
    for (size_t i = 0; i < ALPHABET_SIZE; ++i) {
        if (frequencies[i] > 0 && frequencies[i] < minFreq) {
            minFreq = frequencies[i];
            bestSym = static_cast<byte_t>(i);
        }
    }
    return std::make_pair(bestSym, minFreq);
}

} // namespace HuffmanEngine
