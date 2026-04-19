#pragma once
#include <string>
#include <type_traits>

template <class V>
inline constexpr bool is_compressed = std::is_empty_v<V> && !std::is_final_v<V>;  // NOLINT

template <int I, class V, bool EBO = is_compressed<V>>
struct Storage;

template <int I, class V>
struct Storage<I, V, true> : V {
    using V::V;

    Storage() = default;
    Storage(const Storage&) = default;
    Storage(Storage&&) = default;

    template <class T>
    explicit Storage(T&& v) : V(std::forward<T>(v)) {
    }

    V& Get() {
        return *this;
    }

    const V& Get() const {
        return *this;
    }
};

template <int I, class V>
struct Storage<I, V, false> {
    V value_;

    Storage() : value_(V()) {
    }
    Storage(const Storage&) = default;
    Storage(Storage&&) = default;

    template <class T>
    explicit Storage(T&& v) : value_(std::forward<T>(v)) {
    }

    V& Get() {
        return value_;
    }

    const V& Get() const {
        return value_;
    }
};

template <typename F, typename S>
class CompressedPair : Storage<0, F>, Storage<1, S> {
public:
    template <class F1, class S1>
    CompressedPair(F1&& f, S1&& s)
        : Storage<0, F>(std::forward<F1>(f)), Storage<1, S>(std::forward<S1>(s)) {
    }

    CompressedPair() = default;
    CompressedPair(const CompressedPair&) = default;
    CompressedPair(CompressedPair&&) = default;
    CompressedPair& operator=(const CompressedPair&) = default;
    CompressedPair& operator=(CompressedPair&&) = default;
    ~CompressedPair() = default;

    F& GetFirst() {
        return Storage<0, F>::Get();
    }

    const F& GetFirst() const {
        return Storage<0, F>::Get();
    }

    S& GetSecond() {
        return Storage<1, S>::Get();
    }

    const S& GetSecond() const {
        return Storage<1, S>::Get();
    }
};