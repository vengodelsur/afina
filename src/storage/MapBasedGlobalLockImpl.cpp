#include "MapBasedGlobalLockImpl.h"

#include <mutex>

namespace Afina {
namespace Backend {

// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::Put(const std::string &key, const std::string &value) { 
    std::unique_lock<std::mutex> lock(_mutex);
    size_t entry_size = key.size() + value.size();
    if (entry_size > _max_size) {
        return false;
    }
    while (entry_size + _current_size > _max_size) {
        return false;
    }
    return false; 
}

// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::PutIfAbsent(const std::string &key, const std::string &value) { 
    std::unique_lock<std::mutex> lock(_mutex);
    return false; 
}

// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::Set(const std::string &key, const std::string &value) { 
    std::unique_lock<std::mutex> lock(_mutex); //shared?
    return false; 
}

// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::Delete(const std::string &key) { 
    std::unique_lock<std::mutex> lock(_mutex);
    return false; 
}

// See MapBasedGlobalLockImpl.h
bool MapBasedGlobalLockImpl::Get(const std::string &key, std::string &value) const { 
    std::unique_lock<std::mutex> lock(_mutex); //shared?
    return false; 
}

} // namespace Backend
} // namespace Afina
