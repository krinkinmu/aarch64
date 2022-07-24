#ifndef __UTIL_INTRUSIVE_LIST_H__
#define __UTIL_INTRUSIVE_LIST_H__

namespace util {

struct Link {
    Link *next = nullptr;
    Link *prev = nullptr;
};


class IntrusiveListIteratorBase {
public:
    IntrusiveListIteratorBase();
    explicit IntrusiveListIteratorBase(Link* pos);

    IntrusiveListIteratorBase(const IntrusiveListIteratorBase& other) = default;
    IntrusiveListIteratorBase(IntrusiveListIteratorBase&& other) = default;
    IntrusiveListIteratorBase& operator=(
        const IntrusiveListIteratorBase& other) = default;
    IntrusiveListIteratorBase& operator=(
        IntrusiveListIteratorBase&& other) = default;

    bool Next();
    bool Prev();
    Link* Item();
    const Link* ConstItem() const;

private:
    Link* pos_;
};

bool operator==(
    const IntrusiveListIteratorBase& l, const IntrusiveListIteratorBase& r);
bool operator!=(
    const IntrusiveListIteratorBase& l, const IntrusiveListIteratorBase& r);


class ConstIntrusiveListIteratorBase {
public:
    ConstIntrusiveListIteratorBase();
    explicit ConstIntrusiveListIteratorBase(const Link* pos);
    explicit ConstIntrusiveListIteratorBase(
        const IntrusiveListIteratorBase& it);

    ConstIntrusiveListIteratorBase(
        const ConstIntrusiveListIteratorBase& other) = default;
    ConstIntrusiveListIteratorBase(
        ConstIntrusiveListIteratorBase&& other) = default;
    ConstIntrusiveListIteratorBase& operator=(
        const ConstIntrusiveListIteratorBase& other) = default;
    ConstIntrusiveListIteratorBase& operator=(
        ConstIntrusiveListIteratorBase&& other) = default;

    bool Next();
    bool Prev();
    const Link* ConstItem() const;

private:
    const Link* pos_;
};

bool operator==(
    const ConstIntrusiveListIteratorBase& l,
    const ConstIntrusiveListIteratorBase& r);
bool operator!=(
    const ConstIntrusiveListIteratorBase& l,
    const ConstIntrusiveListIteratorBase& r);


class IntrusiveListBase {
public:
    IntrusiveListBase();

    IntrusiveListBase(const IntrusiveListBase& other) = delete;
    IntrusiveListBase& operator=(const IntrusiveListBase& other) = delete;

    IntrusiveListBase(IntrusiveListBase&& other);
    IntrusiveListBase& operator=(IntrusiveListBase&& other);

    bool Empty() const;

    IntrusiveListIteratorBase Begin();
    IntrusiveListIteratorBase End();
    ConstIntrusiveListIteratorBase ConstBegin() const;
    ConstIntrusiveListIteratorBase ConstEnd() const;

    IntrusiveListIteratorBase LinkAt(IntrusiveListIteratorBase pos, Link* node);
    ConstIntrusiveListIteratorBase LinkAt(
        ConstIntrusiveListIteratorBase pos, Link* node);

    IntrusiveListIteratorBase Unlink(IntrusiveListIteratorBase pos);
    ConstIntrusiveListIteratorBase Unlink(ConstIntrusiveListIteratorBase pos);

    void Clear();
    void Splice(ConstIntrusiveListIteratorBase pos, IntrusiveListBase& other);
    void Splice(ConstIntrusiveListIteratorBase pos, IntrusiveListBase&& other);

    void Swap(IntrusiveListBase& other);

    bool PopBack();
    bool PopFront();

private:
    Link head_;
};


template <typename T>
struct ListNode : private Link {
    struct Link* Link() { return static_cast<struct Link*>(this); }
    const struct Link* Link() const
    { return static_cast<const struct Link*>(this); }

    static T* FromLink(struct Link* link)
    { return static_cast<T*>(link); }
    static const T* FromLink(const struct Link* link)
    { return static_cast<const T*>(link); }
};

template <typename T> class IntrusiveList;

template <typename T>
class IntrusiveListIterator : public IntrusiveListIteratorBase {
public:
    IntrusiveListIterator() : IntrusiveListIteratorBase() {}

    IntrusiveListIterator(const IntrusiveListIterator& other) = default;
    IntrusiveListIterator(IntrusiveListIterator&& other) = default;
    IntrusiveListIterator& operator=(
        const IntrusiveListIterator& other) = default;
    IntrusiveListIterator& operator=(IntrusiveListIterator&& other) = default;

    T* operator->() { return ListNode<T>::FromLink(Item()); }
    T& operator*() { return *ListNode<T>::FromLink(Item()); }

    IntrusiveListIterator& operator++() {
        Next();
        return *this;
    }

    IntrusiveListIterator operator++(int) {
        IntrusiveListIterator prev(*this);
        Next();
        return prev;
    }

    IntrusiveListIterator& operator--() {
        Prev();
        return *this;
    }

    IntrusiveListIterator operator--(int) {
        IntrusiveListIterator next(*this);
        Prev();
        return next;
    }

private:
    explicit IntrusiveListIterator(Link* node)
        : IntrusiveListIteratorBase(node) {}

