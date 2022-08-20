#ifndef __COMMON_INTRUSIVE_LIST_H__
#define __COMMON_INTRUSIVE_LIST_H__

#include <utility>


namespace common {

namespace impl {

struct Link {
    Link *next = nullptr;
    Link *prev = nullptr;
};


class Iterator {
public:
    Iterator();
    explicit Iterator(Link* pos);

    Iterator(const Iterator&) = default;
    Iterator(Iterator&&) = default;
    Iterator& operator=(const Iterator&) = default;
    Iterator& operator=(Iterator&&) = default;

    void Next();
    void Prev();
    Link* Item();
    const Link* ConstItem() const;

private:
    Link* pos_;
};


bool operator==(const Iterator& l, const Iterator& r);
bool operator!=(const Iterator& l, const Iterator& r);


class List {
public:
    List();

    List(const List&) = delete;
    List& operator=(const List&) = delete;

    List(List&& other);
    List& operator=(List&& other);

    Link* Begin();
    const Link* ConstBegin() const;

    Link* End();
    const Link* ConstEnd() const;

    bool Empty() const;
    void Clear();
    void Swap(List& other);
    void Splice(Link *pos, List& other);
    void Splice(Link *pos, List&& other);

    Link* LinkAt(Link *pos, Link* node);
    Link* Unlink(Link *pos);

    Link* Front();
    Link* Back();

    Link* PopBack();
    Link* PopFront();
    void PushBack(Link* node);
    void PushFront(Link* node);

private:
    Link head_;
};

}  // namespace impl


template <typename T>
struct ListNode : private impl::Link {
    impl::Link* Link() { return static_cast<impl::Link*>(this); }
    const impl::Link* Link() const
    { return static_cast<const impl::Link*>(this); }

    static T* FromLink(impl::Link* link)
    { return static_cast<T*>(link); }
    static const T* FromLink(const impl::Link* link)
    { return static_cast<const T*>(link); }
};


template <typename T> struct IntrusiveList;

template <typename T>
struct IntrusiveListIterator : private impl::Iterator {
    IntrusiveListIterator() = default;
    explicit IntrusiveListIterator(impl::Link* pos) : impl::Iterator(pos) {}

    IntrusiveListIterator(const IntrusiveListIterator&) = default;
    IntrusiveListIterator(IntrusiveListIterator&&) = default;
    IntrusiveListIterator& operator=(const IntrusiveListIterator&) = default;
    IntrusiveListIterator& operator=(IntrusiveListIterator&&) = default;

    T* Item() { return ListNode<T>::FromLink(impl::Iterator::Item()); }
    const T* ConstItem() const
    { return ListNode<T>::FromLink(impl::Iterator::ConstItem()); }

    T* operator->() { return Item(); }
    T& operator*() { return *Item(); }

    IntrusiveListIterator operator++(int) {
        IntrusiveListIterator prev(*this);
        Next();
        return prev;
    }

    IntrusiveListIterator& operator++() {
        Next();
        return *this;
    }

    IntrusiveListIterator operator--(int) {
        IntrusiveListIterator next(*this);
        Prev();
        return next;
    }

    IntrusiveListIterator& operator--() {
        Prev();
        return *this;
    }

    friend struct IntrusiveList<T>;
    friend bool operator==(
        const IntrusiveListIterator& l, const IntrusiveListIterator& r);
};


template <typename T>
struct IntrusiveListConstIterator : private impl::Iterator {
    IntrusiveListConstIterator() = default;
    explicit IntrusiveListConstIterator(const impl::Link* pos)
        : impl::Iterator(const_cast<impl::Link*>(pos))
    {}

    IntrusiveListConstIterator(const IntrusiveListConstIterator&) = default;
    IntrusiveListConstIterator(IntrusiveListConstIterator&&) = default;
    IntrusiveListConstIterator& operator=(
        const IntrusiveListConstIterator&) = default;
    IntrusiveListConstIterator& operator=(
        IntrusiveListConstIterator&&) = default;

    const T* ConstItem() const
    { return ListNode<T>::FromLink(impl::Iterator::ConstItem()); }
    const T* operator->() { return ConstItem(); }
    const T& operator*() { return *ConstItem(); }

    IntrusiveListConstIterator operator++(int) {
        IntrusiveListConstIterator prev(*this);
        Next();
        return prev;
    }

    IntrusiveListConstIterator& operator++() {
        Next();
        return *this;
    }

