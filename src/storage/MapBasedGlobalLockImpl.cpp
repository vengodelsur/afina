#include "MapBasedGlobalLockImpl.h"

#include <mutex>

namespace Afina {
namespace Backend {

// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::Put(const std::string &key,
                                 const std::string &value) {
    std::unique_lock<std::mutex> lock(_mutex);

    if (CheckSize(key, value)) {
        auto iterator = _backend.find(key);

        if (iterator != _backend.end()) {
            _cache.MoveToHead(&iterator->second);
            return SetHeadValue(key, value);
        } else {
            return AddEntry(key, value);
        }
    } else {
        return false;
    }
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
    } else {
        return false;
    }
    return true;
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
        return true;
    } else {
        return false;
    }
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
    Entry* head = _cache.GetHead();
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
    Entry* head = _cache.GetHead();
    _backend.emplace(_cache.GetHead()->GetKeyReference(), std::ref(*head));
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
    while (size_difference + _current_size > _max_size) {
        DeleteLast();
    }
    _cache.GetHead()->SetValue(value);
    _current_size += size_difference;
    return true;
}

//
}  // namespace Backend
}  // namespace Afina
