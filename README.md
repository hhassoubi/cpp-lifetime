# Lifetime C++ Library

## Description 

The `hsqr::Lifetime` C++ library provides robust management of object lifetimes, preventing dangling pointers and ensuring safe access to object. Key features include:

- **Automatic Management**: Ensures automatic resource safe access without relying on `std::shared_ptr`.
- **Thread Safety**: Guarantees safe handling of object lifetimes in multithreaded environments.
- **Lifetime Lock and Observer**: Offers classes to observe and lock object lifetimes, ensuring safe access.
- **Binding Functions**: Enables binding functions to a lifetime of an object, executing only while the object is alive.
- **Value Semantic Paradigm**: Follows the value semantic paradigm, enhancing reliability and predictability.

Ideal for applications requiring precise control over object lifetimes, especially in concurrent programming.

## Examples

### Member function callback with lifetime

```c++
#include "hsqr/lifetime.h"
#include <thread>

class Target
{
public:
    void doAsyncWork() {
        hsqr::LifetimeObserver observer(m_lifetime);
        std::thread t([this, observer] {
            hsqr::LifetimeLock guard(observer);
            if (guard) {
                // it is safe to use this object
                // the destructor of this class will
                // wait for this block of code to exit
                // do the work ...
            }
        });
        t.detach();
    }
private:
    // the Lifetime member should be the last data member
    hsqr::Lifetime m_lifetime;
};

{
    // ...
    Target target;
    target.doAsyncWork();
    // ... 
    // the destructor of target will wait for 1 second
    // the timeout could be changed with the constructor of Lifetime
}

```

### Canceling an async call without complicated cancellation logic 

```c++

SomeServiceClient client;
hsqr::Lifetime responseLifetime;
client.RequestXYZ( hsqr::bind_lifetime(responseLifetime, [](){
    // response handler
}));
//...
// after some time end the lifetime manually 
responseLifetime.end();
// Response handler will not be called
// But end will wait if the object is on flight 
// 
```


## TODO & Questions
 - Create a shared lifetime object 
 - Should the default timeout be large? 