    friend class IntrusiveList<T>;
    friend bool operator==(
        const IntrusiveListIterator<T>& l, const IntrusiveListIterator<T>& r);
};

template <typename T>
bool operator==(
        const IntrusiveListIterator<T>& l, const IntrusiveListIterator<T>& r) {
    return static_cast<const IntrusiveListIteratorBase&>(l) ==
            static_cast<const IntrusiveListIteratorBase&>(r);
}

template <typename T>
bool operator!=(
        const IntrusiveListIterator<T>& l, const IntrusiveListIterator<T>& r) {
    return !(l == r);
}


template <typename T>
class ConstIntrusiveListIterator : public ConstIntrusiveListIteratorBase {
public:
    ConstIntrusiveListIterator() : ConstIntrusiveListIteratorBase() {}
    explicit ConstIntrusiveListIterator(const IntrusiveListIterator<T>& it)
        : ConstIntrusiveListIteratorBase(it) {}

    ConstIntrusiveListIterator(
        const ConstIntrusiveListIterator& other) = default;
    ConstIntrusiveListIterator(ConstIntrusiveListIterator&& other) = default;
    ConstIntrusiveListIterator& operator=(
        const ConstIntrusiveListIterator& other) = default;
    ConstIntrusiveListIterator& operator=(
        ConstIntrusiveListIterator&& other) = default;

    const T* operator->() const { return ListNode<T>::FromLink(ConstItem()); }
    const T& operator*() const { return *ListNode<T>::FromLink(ConstItem()); }

    ConstIntrusiveListIterator& operator++() {
        Next();
        return *this;
    }

    ConstIntrusiveListIterator operator++(int) {
        ConstIntrusiveListIterator prev(*this);
        Next();
        return prev;
    }

    ConstIntrusiveListIterator& operator--() {
        Prev();
        return *this;
    }

    ConstIntrusiveListIterator operator--(int) {
        ConstIntrusiveListIterator next(*this);
        Prev();
        return next;
    }

private:
    explicit ConstIntrusiveListIterator(const Link* node)
        : ConstIntrusiveListIteratorBase(node) {}

    friend class IntrusiveList<T>;
    friend bool operator==(
        const ConstIntrusiveListIterator<T>& l,
        const ConstIntrusiveListIterator<T>& r);
};

template <typename T>
bool operator==(
        const ConstIntrusiveListIterator<T>& l,
        const ConstIntrusiveListIterator<T>& r) {
    return static_cast<const ConstIntrusiveListIteratorBase&>(l) ==
            static_cast<const ConstIntrusiveListIteratorBase&>(r);
}

template <typename T>
bool operator!=(
        const ConstIntrusiveListIterator<T>& l,
        const ConstIntrusiveListIterator<T>& r) {
    return !(l == r);
}


template <typename T>
class IntrusiveList : private IntrusiveListBase {
public:
    IntrusiveList() : IntrusiveListBase() {}

    IntrusiveList(const IntrusiveList& other) = delete;
    IntrusiveList& operator=(const IntrusiveList& other) = delete;

    IntrusiveList(IntrusiveList&& other) : IntrusiveListBase(other) {}
    IntrusiveList& operator=(IntrusiveList&& other) {
        IntrusiveListBase::operator=(other);
        return *this;
    }

    using IntrusiveListBase::Empty;
    using IntrusiveListBase::Clear;
    using IntrusiveListBase::PopBack;
    using IntrusiveListBase::PopFront;

    IntrusiveListIterator<T> Begin() {
        return IntrusiveListIterator<T>(IntrusiveListBase::Begin().Item());
    }

    IntrusiveListIterator<T> End() {
        return IntrusiveListIterator<T>(IntrusiveListBase::End().Item());
    }

    ConstIntrusiveListIterator<T> ConstBegin() const {
        return IntrusiveListIterator<T>(
            IntrusiveListBase::ConstBegin().ConstItem());
    }

    ConstIntrusiveListIterator<T> ConstEnd() const {
        return IntrusiveListIterator<T>(
            IntrusiveListBase::ConstEnd().ConstItem());
    }

    IntrusiveListIterator<T> LinkAt(
        IntrusiveListIterator<T> pos, ListNode<T>* node) {
        return IntrusiveListIterator<T>(
            IntrusiveListBase::LinkAt(pos, node->Link()).Item());
    }

    ConstIntrusiveListIterator<T> LinkAt(
        ConstIntrusiveListIterator<T> pos, ListNode<T>* node) {
        return ConstIntrusiveListIterator<T>(
            IntrusiveListBase::LinkAt(pos, node->Link()).ConstItem());
    }

    IntrusiveListIterator<T> Unlink(IntrusiveListIterator<T> pos) {
        return IntrusiveListIterator<T>(IntrusiveListBase::Unlink(pos).Item());
    }

    ConstIntrusiveListIterator<T> Unlink(ConstIntrusiveListIterator<T> pos) {
        return ConstIntrusiveListIterator<T>(
            IntrusiveListBase::Unlink(pos).ConstItem());
    }

    void Unlink(ListNode<T>* node) {
        Unlink(IntrusiveListIterator<T>(node->Link()));
    }

    void Unlink(const ListNode<T>* node) {
        Unlink(ConstIntrusiveListIterator<T>(node->Link()));
    }

    void Splice(ConstIntrusiveListIterator<T> pos, IntrusiveList& other) {
        IntrusiveListBase::Splice(pos, other);
    }

    void Splice(ConstIntrusiveListIterator<T> pos, IntrusiveList&& other) {
        IntrusiveListBase::Splice(pos, Move(other));
    }

    void Swap(IntrusiveList& other) {
        IntrusiveListBase::Swap(other);
    }
};

}  // namespace util

#endif  // __UTIL_INTRUSIVE_LIST_H__
