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
            //std::cout << _cache << std::endl;
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
    auto size_difference = _cache.GetHead()->GetValueSize() - value.size();
    // std::cout << typeid(size_difference).name() << std::endl;

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
        tmp = tmp->_next;
        delete previous;
    }
}

Entry *CacheList::GetHead() { return _head; }

Entry *CacheList::GetTail() { return _tail; }

void CacheList::AddToHead(Entry *entry) {
    if (_head != nullptr) {
        entry->_next = _head;
        _head->_previous = entry;
        _head = entry;
    } else {
        entry->_next = _head;
        _head = entry;
        _tail = entry;
    }
}

void CacheList::Exclude(Entry *entry) {
    Entry *next = entry->_next;
    Entry *previous = entry->_previous;

    if (entry != _tail) {
        next->_previous = previous;
    } else {
        _tail = previous;
    }
    if (entry != _head) {
        previous->_next = next;
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
