
//          Copyright Oliver Kowalke 2013.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#ifndef WORKSTEALING_QUEUE_H
#define WORKSTEALING_QUEUE_H

#include <algorithm>

#include <boost/config.hpp>
#include <boost/thread/lock_types.hpp> 

#include <boost/fiber/detail/config.hpp>
#include <boost/fiber/detail/worker_fiber.hpp>
#include <boost/fiber/detail/spinlock.hpp>

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

class workstealing_queue : private noncopyable
{
private:
    boost::fibers::detail::spinlock         splk_;
    boost::fibers::detail::worker_fiber *   head_;
    boost::fibers::detail::worker_fiber *   tail_;

    bool empty_() const BOOST_NOEXCEPT
    { return 0 == head_; }

    void push_( boost::fibers::detail::worker_fiber * item) BOOST_NOEXCEPT
    {
        BOOST_ASSERT( 0 != item);
        BOOST_ASSERT( 0 == item->next() );

        if ( empty_() )
            head_ = tail_ = item;
        else
        {
            tail_->next( item);
            tail_ = item;
        }
    }

    boost::fibers::detail::worker_fiber * pop_() BOOST_NOEXCEPT
    {
        BOOST_ASSERT( ! empty_() );

        boost::fibers::detail::worker_fiber * item = head_;
        head_ = head_->next();
        if ( 0 == head_) tail_ = 0;
        item->next_reset();
        return item;
    }

    bool steal_( boost::fibers::detail::worker_fiber::ptr_t * f)
    {
        boost::fibers::detail::worker_fiber * x = head_, * prev = 0;
        while ( 0 != x)
        {
            boost::fibers::detail::worker_fiber * nxt = x->next();
            if ( ! x->thread_affinity() )
            {
                if ( x == head_)
                {
                    BOOST_ASSERT( 0 == prev);

                    head_ = nxt;
                    if ( 0 == head_)
                        tail_ = 0;
                }
                else
                {
                    BOOST_ASSERT( 0 != prev);

                    if ( 0 == nxt)
                        tail_ = prev;

                    prev->next( nxt); 
                }
                x->next_reset();
                std::swap( f, x);
                return true;
            }
            else
                prev = x;
            x = nxt;
        }
        return false;
    }

public:
    workstealing_queue() BOOST_NOEXCEPT :
        splk_(),
        head_( 0),
        tail_( 0)
    {}

    void push( boost::fibers::detail::worker_fiber * f) BOOST_NOEXCEPT
    {
        BOOST_ASSERT( f);
        boost::unique_lock< boost::fibers::detail::spinlock > lk( splk_);
        push_( f);
    }

    boost::fibers::detail::worker_fiber * pop() BOOST_NOEXCEPT
    {
        BOOST_ASSERT( f);
        boost::unique_lock< boost::fibers::detail::spinlock > lk( splk_);
        return pop_();
    }

    bool steal( boost::fibers::detail::worker_fiber::ptr_t * f)
    {
        boost::unique_lock< boost::fibers::detail::spinlock > lk( splk_);
        return steal_( f);
    }
};

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif //  WORKSTEALING_QUEUE_H
