#define HSQR_TESTING
#include "hsqr/lifetime.h"

#include <cassert>
#include <functional>
#include <iostream>


using namespace hsqr;
// ------------------------------------------
struct A
{
    Lifetime m_lifetime;
};

void test_copy()
{
    //std::bind()
    A a;
    A b(a);
    assert(a.m_lifetime.state() != b.m_lifetime.state());
    assert(a.m_lifetime.state());
    assert(b.m_lifetime.state());
}

void test_copy_assignment()
{
    A a;
    A b;
    auto b_state = b.m_lifetime.state();
    b = a;
    assert(a.m_lifetime.state() != b.m_lifetime.state());
    assert(a.m_lifetime.state());
    assert(b.m_lifetime.state() == b_state);
}

void test_move()
{
    A a;
    A b(std::move(a));
    assert(a.m_lifetime.state() != b.m_lifetime.state());
    assert(a.m_lifetime.state() == nullptr);
    assert(b.m_lifetime.state());
}

void test_move_assignment()
{
    A a;
    A b;
    auto b_state = b.m_lifetime.state();
    b = std::move(a);
    assert(a.m_lifetime.state() != b.m_lifetime.state());
    assert(a.m_lifetime.state() == nullptr);
    assert(b.m_lifetime.state() == b_state);
}   

void test_lifetime_free_observers()
{
    // using heap to control the Lifetime of the object
    Lifetime* lt = new Lifetime();
    LifetimeObserver observer1(*lt);
    LifetimeObserver observer2(*lt);
    
    assert(lt->use_count() == 1);
    delete lt;

    assert(observer1.lock() == false);
    assert(observer2.lock() == false);   
}

void test_lifetime_locked_observers()
{
    // using heap to control the Lifetime of the object
    Lifetime* lt = new Lifetime();
    LifetimeObserver observer1(*lt);
    LifetimeObserver observer2(*lt);
    {
        LifetimeLock guard1(observer1);
        assert(guard1);
        assert(guard1.is_lock());
        assert(lt->use_count() == 2);
        {
            LifetimeLock guard2(observer2);
            assert(guard2);
            assert(guard2.is_lock());
            assert(lt->use_count() == 3);
        }
        assert(lt->use_count() == 2);
    }
    assert(lt->use_count() == 1);
    delete lt;

    assert(observer1.lock() == false);
    assert(observer2.lock() == false);   
}

void test_wait_for_observer()
{
    Lifetime* lt = new Lifetime();
 
    LifetimeObserver observer(*lt);
    std::thread t([observer] {
        LifetimeLock guard(observer);
        if (guard) {
            // sleep for 500ms. the default timeout is 1000ms 
            std::this_thread::sleep_for(std::chrono::milliseconds(500));    
            std::cout << "sleep done" << std::endl;
        }
    });

    // 100 ms sleep to make sure the thread is started
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // this will assert if timedout
    delete lt; 
    std::cout << "test done" << std::endl;

    t.join();
}

void test_wait_for_observers()
{
    Lifetime* lt = new Lifetime();
    LifetimeObserver observer(*lt);
    std::thread t1([observer] {
        LifetimeLock guard(observer);
        if (guard) {
            // sleep for 500ms. the default timeout is 1000ms 
            std::this_thread::sleep_for(std::chrono::milliseconds(500));    
            std::cout << "thread 1: sleep done" << std::endl;
        }
    });

    std::thread t2([observer] {
        LifetimeLock guard(observer);
        if (guard) {
            // sleep for 500ms. the default timeout is 1000ms 
            std::this_thread::sleep_for(std::chrono::milliseconds(500));    
            std::cout << "thread 2: sleep done" << std::endl;
        }
    });

    // 100 ms sleep to make sure the thread is started
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // this will assert if timedout
    delete lt; 
    std::cout << "test done" << std::endl;

    t1.join();
    t2.join();
}

void test_wait_for_observer_with_timeout()
{
    Lifetime* lt = new Lifetime(std::chrono::seconds(3));
 
    LifetimeObserver observer(*lt);
    std::thread t([observer] {
        LifetimeLock guard(observer);
        if (guard) {
            // sleep for 500ms. the default timeout is 1000ms 
            std::this_thread::sleep_for(std::chrono::seconds(2));    
            std::cout << "sleep done" << std::endl;
        }
    });

    // 100 ms sleep to make sure the thread is started
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // this will assert if timedout
    delete lt; 
    std::cout << "test done" << std::endl;

    t.join();
}


void test_bind()
{
    std::string a="a";
    std::string b="b";
    std::string c="c";

    Lifetime lt = Lifetime();
    {
        // using lambda return void
        auto f = bind_lifetime(lt, [](std::string a, std::string& b, const std::string& c, std::string&& d) {
            std::cout << "a: " << a << ", ";
            std::cout << "b: " << b << ", ";
            std::cout << "c: " << c << ", ";
            std::cout << "d: " << d << std::endl;
        });
        f(a, b, c, std::string("d"));
    }
    {
        // using lambda return int
        auto f = bind_lifetime(lt, [](std::string a, std::string& b, const std::string& c, std::string&& d) {
            std::cout << "a: " << a << ", ";
            std::cout << "b: " << b << ", ";
            std::cout << "c: " << c << ", ";
            std::cout << "d: " << d << std::endl;
            return a;
        });
        f(a, b, c, std::string("d"));
    }
    struct BindTo
    {
        void foo(std::string a, std::string& b, const std::string& c, std::string&& d)
        {
            std::cout << "a: " << a << ", ";
            std::cout << "b: " << b << ", ";
            std::cout << "c: " << c << ", ";
            std::cout << "d: " << d << std::endl;
        }
    };
    {
        // using bind
        BindTo bindTo;
        auto f = bind_lifetime(lt, std::bind(&BindTo::foo, &bindTo, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
        f(a, b, c, std::string("d"));
    }
    {
        // make std:function
        BindTo bindTo;
        std::function<void(std::string, std::string&, const std::string&, std::string&&)> f = bind_lifetime(lt, 
            std::bind(&BindTo::foo, &bindTo, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
        f(a, b, c, std::string("d"));
    }
    {
        // copy to std:function
        BindTo bindTo;
        std::function<void(std::string, std::string&, const std::string&, std::string&&)> f;
        {
            auto b = bind_lifetime(lt, 
                std::bind(&BindTo::foo, &bindTo, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
            f = b;
        }
        f(a, b, c, std::string("d"));
    }
}




int main()
{
    test_copy();
    test_move();
    test_copy_assignment();
    test_move_assignment();
    test_lifetime_free_observers();
    test_lifetime_locked_observers();
    test_wait_for_observer();
    test_wait_for_observers();
    test_wait_for_observer_with_timeout();
    test_bind();


    return 0;
}