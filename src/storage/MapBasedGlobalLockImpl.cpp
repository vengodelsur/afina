#include "MapBasedGlobalLockImpl.h"

#include <mutex>

namespace Afina {
namespace Backend {

// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::Put(const std::string &key,
                                 const std::string &value) {
    std::unique_lock<std::mutex> lock(_mutex);

    Entry entry(key, value);
    // size_t entry_size = key.size() + value.size();
    size_t entry_size = entry.size();

    if (entry_size > _max_size) {
        return false;
    }

    auto iterator = _backend.find(key);

    if (iterator != _backend.end()) {
        // move existing key to tail and delete
        _cache.splice(--_cache.end(), _cache, iterator->second);
        //_cache.pop_back();
        delete_last();
    }

    while (entry_size + _current_size > _max_size) {
         delete_last();   
    }

    _cache.emplace_front(key, value);
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

    while (entry_size + _current_size > _max_size) {
        delete_last();
    }

    
    _cache.emplace_front(entry);
    _backend.emplace(key, _cache.cbegin());
    _current_size += entry_size;

    return true;
}

// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::Set(const std::string &key,
                                 const std::string &value) {
    std::unique_lock<std::mutex> lock(_mutex);  // shared?

    auto iterator = _backend.find(key);

    if (iterator == _backend.end()) {
        return false;
    } else {
        // move existing key to tail and delete
        _cache.splice(_cache.begin(), _cache, iterator->second);
        size_t size_difference = _cache.front().get_value_size() - value.size();
        _cache.front().set_value(value);
        //_current_size += previous_value_size -= ;
    }

    /*size_t entry_size = key.size() + value.size();

    if (entry_size > _max_size) {
        return false;
    }

    while (entry_size + _current_size > _max_size) {
        auto tail = _cache.back();
        size_t tail_size = tail.first.size() + tail.second.size();
        _backend.erase(tail.first);
        _cache.pop_back();
        _current_size -= tail_size;
    }

    auto entry = std::make_pair(key, value);
    _cache.emplace_front(entry);
    _backend.emplace(key, _cache.cbegin());*/

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

}  // namespace Backend
}  // namespace Afina
