#pragma once

#include <array>
#include <cstdint>
#include <cstdlib>

template<std::size_t SIZE, std::size_t MAX_COROUTINES>
class CoroutineAllocator {
private:
    struct Frame {
        union {
            std::array<char, SIZE> frame;
            Frame *next{nullptr};
        } data;
    };

public:
    CoroutineAllocator() {
        _allocated_frames = reinterpret_cast<Frame *>(std::aligned_alloc(4096U, MAX_COROUTINES * SIZE));
        _first_free = _allocated_frames;
        for (auto i = 0U; i < MAX_COROUTINES - 1U; ++i) {
            _allocated_frames[i].data.next = &_allocated_frames[i + 1U];
        }
    }

    ~CoroutineAllocator() { std::free(_allocated_frames); }

    void *allocate() {
        auto *next = _first_free;
        _first_free = next->data.next;
        return &next->data.frame;
    }

    void free(void *coroutine) {
        auto *frame = reinterpret_cast<Frame *>(coroutine);
        frame->data.next = _first_free;
        _first_free = frame;
    }

private:
    Frame *_allocated_frames;
    Frame *_first_free;
};