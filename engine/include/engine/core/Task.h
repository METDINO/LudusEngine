#pragma once

#include <coroutine>
#include <functional>
#include <thread>
#include <utility>

template <typename F> struct BuildSciptsAwaitable {
    F func_;

    BuildSciptsAwaitable(F f) : func_(std::forward<F>(f)) {}

    auto await_ready() noexcept { return false; }
    auto await_suspend(std::coroutine_handle<> handle) noexcept {
        std::thread([handle, *this]() { func_(); }).detach();
    }
    void await_resume() noexcept {}
};

template <typename T> struct Coroutine {

    struct promise_type {
        T value;
        Coroutine get_return_object() {
            return Coroutine{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_never initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void unhandled_exception() {}
        template <std::convertible_to<T> From> std::suspend_always yield_value(From&& f) {
            value = std::forward<From>(f);
            return {};
        }

        T return_value(T v) {
            value = std::move(v);
            return {};
        }
    };
    std::coroutine_handle<promise_type> h{};

    ~Coroutine() {
        if (h)
            h.destroy();
    }
};