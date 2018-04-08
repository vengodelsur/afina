#include "MapBasedGlobalLockImpl.h"

#include <mutex>

namespace Afina {
namespace Backend {

// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::Put(const std::string &key,
                                 const std::string &value) {
    std::unique_lock<std::mutex> lock(_mutex);

    // Entry entry(key, value);
    size_t entry_size = key.size() + value.size();
    // size_t entry_size = entry.size();

    if (entry_size > _max_size) {
        return false;
    }

    auto iterator = _backend.find(key);

    if (iterator != _backend.end()) {
        _cache.splice(_cache.begin(), _cache, iterator->second);
        return set_head_value(key, value);
    } else {
        return add_entry(key, value);
    }
}

// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::PutIfAbsent(const std::string &key,
                                         const std::string &value) {
    std::unique_lock<std::mutex> lock(_mutex);

    Entry entry(key, value);

    if (_backend.find(key) != _backend.end()) {
        return false;
    }

    size_t entry_size = entry.size();

    if (entry_size > _max_size) {
        return false;
    }

    return add_entry(key, value);

    return true;
}
// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::Set(const std::string &key,
                                 const std::string &value) {
    std::unique_lock<std::mutex> lock(_mutex);  // shared?

    if (key.size() + value.size() > _max_size) {
        return false;
    }

    auto iterator = _backend.find(key);

    if (iterator == _backend.end()) {
        return false;
    } else {
        _cache.splice(_cache.begin(), _cache, iterator->second);
        return set_head_value(key, value);
    }
    return true;
}

// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::Delete(const std::string &key) {
    std::unique_lock<std::mutex> lock(_mutex);

    auto iterator = _backend.find(key);

    if (iterator == _backend.end()) {
        return true;
    } else {
        // move existing key to tail and delete
        _cache.splice(--_cache.end(), _cache, iterator->second);
        delete_last();
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

    _cache.splice(_cache.begin(), _cache, iterator->second);
    _backend.emplace(key, _cache.cbegin());
    value = _cache.front().get_value();

    return true;
}
bool MapBasedGlobalLockImpl::add_entry(const std::string &key,
                                       const std::string &value) {
    Entry entry(key, value);
    size_t entry_size = entry.size();
    while (entry_size + _current_size > _max_size) {
        delete_last();
    }

    _cache.emplace_front(entry);
    //_backend.emplace(_cache.front().first, _cache.cbegin());
    _backend.emplace(_cache.front().get_key_reference(), _cache.cbegin());
    _current_size += entry_size;

    return true;
}
void MapBasedGlobalLockImpl::delete_last() {
    auto tail = _cache.back();
    size_t tail_size = tail.size();
    _backend.erase(tail.get_key_reference());
    _cache.pop_back();
    _current_size -= tail_size;
}

bool MapBasedGlobalLockImpl::set_head_value(const std::string &key,
                                            const std::string &value) {
    auto size_difference = _cache.front().get_value_size() - value.size();
    while (size_difference + _current_size > _max_size) {
        delete_last();
    }
    _cache.front().set_value(value);
    _current_size += size_difference;
    return true;
}

//
}  // namespace Backend
}  // namespace Afina
