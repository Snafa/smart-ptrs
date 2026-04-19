#pragma once

#include "sw_fwd.h"  // Forward declaration

#include <cassert>
#include <cstddef>  // std::nullptr_t
#include <iterator>

namespace shared_ptr_detail {
class ControlBlock {
public:
    ControlBlock() : count_(0) {
    }

    ControlBlock(const size_t count) : count_(count) {
    }

    ControlBlock(const ControlBlock&) = delete;

    virtual ~ControlBlock() = default;

    void Disconnect() {
        if (--count_ == 0) {
            Destroy();
            Delete();
        }
    }

    void Connect() {
        ++count_;
    }

    size_t Count() const {
        return count_;
    }

    virtual void Destroy() = 0;
    virtual void* GetPtrMakeShared() = 0;

private:
    size_t count_;

    void Delete() {
        delete this;
    }
};

template <typename T>
class MakeSharedControlBlock : public ControlBlock {
public:
    // MakeSharedControlBlock() = delete;

    template <typename... Args>
    MakeSharedControlBlock(Args&&... args) : ControlBlock(1) {
        new (data_) T(std::forward<Args>(args)...);
    }

    T* GetPtr() {
        return reinterpret_cast<T*>(data_);
    }

    void* GetPtrMakeShared() override {
        return reinterpret_cast<void*>(data_);
    }

    void Destroy() override {
        GetPtr()->~T();
    }

private:
    alignas(alignof(T)) std::byte data_[sizeof(T)];
};

template <typename T>
class PtrControlBlock : public ControlBlock {
public:
    PtrControlBlock() : ControlBlock(0), data_(nullptr) {
    }

    template <typename Y>
    PtrControlBlock(Y* data) : ControlBlock(1), data_(data) {
    }

    T* GetPtr() {
        return data_;
    }

    void* GetPtrMakeShared() override {
        return static_cast<void*>(data_);
    }

    void Destroy() override {
        delete data_;
    }

private:
    T* data_;
};
};  // namespace shared_ptr_detail

// https://en.cppreference.com/w/cpp/memory/shared_ptr
template <typename T>
class SharedPtr {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    SharedPtr() : ptr_(nullptr), data_(nullptr) {
    }

    SharedPtr(std::nullptr_t) : ptr_(nullptr), data_(nullptr) {
    }

    template <typename Y>
    explicit SharedPtr(Y* ptr) : ptr_(ptr), data_(new shared_ptr_detail::PtrControlBlock<Y>(ptr)) {
    }

    template <typename Y>
    SharedPtr(const SharedPtr<Y>& other) : ptr_(other.GetPtr()), data_(other.GetData()) {
        if (*this) {
            GetData()->Connect();
        }
    }

    SharedPtr(const SharedPtr& other) : ptr_(other.GetPtr()), data_(other.GetData()) {
        if (*this) {
            GetData()->Connect();
        }
    }

    template <typename Y>
    SharedPtr(SharedPtr<Y>&& other)
        : ptr_(std::move(other.GetPtr())), data_(std::move(other.GetData())) {
        other.ptr_ = nullptr;
        other.data_ = nullptr;
    }

    // Aliasing constructor
    // #8 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    template <typename Y>
    SharedPtr(const SharedPtr<Y>& other, T* ptr) : ptr_(ptr), data_(other.GetData()) {
        assert(GetData());
        if (*this) {
            GetData()->Connect();
        }
    }

    // Promote `WeakPtr`
    // #11 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    explicit SharedPtr(const WeakPtr<T>& other);

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    SharedPtr& operator=(const SharedPtr& other) {
        if (this != &other) {
            Reset();
            GetPtr() = other.GetPtr();
            GetData() = other.GetData();
            if (*this) {
                GetData()->Connect();
            }
        }
        return *this;
    }

    SharedPtr& operator=(SharedPtr&& other) {
        Reset();
        std::swap(this->GetData(), other.GetData());
        std::swap(this->GetPtr(), other.GetPtr());
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~SharedPtr() {
        Reset();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void Reset() {
        if (*this) {
            GetPtr() = nullptr;
            GetData()->Disconnect();
            GetData() = nullptr;
        }
    }

    template <typename Y>
    void Reset(Y* ptr) {
        if (*this) {
            GetData()->Disconnect();
        }
        GetPtr() = ptr;
        GetData() = ptr ? new shared_ptr_detail::PtrControlBlock<Y>(ptr) : nullptr;
    }

    void Swap(SharedPtr& other) {
        std::swap(*this, other);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() const {
        return GetPtr();
    }
    T& operator*() const {
        return *GetPtr();
    }
    T* operator->() const {
        return GetPtr();
    }
    size_t UseCount() const {
        return !*this ? 0 : GetData()->Count();
    }
    explicit operator bool() const {
        return GetPtr() != nullptr;
    }

private:
    T* ptr_;
    shared_ptr_detail::ControlBlock* data_;

    T*& GetPtr() {
        return ptr_;
    }

    shared_ptr_detail::ControlBlock*& GetData() {
        return data_;
    }

    T* GetPtr() const {
        return ptr_;
    }

    shared_ptr_detail::ControlBlock* GetData() const {
        return data_;
    }

    SharedPtr(T* ptr, shared_ptr_detail::ControlBlock* data) : ptr_(ptr), data_(data) {
    }

    template <typename U, typename... A>
    friend SharedPtr<U> MakeShared(A&&...);

    template <typename U>
    friend class SharedPtr;
};

template <typename T, typename U>
inline bool operator==(const SharedPtr<T>& left, const SharedPtr<U>& right) {
    return left.GetPtr() == right.GetPtr() && left.GetData() == right.GetData();
}

// Allocate memory only once
template <typename T, typename... Args>
SharedPtr<T> MakeShared(Args&&... args) {
    auto* data = new shared_ptr_detail::MakeSharedControlBlock<T>(std::forward<Args>(args)...);
    auto* ptr = static_cast<T*>(data->GetPtrMakeShared());
    return SharedPtr<T>(ptr, data);
}

// Look for usage examples in tests
template <typename T>
class EnableSharedFromThis {
public:
    SharedPtr<T> SharedFromThis();
    SharedPtr<const T> SharedFromThis() const;

    WeakPtr<T> WeakFromThis() noexcept;
    WeakPtr<const T> WeakFromThis() const noexcept;
};
