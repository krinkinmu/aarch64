#include "util/intrusive_list.h"
#include "util/utility.h"

namespace util {

namespace {

void LinkBefore(Link* pos, Link* node) {
    Link* prev = pos->prev;
    Link* next = pos;

    node->next = next;
    node->prev = prev;
    prev->next = node;
    next->prev = node;
}

void SpliceBefore(Link* pos, Link* first, Link* last) {
    // Remove from the old list
    Link* prev = first->prev;
    Link* next = last->next;
    prev->next = next;
    next->prev = prev;

    // Add to the new list
    prev = pos->prev;
    next = pos;
    first->prev = prev;
    prev->next = first;
    last->next = next;
    next->prev = last;
}

void Unlink(Link* node) {
    Link* prev = node->prev;
    Link* next = node->next;

    prev->next = next;
    next->prev = prev;
    node->prev = nullptr;
    node->next = nullptr;
}

}  // namespace

IntrusiveListIteratorBase::IntrusiveListIteratorBase() : pos_(nullptr) {}

IntrusiveListIteratorBase::IntrusiveListIteratorBase(Link* pos) : pos_(pos) {}

bool IntrusiveListIteratorBase::Next() {
    if (pos_ == nullptr) {
        return false;
    }
    pos_ = pos_->next;
    return true;
}

bool IntrusiveListIteratorBase::Prev() {
    if (pos_ == nullptr) {
        return false;
    }
    pos_ = pos_->prev;
    return true;
}

Link* IntrusiveListIteratorBase::Item() {
    return pos_;
}

const Link* IntrusiveListIteratorBase::ConstItem() const {
    return pos_;
}

bool operator==(
        const IntrusiveListIteratorBase& l,
        const IntrusiveListIteratorBase& r) {
    return l.ConstItem() == r.ConstItem();
}

bool operator!=(
        const IntrusiveListIteratorBase& l,
        const IntrusiveListIteratorBase& r) {
    return !(l == r);
}


ConstIntrusiveListIteratorBase::ConstIntrusiveListIteratorBase()
        : pos_(nullptr) {}

ConstIntrusiveListIteratorBase::ConstIntrusiveListIteratorBase(const Link* pos)
        : pos_(pos) {}

ConstIntrusiveListIteratorBase::ConstIntrusiveListIteratorBase(
        const IntrusiveListIteratorBase& it) : pos_(it.ConstItem()) {}

bool ConstIntrusiveListIteratorBase::Next() {
    if (pos_ == nullptr) {
        return false;
    }
    pos_ = pos_->next;
    return true;
}

bool ConstIntrusiveListIteratorBase::Prev() {
    if (pos_ == nullptr) {
        return false;
    }
    pos_ = pos_->prev;
    return false;
}

const Link* ConstIntrusiveListIteratorBase::ConstItem() const {
    return pos_;
}

bool operator==(
        const ConstIntrusiveListIteratorBase& l,
        const ConstIntrusiveListIteratorBase& r) {
    return l.ConstItem() == r.ConstItem();
}

bool operator!=(
        const ConstIntrusiveListIteratorBase& l,
        const ConstIntrusiveListIteratorBase& r) {
    return !(l == r);
}


IntrusiveListBase::IntrusiveListBase() {
    Clear();
}

IntrusiveListBase::IntrusiveListBase(IntrusiveListBase&& other) {
    Clear();
    Splice(ConstBegin(), other);
}

IntrusiveListBase& IntrusiveListBase::operator=(IntrusiveListBase&& other) {
    if (this == &other) {
        return *this;
    }

    Clear();
    Splice(ConstBegin(), other);
    return *this;
}

bool IntrusiveListBase::Empty() const { return ConstBegin() == ConstEnd(); }

IntrusiveListIteratorBase IntrusiveListBase::Begin() {
    return IntrusiveListIteratorBase(head_.next);
}

IntrusiveListIteratorBase IntrusiveListBase::End() {
    return IntrusiveListIteratorBase(&head_);
}

ConstIntrusiveListIteratorBase IntrusiveListBase::ConstBegin() const {
    return ConstIntrusiveListIteratorBase(head_.next);
}

ConstIntrusiveListIteratorBase IntrusiveListBase::ConstEnd() const {
    return ConstIntrusiveListIteratorBase(&head_);
}

IntrusiveListIteratorBase IntrusiveListBase::LinkAt(
        IntrusiveListIteratorBase pos, Link* node) {
    LinkBefore(pos.Item(), node);
    return IntrusiveListIteratorBase(node);
}

ConstIntrusiveListIteratorBase IntrusiveListBase::LinkAt(
        ConstIntrusiveListIteratorBase pos, Link* node) {
    LinkBefore(const_cast<Link*>(pos.ConstItem()), node);
    return ConstIntrusiveListIteratorBase(node);
}

IntrusiveListIteratorBase IntrusiveListBase::Unlink(
        IntrusiveListIteratorBase pos) {
    IntrusiveListIteratorBase next(pos); next.Next();
    ::util::Unlink(pos.Item());
    return next;
}

ConstIntrusiveListIteratorBase IntrusiveListBase::Unlink(
        ConstIntrusiveListIteratorBase pos) {
    ConstIntrusiveListIteratorBase next(pos); next.Next();
    ::util::Unlink(const_cast<Link*>(pos.ConstItem()));
    return next;
}

void IntrusiveListBase::Clear() {
    head_.next = &head_;
    head_.prev = &head_;
}

void IntrusiveListBase::Splice(
        ConstIntrusiveListIteratorBase pos, IntrusiveListBase& other) {
    if (other.Empty()) {
        return;
    }
    SpliceBefore(
        const_cast<Link*>(pos.ConstItem()), other.head_.next, other.head_.prev);
}

void IntrusiveListBase::Splice(
        ConstIntrusiveListIteratorBase pos, IntrusiveListBase&& other) {
    Splice(pos, other);
}

void IntrusiveListBase::Swap(IntrusiveListBase& other) {
    IntrusiveListBase copy(Move(other));
    other = Move(*this);
    *this = Move(copy);
}

bool IntrusiveListBase::PopBack() {
    if (Empty()) {
        return false;
    }

    IntrusiveListIteratorBase it = End(); it.Prev();
    Unlink(it);
    return true;
}

bool IntrusiveListBase::PopFront() {
    if (Empty()) {
        return false;
    }

    Unlink(Begin());
    return true;
}

}  // namespace util
