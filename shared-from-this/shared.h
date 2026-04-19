#pragma once

#include "sw_fwd.h"  // Forward declaration

#include <new>
#include <cassert>
#include <cstddef>  // std::nullptr_t
#include <utility>
#include <iterator>
#include <type_traits>

namespace ptr_detail {
class ControlBlock {
public:
    ControlBlock() : strong_count_(0), weak_count_(0) {
    }

    explicit ControlBlock(const size_t strong) : strong_count_(strong), weak_count_(1) {
    }

    ControlBlock(const ControlBlock&) = delete;

    virtual ~ControlBlock() = default;

    void SharedDisconnect() {
        assert(strong_count_ > 0);
        if (--strong_count_ == 0) {
            Destroy();
            if (--weak_count_ == 0) {
                Delete();
            }
        }
    }

    void WeakDisconnect() {
        assert(weak_count_ > 0);
        if (--weak_count_ == 0 && strong_count_ == 0) {
            Delete();
        }
    }

    bool TrySharedConnect() {
        if (strong_count_ == 0) {
            return false;
        }
        ++strong_count_;
        return true;
    }

    void SharedConnect() {
        ++strong_count_;
    }

    void WeakConnect() {
        ++weak_count_;
    }

    size_t SharedCount() const {
        return strong_count_;
    }

    size_t WeakCount() const {
        return weak_count_;
    }

    virtual void Destroy() = 0;
    virtual void* GetPtrMakeShared() = 0;

private:
    size_t strong_count_, weak_count_;

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
    PtrControlBlock() : data_(nullptr) {
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

};  // namespace ptr_detail

class EnableSharedFromThisBase {};

template <typename T>
class EnableSharedFromThis;

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
    explicit SharedPtr(Y* ptr) : ptr_(ptr), data_(new ptr_detail::PtrControlBlock<Y>(ptr)) {
        if constexpr (std::is_convertible_v<Y*, EnableSharedFromThisBase*>) {
            InitWeakThis(ptr);
        }
    }

    template <typename Y>
    SharedPtr(const SharedPtr<Y>& other) : ptr_(other.GetPtr()), data_(other.GetData()) {
        if (GetData()) {
            GetData()->SharedConnect();
        }
    }

    SharedPtr(const SharedPtr& other) : ptr_(other.GetPtr()), data_(other.GetData()) {
        if (GetData()) {
            GetData()->SharedConnect();
        }
    }

    template <typename Y>
    SharedPtr(SharedPtr<Y>&& other) : ptr_(other.GetPtr()), data_(other.GetData()) {
        other.ptr_ = nullptr;
        other.data_ = nullptr;
    }

    // Aliasing constructor
    // #8 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    template <typename Y>
    SharedPtr(const SharedPtr<Y>& other, T* ptr) : ptr_(ptr), data_(other.GetData()) {
        if (GetData()) {
            GetData()->SharedConnect();
        }
    }

    // Promote `WeakPtr`
    // #11 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    explicit SharedPtr(const WeakPtr<T>& other) : SharedPtr(other.Lock()) {
        if (!GetData()) {
            throw BadWeakPtr{};
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    SharedPtr& operator=(const SharedPtr& other) {
        if (this != &other) {
            Reset();
            GetPtr() = other.GetPtr();
            GetData() = other.GetData();
            if (GetData()) {
                GetData()->SharedConnect();
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
        if (GetData()) {
            GetPtr() = nullptr;
            GetData()->SharedDisconnect();
            GetData() = nullptr;
        }
    }

    template <typename Y>
    void Reset(Y* ptr) {
        if (GetData()) {
            GetData()->SharedDisconnect();
        }
        GetPtr() = ptr;
        GetData() = ptr ? new ptr_detail::PtrControlBlock<Y>(ptr) : nullptr;
    }

    void Swap(SharedPtr& other) {
        std::swap(ptr_, other.ptr_);
        std::swap(data_, other.data_);
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
        return !GetData() ? 0 : GetData()->SharedCount();
    }

    explicit operator bool() const {
        return GetPtr() != nullptr;
    }

    template <typename Y>
    bool operator==(const SharedPtr<Y>& other) const {
        return GetPtr() == other.GetPtr() && GetData() == other.GetData();
    }

private:
    T* ptr_;
    ptr_detail::ControlBlock* data_;

    T*& GetPtr() {
        return ptr_;
    }

    ptr_detail::ControlBlock*& GetData() {
        return data_;
    }

    T* GetPtr() const {
        return ptr_;
    }

    ptr_detail::ControlBlock* GetData() const {
        return data_;
    }

    SharedPtr(T* ptr, ptr_detail::ControlBlock* data) : ptr_(ptr), data_(data) {
    }

    template <typename U, typename... A>
    friend SharedPtr<U> MakeShared(A&&...);

    template <typename Y>
    void InitWeakThis(EnableSharedFromThis<Y>* e) {
        e->weak_this = *this;
    }

    template <typename U>
    friend class SharedPtr;

    template <typename U>
    friend class WeakPtr;
};

// Allocate memory only once
template <typename T, typename... Args>
SharedPtr<T> MakeShared(Args&&... args) {
    auto* data = new ptr_detail::MakeSharedControlBlock<T>(std::forward<Args>(args)...);
    auto* ptr = static_cast<T*>(data->GetPtrMakeShared());
    SharedPtr<T> shared(ptr, data);
    if constexpr (std::is_convertible_v<T*, EnableSharedFromThisBase*>) {
        shared.InitWeakThis(ptr);
    }
    return shared;
}

template <typename T>
class EnableSharedFromThis : public EnableSharedFromThisBase {
public:
    SharedPtr<T> SharedFromThis() {
        return weak_this.Lock();
    }

    SharedPtr<const T> SharedFromThis() const {
        return weak_this.Lock();
    }

    WeakPtr<T> WeakFromThis() noexcept {
        return weak_this;
    }

    WeakPtr<const T> WeakFromThis() const noexcept {
        return weak_this;
    }

protected:
    WeakPtr<T> weak_this;

    template <typename Y>
    friend class SharedPtr;
};