    IntrusiveListConstIterator operator--(int) {
        IntrusiveListConstIterator next(*this);
        Prev();
        return next;
    }

    IntrusiveListConstIterator& operator--() {
        Prev();
        return *this;
    }

    friend struct IntrusiveList<T>;
    friend bool operator==(
        const IntrusiveListConstIterator& l,
        const IntrusiveListConstIterator& r);
};


template <typename T>
bool operator==(
    const IntrusiveListIterator<T>& l, const IntrusiveListIterator<T>& r) {
    return static_cast<impl::Iterator>(l) == static_cast<impl::Iterator>(r);
}

template <typename T>
bool operator!=(
    const IntrusiveListIterator<T>& l, const IntrusiveListIterator<T>& r) {
    return !(l == r);
}

template <typename T>
bool operator==(
    const IntrusiveListConstIterator<T>& l,
    const IntrusiveListConstIterator<T>& r) {
    return static_cast<impl::Iterator>(l) == static_cast<impl::Iterator>(r);
}

template <typename T>
bool operator!=(
    const IntrusiveListConstIterator<T>& l,
    const IntrusiveListConstIterator<T>& r) {
    return !(l == r);
}


template <typename T>
struct IntrusiveList : private impl::List {
    using iterator = IntrusiveListIterator<T>;
    using const_iterator = IntrusiveListConstIterator<T>;

    IntrusiveList() = default;

    IntrusiveList(const IntrusiveList&) = delete;
    IntrusiveList& operator=(const IntrusiveList&) = delete;

    IntrusiveList(IntrusiveList&& other) = default;
    IntrusiveList& operator=(IntrusiveList&& other) = default;

    using impl::List::Empty;
    using impl::List::Clear;

    IntrusiveListIterator<T> Begin() {
        return IntrusiveListIterator<T>(impl::List::Begin());
    }

    IntrusiveListIterator<T> End() {
        return IntrusiveListIterator<T>(impl::List::End());
    }

    IntrusiveListConstIterator<T> ConstBegin() const {
        return IntrusiveListConstIterator<T>(impl::List::ConstBegin());
    }

    IntrusiveListConstIterator<T> ConstEnd() const {
        return IntrusiveListConstIterator<T>(impl::List::ConstEnd());
    }

    IntrusiveListIterator<T> LinkAt(IntrusiveListIterator<T> pos, T* node) {
        impl::Link* link = static_cast<ListNode<T>*>(node)->Link();
        return IntrusiveListIterator<T>(
            impl::List::LinkAt(pos.Item()->Link(), link));
    }

    IntrusiveListIterator<T> Unlink(IntrusiveListIterator<T> pos) {
        return IntrusiveListIterator<T>(impl::List::Unlink(pos.Item()->Link()));
    }

    void Unlink(T* node) {
        impl::Link* link = static_cast<ListNode<T>*>(node)->Link();
        Unlink(IntrusiveListIterator<T>(link));
    }

    void Splice(IntrusiveListIterator<T> pos, IntrusiveList& other) {
        impl::List::Splice(pos.Item()->Link(), other);
    }

    void Splice(IntrusiveListIterator<T> pos, IntrusiveList&& other) {
        impl::List::Splice(pos.Item()->Link(), std::move(other));
    }

    void Swap(IntrusiveList& other) {
        impl::List::Swap(other);
    }

    T* Front() {
        impl::Link* link = impl::List::Front();
        if (link) {
            return ListNode<T>::FromLink(link);
        }
        return nullptr;
    }

    T* Back() {
        impl::Link* link = impl::List::Back();
        if (link) {
            return ListNode<T>::FromLink(link);
        }
        return nullptr;
    }

    T* PopFront() {
        impl::Link* link = impl::List::PopFront();
        if (link) {
            return ListNode<T>::FromLink(link);
        }
        return nullptr;
    }

    T* PopBack() {
        impl::Link* link = impl::List::PopBack();
        if (link) {
            return ListNode<T>::FromLink(link);
        }
        return nullptr;
    }

    void PushBack(T* node) {
        impl::Link* link = static_cast<ListNode<T>*>(node)->Link();
        impl::List::PushBack(link);
    } 

    void PushFront(T* node) {
        impl::Link* link = static_cast<ListNode<T>*>(node)->Link();
        impl::List::PushFront(link);
    }
};

}  // namespace common

#endif  // __COMMON_INTRUSIVE_LIST_H__
