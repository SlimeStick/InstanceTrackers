#pragma once 

#include <cstddef>
#include <vector>
#include <algorithm>

/**
 * @brief CRTP base class that automatically tracks all active instances of a subclass
 *        in a static std::vector<Derived*>.
 * 
 * Whenever a subclass instance is constructed (also default, copy, or move constructed),
 *  a pointer to it is added to the vector.
 * Whenever a subclass instance is destroyed, it is automatically removed from the vector.
 * 
 * Usage:
 *   class Widget : public VectorInstanceTracker<Widget> { ... };
 *
 * This class is not thread safe.
 */
template <typename Derived>
class VectorInstanceTracker
{
public:
    /// Returns a const reference to the static vector of all active instances.
    [[nodiscard]] static const std::vector<Derived*>& get_instances() noexcept
	{
        return _tracked_instances;
    }

private:
    friend Derived;

	VectorInstanceTracker()
	{
        _start_tracking_instance();
    }

    VectorInstanceTracker(const VectorInstanceTracker&)
	{
        _start_tracking_instance();
    }

    // Sadly, tracking a new object means allocating heap memory which can throw exceptions
    VectorInstanceTracker(VectorInstanceTracker&&) noexcept(false)
	{
        _start_tracking_instance();
    }

    // Assignment does not create or destroy objects, so registrations do not change
    VectorInstanceTracker& operator=(const VectorInstanceTracker&) noexcept = default;
    VectorInstanceTracker& operator=(VectorInstanceTracker&&) noexcept = default;

    // Destructor unregisters this instance
    ~VectorInstanceTracker() 
	{
        _stop_tracking_instance();
    }

    void _start_tracking_instance()
	{
        _tracked_instances.push_back(static_cast<Derived*>(this));
    }

    void _stop_tracking_instance() 
	{
        auto instance_iterator = std::find(_tracked_instances.begin(), _tracked_instances.end(), static_cast<Derived*>(this));
        if (instance_iterator != _tracked_instances.end())
        {
            _tracked_instances.erase(instance_iterator);
        }
    }

	inline static std::vector<Derived*> _tracked_instances;
};
