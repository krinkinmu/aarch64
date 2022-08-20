#include "common/intrusive_list.h"


namespace common {

namespace impl {

Iterator::Iterator() : pos_(nullptr) {}

Iterator::Iterator(Link* pos) : pos_(pos) {}

void Iterator::Next() { if (pos_) pos_ = pos_->next; }

void Iterator::Prev() { if (pos_) pos_ = pos_->prev; }

Link* Iterator::Item() { return pos_; }

const Link* Iterator::ConstItem() const { return pos_; }


bool operator==(const Iterator& l, const Iterator& r) {
    return l.ConstItem() == r.ConstItem();
}

bool operator!=(const Iterator& l, const Iterator& r) {
    return !(l == r);
}


List::List() { Clear(); }

List::List(List&& other) {
    Clear();
    Splice(Begin(), other);
}

List& List::operator=(List&& other) {
    if (this == &other) {
        return *this;
    }

    Clear();
    Splice(Begin(), other);
    return *this;
}

Link* List::Begin() { return head_.next; }

const Link* List::ConstBegin() const { return head_.next; }

Link* List::End() { return &head_; }

const Link* List::ConstEnd() const { return &head_; }

bool List::Empty() const { return ConstBegin() == ConstEnd(); }

void List::Clear() {
    head_.next = &head_;
    head_.prev = &head_;
}

void List::Swap(List& other) {
    List tmp = std::move(other);
    other = std::move(*this);
    *this = std::move(tmp);
}

void List::Splice(Link *pos, List& other) {
    if (other.Empty()) {
        return;
    }

    Link* first = other.Begin();
    Link* last = other.End()->prev;
    other.Clear();

    Link* prev = pos->prev;
    Link* next = pos;

    first->prev = prev;
    last->next = next;
    prev->next = first;
    next->prev = last;
}

void List::Splice(Link *pos, List&& other) {
    Splice(pos, other);
}

Link* List::LinkAt(Link *pos, Link *node) {
    Link* prev = pos->prev;
    Link* next = pos;

    node->next = next;
    node->prev = prev;
    prev->next = node;
    next->prev = node;

    return node;
}

Link* List::Unlink(Link *pos) {
    Link* prev = pos->prev;
    Link* next = pos->next;

    prev->next = next;
    next->prev = prev;
    pos->prev = nullptr;
    pos->next = nullptr;

    return next;
}

Link* List::Front() {
    if (Empty()) {
        return nullptr;
    }
    return Begin();
}

Link* List::Back() {
    if (Empty()) {
        return nullptr;
    }
    return Back()->prev;
}

Link* List::PopFront() {
    if (Empty()) {
        return nullptr;
    }

    Link* ptr = Begin();
    Unlink(ptr);
    return ptr;
}

Link* List::PopBack() {
    if (Empty()) {
        return nullptr;
    }

    Link* ptr = End()->prev;
    Unlink(ptr);
    return ptr;
}

void List::PushBack(Link* link) {
    LinkAt(End(), link);
}

void List::PushFront(Link* link) {
    LinkAt(Begin(), link);
}

}  // namespace impl

}  // namespace common
