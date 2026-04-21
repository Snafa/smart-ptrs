#pragma once

#include "compressed_pair.h"

#include <cstddef>  // std::nullptr_t
#include <memory>

template<class T>
struct Slug {
    Slug() = default;

    template<class U, class = std::enable_if_t<std::is_convertible_v<U *, T *> > >
    explicit Slug(const Slug<U> &) noexcept {
    }

    template<class U, class = std::enable_if_t<std::is_convertible_v<U *, T *> > >
    Slug &operator=(const Slug<U> &) noexcept {
        return *this;
    }

    void operator()(T *t) const noexcept {
        delete t;
    }
};

template<class T>
struct Slug<T[]> {
    void operator()(T *t) const noexcept {
        delete[] t;
    }
};

template<typename T, typename Deleter>
class Base {
public:
    explicit Base(T *ptr = nullptr) : data_(ptr, Deleter()) {
    }

    Base(T *ptr, const Deleter &deleter) : data_(ptr, deleter) {
    }

    Base(T *ptr, Deleter &&deleter) : data_(ptr, std::move(deleter)) {
    }

    Base(Base &&other) noexcept {
        GetPtr() = other.GetPtr();
        GetDeleter() = std::move(std::move(other.GetDeleter()));
        other.GetPtr() = nullptr;
    }

    Base(const Base &) = delete;

    Base &operator=(const Base &) = delete;

    Base &operator=(Base &&other) noexcept {
        if (this != &other) {
            Reset();
            std::swap(GetPtr(), other.GetPtr());
            std::swap(GetDeleter(), other.GetDeleter());
            other.GetPtr() = nullptr;
        }
        return *this;
    }

    Base &operator=(std::nullptr_t) {
        Reset();
        return *this;
    }

    ~Base() {
        Reset();
    }

    T *Release() {
        T *__p = GetPtr(); // NOLINT
        GetPtr() = nullptr;
        return __p;
    }

    void Reset(T *ptr = nullptr) {
        if (GetPtr() != ptr) {
            T *__old_p = GetPtr(); // NOLINT
            GetPtr() = ptr;
            if (__old_p) {
                GetDeleter()(__old_p);
            }
        }
    }

    void Swap(Base &other) {
        std::swap(other.GetPtr(), GetPtr());
        std::swap(other.GetDeleter(), GetDeleter());
    }

    T *Get() const {
        return GetPtr();
    }

    Deleter &GetDeleter() {
        return data_.GetSecond();
    }

    const Deleter &GetDeleter() const {
        return data_.GetSecond();
    }

    explicit operator bool() const {
        return GetPtr() != nullptr;
    }

protected:
    CompressedPair<T *, Deleter> data_;

    T *&GetPtr() {
        return data_.GetFirst();
    }

    T *GetPtr() const {
        return data_.GetFirst();
    }
};

// Primary template
template<typename T, typename Deleter = Slug<T> >
class UniquePtr : public Base<T, Deleter> {
public:
    using Base<T, Deleter>::Base;
    using Base<T, Deleter>::operator=;

    T &operator*() const {
        return *Base<T, Deleter>::GetPtr();
    }

    T *operator->() const {
        return Base<T, Deleter>::GetPtr();
    }

    template<class U, class E,
        std::enable_if_t<
            std::is_convertible_v<U *, T *> && std::is_constructible_v<Deleter, E &&>, int> = 0>
    UniquePtr(UniquePtr<U, E> &&other) noexcept
        : Base<T, Deleter>(static_cast<T *>(other.Release()),
                           Deleter(std::move(other.GetDeleter()))) {
    }

    template<class U, class E,
        std::enable_if_t<std::is_convertible_v<U *, T *> && std::is_assignable_v<Deleter &, E &&>,
            int> = 0>
    UniquePtr &operator=(UniquePtr<U, E> &&other) noexcept {
        this->Reset(static_cast<T *>(other.Release()));
        this->GetDeleter() = std::move(other.GetDeleter());
        return *this;
    }
};

template<typename T, typename Deleter>
class UniquePtr<T[], Deleter> : public Base<T, Deleter> {
public:
    using Base<T, Deleter>::Base;
    using Base<T, Deleter>::operator=;

    T &operator[](std::size_t sz) {
        return Base<T, Deleter>::GetPtr()[sz];
    }
};

template<typename Deleter>
class UniquePtr<void, Deleter> : public Base<void, Deleter> {
public:
    using Base<void, Deleter>::Base;
    using Base<void, Deleter>::operator=;
};
