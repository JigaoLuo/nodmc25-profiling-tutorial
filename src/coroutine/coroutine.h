#pragma once

#include <coroutine>
#include <cstdint>
#include "coroutine_allocator.h"
#include "prefetch_descriptor.h"

thread_local CoroutineAllocator</* size of one coroutine frame*/ 256U, /* max coroutines */ 32U> coro_allocator;

class Annotation {
public:
    Annotation() noexcept = default;

    explicit Annotation(const std::uint16_t execution_time) noexcept: _execution_time(execution_time) {}

    explicit Annotation(const PrefetchDescriptor prefetch_descriptor) noexcept: _prefetch_descriptor(
            prefetch_descriptor) {}


    ~Annotation() noexcept = default;

    [[nodiscard]] std::uint16_t execution_time() const noexcept { return _execution_time; }

    [[nodiscard]] PrefetchDescriptor prefetch_descriptor() const noexcept { return _prefetch_descriptor; }

private:
    std::uint16_t _execution_time{0U};
    PrefetchDescriptor _prefetch_descriptor;
};

class Coroutine {
public:
    enum Stage : std::uint8_t {
        KeyLookup = 0U,
        ValueLookup = 1U,
    };

    struct promise_type /// This name is forced by the standard.
    {
        using Handle = std::coroutine_handle<promise_type>;

        Coroutine get_return_object() { return Coroutine{Handle::from_promise(*this)}; }

        std::suspend_always await_transform(Annotation &&annotation) {
            _annotation = annotation;
            return {};
        }

        /// Never suspend when first time spawning a coroutine.
        std::suspend_never initial_suspend() noexcept { return {}; }

        /// Always suspend when the coroutine returned.
        std::suspend_always final_suspend() noexcept { return {}; }

        /// What happens if the coroutine calls co_return;
        void return_value(Annotation &&annotation) { _annotation = annotation; }

        /// What happens if there is an unhandled exception.
        void unhandled_exception() {}

        void *operator new([[maybe_unused]] const std::size_t /* size */) noexcept { return coro_allocator.allocate(); }

        void operator delete(void *pointer, [[maybe_unused]] const std::size_t /* size */) noexcept {
            coro_allocator.free(pointer);
        }

        /// Annotation by the application.
        Annotation _annotation;
    };

    Coroutine() noexcept = default;

    explicit Coroutine(promise_type::Handle coroutine) : _coroutine_handle(coroutine) {}

    /**
     * Deallocate the frame pointer.
     */
    void destroy() { _coroutine_handle.destroy(); }

    /**
     * Continue execution.
     */
    void resume() { _coroutine_handle.resume(); }

    /**
     * @return True, if the coroutine co_returned.
     */
    [[nodiscard]] bool is_done() const { return _coroutine_handle.done(); }

    /**
     * @return Annotation by the application.
     */
    [[nodiscard]] Annotation annotation() const noexcept { return _coroutine_handle.promise()._annotation; }

private:
    promise_type::Handle _coroutine_handle;
};