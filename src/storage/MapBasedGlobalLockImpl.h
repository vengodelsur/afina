#ifndef AFINA_STORAGE_MAP_BASED_GLOBAL_LOCK_IMPL_H
#define AFINA_STORAGE_MAP_BASED_GLOBAL_LOCK_IMPL_H

#include <afina/Storage.h>
#include <iostream>
#include <mutex>
#include <string>
#include <unordered_map>

namespace Afina {
namespace Backend {
using key = std::string;
using value = std::string;

class Entry {
   public:
    Entry() : _key(NULL), _value(NULL), _next(nullptr), _previous(nullptr) {}
    Entry(const std::string &key, const std::string &value,
          Entry *next = nullptr, Entry *previous = nullptr)
        : _key(key), _value(value), _next(next), _previous(previous) {}

    size_t Size() const { return _key.size() + _value.size(); }
    size_t GetValueSize() const { return _value.size(); }

    void SetValue(const std::string &value) const { _value = value; }
    const std::string &GetKeyReference() const { return _key; }
    std::string GetValue() const { return _value; }

    Entry *_next;
    Entry *_previous;
    friend std::ostream &operator<<(std::ostream &out, const Entry &entry) {
        out << "Address: " << &entry;
        out << " key: " << entry.GetKeyReference();
        out << " value: " << entry.GetValue();
        out << " next: " << entry._next;
        out << " previous: " << entry._previous;
        return out;
    }

   private:
    const std::string _key;
    mutable std::string _value;

    // Entry& will be used instead of iterator
};
class CacheList {
   public:
    CacheList() : _head(nullptr), _tail(nullptr) {}
    ~CacheList();
    void MoveToHead(Entry *entry);

    Entry *GetHead();
    Entry *GetTail();
    void AddToHead(Entry *entry);
    void DeleteTail();
    void Delete(Entry *entry);
    void Exclude(Entry *entry);
    void Print() {
        Entry* tmp = nullptr;
        if (_head != nullptr) {
            std::cout << "HEAD " << *_head << std::endl;
            tmp = _head->_next;
        }
        if (_tail != nullptr) {
            std::cout << "TAIL " << *_head << std::endl;
        }
        
        while (tmp != nullptr) {
            std::cout << *tmp << std::endl;
            tmp = tmp->_next;
        }
    }

   private:
    Entry *_head;
    Entry *_tail;
};

/**
 * # Map based implementation with global lock
 *
 *
 */
class MapBasedGlobalLockImpl : public Afina::Storage {
   public:
    MapBasedGlobalLockImpl(size_t max_size = 1024)
        : _max_size(max_size), _current_size(0) {}
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
    bool SetHeadValue(const std::string &key, const std::string &value);
    bool AddEntry(const std::string &key, const std::string &value);
    void DeleteLast();
    bool CheckSize(const std::string &key, const std::string &value) {
        size_t entry_size = key.size() + value.size();

        if (entry_size <= _max_size) {
            return true;
        } else {
            return false;
        }
    }

   private:
    // using entry = std::pair<const key, value>;

    size_t _max_size;
    size_t _current_size;

    /*mutable std::unordered_map<std::reference_wrapper<const key>,
                               std::list<Entry>::const_iterator,
       std::hash<key>,
                               std::equal_to<key>>*/
    mutable std::unordered_map<std::reference_wrapper<const key>, Entry &,
                               std::hash<key>, std::equal_to<key>>
        _backend;
    // mutable std::list<Entry> _cache;
    mutable CacheList _cache;

    mutable std::mutex _mutex;
};

}  // namespace Backend
}  // namespace Afina

#endif  // AFINA_STORAGE_MAP_BASED_GLOBAL_LOCK_IMPL_H
