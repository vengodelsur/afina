#ifndef AFINA_STORAGE_MAP_BASED_GLOBAL_LOCK_IMPL_H
#define AFINA_STORAGE_MAP_BASED_GLOBAL_LOCK_IMPL_H

#include <mutex>
#include <string>
#include <unordered_map>
#include <iostream>
#include <afina/Storage.h>

namespace Afina {
namespace Backend {
class Entry {
   public:
    Entry() : _key(NULL), _value(NULL), _next(nullptr), _previous(nullptr) {}
    Entry(const std::string &key, const std::string &value,
          Entry *next = nullptr, Entry *previous = nullptr)
        : _key(key), _value(value), _next(next), _previous(previous) {}

    size_t size() const { return _key.size() + _value.size(); }
    size_t get_value_size() const { return _value.size(); }

    void set_value(const std::string &value) const { _value = value; }
    const std::string &get_key_reference() const { return _key; }
    std::string get_value() const { return _value; }
    Entry *get_next() const { return _next; }
    Entry *get_previous() const { return _previous; }

    Entry *_next;
    Entry *_previous;

   private:
    const std::string _key;
    mutable std::string _value;

    // Entry& will be used instead of iterator
};
class CacheList {
   public:
    CacheList() : _head(nullptr), _tail(nullptr) {}
    ~CacheList() {
        Entry *tmp = _head;
        while (tmp != nullptr) {
            Entry *previous = tmp;
            tmp = tmp->get_next();
            delete previous;
        }
    }
    void MoveToHead(Entry *entry) {
        if (entry != _head) {
        Exclude(entry);

        AddToHead(entry);
        std::cout << "moved to head" << std::endl;
        Print();
        }
        else { return;}
    }

    Entry *GetHead() { return _head; }
    Entry *GetTail() { return _tail; }
    void AddToHead(Entry *entry) {
        if (_head != nullptr) {
            entry->_next = _head;  // copying?
            _head->_previous = entry;
            _head = entry;
        } else {
            entry->_next = _head;
            _head = entry;
            _tail = entry;
        }
        std::cout << "added to head" << std::endl;
        Print();
    }
    void DeleteTail() { Delete(_tail); }
    void Delete(Entry *entry) {
        Exclude(entry);
        delete entry;
        
    }
    void Exclude(Entry *entry) {
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
        std::cout << "excluded" << std::endl;
        Print();
    }

    void Print() {
        Entry *tmp = _head;
        while (tmp != nullptr) {
            Entry *previous = tmp;
            std::cout << "Address: " << tmp << " key: " << tmp->get_key_reference() << " value: " << tmp->get_value() << std::endl;
            tmp = tmp->get_next();
        
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
    bool set_head_value(const std::string &key, const std::string &value);
    bool add_entry(const std::string &key, const std::string &value);
    void delete_last();
    bool CheckSize(const std::string &key, const std::string &value) {
        size_t entry_size = key.size() + value.size();

        if (entry_size <= _max_size) {
            return true;
        } else {
            return false;
        }
    }

   private:
    using key = std::string;
    using value = std::string;
    // using entry = std::pair<const key, value>;

    size_t _max_size;
    size_t _current_size;

    /*mutable std::unordered_map<std::reference_wrapper<const key>,
                               std::list<Entry>::const_iterator, std::hash<key>,
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
