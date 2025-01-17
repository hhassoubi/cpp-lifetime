#include <cassert>
#include <chrono>
#include <memory>
#include <thread>
#include <type_traits>

// thank you to the boost.org for this code
#if !defined(HSQR_HAS_STD_INVOKE_RESULT)
#  if defined(_MSC_VER)
#   if (_MSC_VER >= 1911 && _MSVC_LANG >= 201703)
#    define HSQR_HAS_STD_INVOKE_RESULT 1
#   endif
#  else
#   if (__cplusplus >= 201703)
#    define HSQR_HAS_STD_INVOKE_RESULT 1
#   endif 
# endif
#endif

namespace hsqr {

#if defined(HSQR_HAS_STD_INVOKE_RESULT)
// thank you to the boost.org for this code
template <typename> struct result_of;

template <typename F, typename... Args>
struct result_of<F(Args...)> : std::invoke_result<F, Args...> {};

template <typename T>
using result_of_t = typename result_of<T>::type;

#else

using std::result_of;

template <typename T>
using result_of_t = typename std::result_of<T>::type;

#endif

template <typename T>
using decay_t = typename std::decay<T>::type;



struct LifetimeSharedState
{
};

class Lifetime
{
    friend class LifetimeObserver;
    friend class LifetimeLock;
public:
    Lifetime()
        : m_state(std::make_shared<LifetimeSharedState>()),
          m_end_of_life_timeout(std::chrono::milliseconds(1000))
    {
    }

    template <typename Rep, typename Period>
    Lifetime(std::chrono::duration<Rep, Period> end_of_life_timeout)
        : m_state(std::make_shared<LifetimeSharedState>()),
          m_end_of_life_timeout(std::chrono::duration_cast<std::chrono::milliseconds>(end_of_life_timeout))
    {
    }

    ~Lifetime()
    {
        if ( !end() )
            assert(false);        
    }

    // copy constructor create new Lifetime object
    Lifetime(const Lifetime&)
        : m_state(std::make_shared<LifetimeSharedState>())
    {
    }
    // move constructor create new Lifetime object because move dose
    // not preserve the address of the object
    Lifetime(Lifetime && other) {
        // 1. end lifetime of then other object
        if ( !other.end() )
            assert(false);
        // 2. create a new Lifetime, because the address of the object is not the same
        m_state = std::make_shared<LifetimeSharedState>();
    }

    // no need to create new life time because the address is still the same
    Lifetime &operator=(const Lifetime &) {
        return *this;
    }

    // no need to create new life time because the address is still the same
    // the lifetime of other has to end now
    Lifetime &operator=(Lifetime && other) {
        if ( !other.end() )
            assert(false);
        return *this;
    }

    // end the lifetime of the object now.
    // can be used to control the lifetime of an object manually 
    bool end() {
        return end_and_wait_for_observers(m_end_of_life_timeout);
    }

    // end the lifetime of the object now with different timeout
    // can be used to control the lifetime of an object manually 
    template <typename Rep, typename Period>
    bool end(std::chrono::duration<Rep, Period>  timeout) {
        return end_and_wait_for_observers(std::chrono::duration_cast<std::chrono::milliseconds>(timeout));
    }

#ifdef HSQR_TESTING
    LifetimeSharedState* state() const {
        return m_state.get();
    }
    long use_count() const {
        return m_state.use_count();
    }
#endif
private:
    bool end_and_wait_for_observers(std::chrono::milliseconds timeout) 
    {
        std::chrono::time_point<std::chrono::system_clock> end_of_life = std::chrono::system_clock::now() + timeout;
        // 1. copy the shared state to a weak_ptr
        std::weak_ptr<LifetimeSharedState> weak_state = m_state;
        // 2. reset the state (delete the shared state if count is 0)
        m_state.reset();
        while (weak_state.use_count() > 0) {
            if (std::chrono::system_clock::now() > end_of_life) {
                // timeout return false
                return false;
            }
            std::this_thread::yield();
        }
        return true;
    }

    std::shared_ptr<LifetimeSharedState> m_state;
    std::chrono::milliseconds m_end_of_life_timeout;
};

class LifetimeLock;

class LifetimeObserver
{
    friend class LifetimeLock;
public:
    LifetimeObserver() = default;
    /* explicit(false) */LifetimeObserver(const Lifetime &lt)
        : m_state(lt.m_state)
    {
    }
    LifetimeLock lock() const;
private:   
    std::weak_ptr<LifetimeSharedState> m_state;
};


class LifetimeLock
{
public:
    /* explicit(false) */  LifetimeLock(const LifetimeObserver &lt)
        : m_locked_state(lt.m_state.lock())
    {
    }
    /* explicit(false) */  LifetimeLock(const Lifetime &lt)
        : m_locked_state(lt.m_state)
    {
    }
    ~LifetimeLock() = default;
    LifetimeLock(const LifetimeLock &) = delete;
    LifetimeLock &operator=(const LifetimeLock &) = delete;
    // move only
    LifetimeLock(LifetimeLock && other) {
        m_locked_state = other.m_locked_state;
        other.m_locked_state.reset();
    }
    LifetimeLock &operator=(LifetimeLock && other) {
        m_locked_state = other.m_locked_state;
        other.m_locked_state.reset();
        return *this;
    }

    bool is_lock() const {
        return m_locked_state != nullptr;
    }
    operator bool() const {
        return is_lock();
    }
    void unlock(){
        m_locked_state.reset();
    }
    
private:    
    std::shared_ptr<LifetimeSharedState> m_locked_state;
};

inline LifetimeLock LifetimeObserver::lock() const {
    return LifetimeLock(*this);
}

template <typename Target>
struct _LifetimeFunctionBinder
{
    template<typename F>
    _LifetimeFunctionBinder(const Lifetime& lt, F&& f)
        : m_lifetime_observer(lt),
          m_target(std::forward<F>(f))
    {
    }

    /// Forwarding function call operator.
    template <typename... Args>
    result_of_t<Target(Args...)> operator()(Args&&... args)
    {
        if (auto lock = m_lifetime_observer.lock()) {
            return this->m_target(std::forward<Args>(args)...);
        }
        return result_of_t<Target(Args...)>();
    }

    /// Forwarding function call operator.
    template <typename... Args>
    result_of_t<Target(Args...)> operator()(Args&&... args) const
    {
        if (auto lock = m_lifetime_observer.lock()) {
            return this->target_(std::forward<Args>(args)...);
        }
        return result_of_t<Target(Args...)>();
    }
    
    
    LifetimeObserver m_lifetime_observer;
    Target m_target;;
};

template <typename F>
_LifetimeFunctionBinder<decay_t<F>> bind_lifetime(const Lifetime& lt, F&& f)
{
    return _LifetimeFunctionBinder<decay_t<F>>(lt, std::forward<F>(f));
}

} // namespace hsqr
