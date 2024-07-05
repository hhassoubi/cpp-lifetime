#include "hsqr/lifetime.h"
#include <cassert>
#include <functional>
#include <iostream>
#include <thread>

class Target
{
public:
    ~Target() {
        std::cout << "Target destroyed" << std::endl;
    }
    void doAsyncWork() {
        hsqr::LifetimeObserver observer(m_lifetime);
        std::thread t([this, observer] {
            hsqr::LifetimeLock guard(observer);
            if (guard) {
                work();
            }
        });
        t.detach();
    }
private:
    void work() {
        std::cout << "start work" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        std::cout << "work done" << std::endl;
    }
    // the Lifetime member should be the last data member
    hsqr::Lifetime m_lifetime;
};



int main()
{
    Target target;
    target.doAsyncWork();
    // give time for the thred to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    // the destructor of target will wait for 1 second
    // the timeout could be changed with the constructor of Lifetime
}