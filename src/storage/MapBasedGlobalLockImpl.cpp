#include "MapBasedGlobalLockImpl.h"

namespace Afina {
namespace Backend {

// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::Put(const std::string &key,
                                 const std::string &value) {
    std::unique_lock<std::mutex> lock(_mutex);

    if (CheckSize(key, value)) {
        auto iterator = _backend.find(key);
        // std::cout << typeid(iterator).name() << std::endl;
        if (iterator != _backend.end()) {
            _cache.MoveToHead(&iterator->second);
            return SetHeadValue(key, value);
        } else {
            // std::cout << _cache << std::endl;
            return AddEntry(key, value);
        }
    }
    return false;
}

// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::PutIfAbsent(const std::string &key,
                                         const std::string &value) {
    std::unique_lock<std::mutex> lock(_mutex);

    if (CheckSize(key, value)) {
        if (_backend.find(key) != _backend.end()) {
            return false;
        }

        return AddEntry(key, value);
    }
    return false;
}
// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::Set(const std::string &key,
                                 const std::string &value) {
    std::unique_lock<std::mutex> lock(_mutex);  // shared?

    if (CheckSize(key, value)) {
        auto iterator = _backend.find(key);

        if (iterator == _backend.end()) {
            return false;
        } else {
            _cache.MoveToHead(&iterator->second);
            return SetHeadValue(key, value);
        }
    }
    return false;
}

// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::Delete(const std::string &key) {
    std::unique_lock<std::mutex> lock(_mutex);

    auto iterator = _backend.find(key);

    if (iterator == _backend.end()) {
        return true;
    } else {
        _cache.Delete(&iterator->second);
        return true;
    }
    return false;
}

// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::Get(const std::string &key,
                                 std::string &value) const {
    std::unique_lock<std::mutex> lock(_mutex);  // shared?
    auto iterator = _backend.find(key);
    if (iterator == _backend.end()) {
        return false;
    }

    _cache.MoveToHead(&iterator->second);
    Entry *head = _cache.GetHead();
    _backend.emplace(key, std::ref(*head));
    value = _cache.GetHead()->GetValue();

    return true;
}
bool MapBasedGlobalLockImpl::AddEntry(const std::string &key,
                                      const std::string &value) {
    Entry *entry = new Entry(key, value);
    size_t entry_size = entry->Size();
    while (entry_size + _current_size > _max_size) {
        DeleteLast();
    }

    _cache.AddToHead(entry);
    Entry *head = _cache.GetHead();
    _backend.emplace(head->GetKeyReference(), std::ref(*head));
    _current_size += entry_size;

    return true;
}
void MapBasedGlobalLockImpl::DeleteLast() {
    Entry *tail = _cache.GetTail();
    size_t tail_size = tail->Size();
    _backend.erase(tail->GetKeyReference());
    _cache.DeleteTail();
    _current_size -= tail_size;
}

bool MapBasedGlobalLockImpl::SetHeadValue(const std::string &key,
                                          const std::string &value) {
    size_t size_difference = _cache.GetHead()->GetValueSize() - value.size();

    while (size_difference + _current_size > _max_size) {
        DeleteLast();
    }
    _cache.GetHead()->SetValue(value);
    _current_size += size_difference;
    return true;
}

CacheList::~CacheList() {
    Entry *tmp = _head;
    while (tmp != nullptr) {
        Entry *previous = tmp;
        tmp = tmp->GetNext();
        delete previous;
    }
}

Entry *CacheList::GetHead() { return _head; }

Entry *CacheList::GetTail() { return _tail; }

void CacheList::AddToHead(Entry *entry) {
    if (_head != nullptr) {
        entry->SetNext(_head);
        _head->SetPrevious(entry);
        _head = entry;
    } else {
        entry->SetNext(_head);
        _head = entry;
        _tail = entry;
    }
}

void CacheList::Exclude(Entry *entry) {
    Entry *next = entry->GetNext();
    Entry *previous = entry->GetPrevious();

    if (entry != _tail) {
        next->SetPrevious(previous);
    } else {
        _tail = previous;
    }
    if (entry != _head) {
        previous->SetNext(next);
    } else {
        _head = next;
    }
}
void CacheList::Delete(Entry *entry) {
    Exclude(entry);
    delete entry;
}

void CacheList::MoveToHead(Entry *entry) {
    if (entry != _head) {
        Exclude(entry);
        AddToHead(entry);
    }
}

void CacheList::DeleteTail() { Delete(_tail); }

//
}  // namespace Backend
}  // namespace Afina
