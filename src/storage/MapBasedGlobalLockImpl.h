#ifndef AFINA_STORAGE_MAP_BASED_GLOBAL_LOCK_IMPL_H
#define AFINA_STORAGE_MAP_BASED_GLOBAL_LOCK_IMPL_H

#include <unordered_map>
#include<list>
#include <mutex>
#include <string>

#include <afina/Storage.h>

/*class Entry
{
    std::string key;
    std::string value;
    Entry* next;
    Entry* previous;
};*/

namespace Afina {
namespace Backend {

/**
 * # Map based implementation with global lock
 *
 *
 */
class MapBasedGlobalLockImpl : public Afina::Storage {
public:
    MapBasedGlobalLockImpl(size_t max_size = 1024) : _max_size(max_size) {}
    ~MapBasedGlobalLockImpl() {}

    // Implements Afina::Storage interface
    bool Put(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool PutIfAbsent(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool Set(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool Delete(const std::string &key) override;

    // Implements Afina::Storage interface
    bool Get(const std::string &key, std::string &value) const override;

private:

    using key = std::string;
    using value = std::string;    
    using entry = std::pair<const key, value>;
    
    size_t _max_size;
    size_t _current_size;

    mutable std::unordered_map<std::reference_wrapper<const key>, std::list<entry>::const_iterator, std::hash<key>, std::equal_to<key>> _backend;
    mutable std::list<entry> _cache;

    mutable std::mutex _mutex;
    
};

} // namespace Backend
} // namespace Afina

#endif // AFINA_STORAGE_MAP_BASED_GLOBAL_LOCK_IMPL_H
