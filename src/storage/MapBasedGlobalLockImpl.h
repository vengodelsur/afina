#ifndef AFINA_STORAGE_MAP_BASED_GLOBAL_LOCK_IMPL_H
#define AFINA_STORAGE_MAP_BASED_GLOBAL_LOCK_IMPL_H

#include <list>
#include <mutex>
#include <string>
#include <unordered_map>

#include <afina/Storage.h>

/*class List {
   public:
    List() : head(NULL), tail(NULL) {}

   private:


   private:
    Entry *head;
    Entry *tail;
};*/

namespace Afina {
namespace Backend {

class Entry {
   public:
    Entry() : _key(NULL), _value(NULL) {}
    Entry(const std::string &key, const std::string &value)
        : _key(key), _value(value) {}

    size_t size() const { return _key.size() + _value.size(); }
    size_t get_value_size() const { return _value.size(); }
    void set_value(const std::string &value) const { _value = value; }
    const std::string &get_key_reference() const { return _key; }
    std::string get_value() const { return _value; }

   private:
    const std::string _key;
    mutable std::string _value;
    // Entry *next;
    // Entry *previous;
};

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
    bool set_head_value(const std::string &key, const std::string &value);
    bool add_entry(const std::string &key, const std::string &value);
    void delete_last();

   private:
    using key = std::string;
    using value = std::string;
    // using entry = std::pair<const key, value>;

    size_t _max_size;
    size_t _current_size;

    mutable std::unordered_map<std::reference_wrapper<const key>,
                               std::list<Entry>::const_iterator, std::hash<key>,
                               std::equal_to<key>>
        _backend;
    mutable std::list<Entry> _cache;

    mutable std::mutex _mutex;
};

}  // namespace Backend
}  // namespace Afina

#endif  // AFINA_STORAGE_MAP_BASED_GLOBAL_LOCK_IMPL_H
