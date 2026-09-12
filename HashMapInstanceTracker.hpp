#pragma once 

#include <cstddef>
#include <unordered_map>
#include <optional>
#include <utility>
#include <stdexcept>

/**
 * @brief CRTP base class that automatically tracks all active instances of a subclass
 *        in a static std::unordered_map.
 *        
 * Subclasses pass a key to this base class constructor.
 * Whenever a subclass instance is constructed (also default, copy, or move constructed),
 *  a pointer to it is added to the map.
 * Whenever a subclass instance is destroyed, it is automatically removed from said map.
 * 
 * Usage:
 *   class Widget : public HashMapInstanceTracker<Widget, std::string> {
 *   public:
 *       Widget(std::string id, ...) : HashMapInstanceTracker(std::move(id)) { ... }
 *   };
 *
 * This class is not thread safe.
 */
template <
    typename Derived,
    typename Key,
    typename Hash = std::hash<Key>,
    typename KeyEqual = std::equal_to<Key>
>
class HashMapInstanceTracker {
public:
    /// Returns a const reference to the static hash map of all active instances.
    [[nodiscard]] static const std::unordered_map<Key, Derived*, Hash, KeyEqual>& get_instances()
    {
        // get_instances is not noexcept if it's called before any object was instantiated
        return _tracked_instances();
    }

    /// Returns the key associated with this instance.
    [[nodiscard]] const std::optional<Key>& get_key() const noexcept
    {
        return _key;
    }

    // A std::unordered_map cannot store two keys with the same value
    HashMapInstanceTracker(const HashMapInstanceTracker&) = delete;
    HashMapInstanceTracker& operator=(const HashMapInstanceTracker&) = delete;

    // Thrown when an instance is created with a key that's already in use by another instance
    class HashMapInstanceTrackerKeyError : public std::exception {};

private:
    friend Derived;

    explicit HashMapInstanceTracker(Key key) : _key(std::move(key))
	{
		if (_tracked_instances().find(*_key) != _tracked_instances().end())
		{
            throw HashMapInstanceTrackerKeyError{};
		}
        _set_key_to_instance();
    }

    // Sadly, tracking a new object means allocating heap memory which can throw exceptions
    HashMapInstanceTracker(HashMapInstanceTracker&& other) noexcept(false) : _key(std::move(other._key))
	{
        // Ensure moved-from object will not unregister this key on destruction
        other._key.reset();
        // Update the key to point to the new instance
        _set_key_to_instance();
    }

    HashMapInstanceTracker& operator=(HashMapInstanceTracker&& other) noexcept(false)
    {
        _remove_key_mapping();
        _key = std::exchange(other._key, std::nullopt);
        _set_key_to_instance();
        return *this;
    }

    ~HashMapInstanceTracker()
	{
        if (_key.has_value())
        {
            _remove_key_mapping();
        }
    }

    void _set_key_to_instance()
	{
        _tracked_instances()[*_key] = static_cast<Derived*>(this);
    }

    void _remove_key_mapping()
	{
        _tracked_instances().erase(*_key);
    }

    std::optional<Key> _key;

    [[nodiscard]] static auto& _tracked_instances() noexcept(false)
    {
        static std::unordered_map<Key, Derived*, Hash, KeyEqual> tracked_instances;
        return tracked_instances;
    }
};
