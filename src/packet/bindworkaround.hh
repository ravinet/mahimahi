/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef BIND_WORKAROUND_HH
#define BIND_WORKAROUND_HH

/* g++ bug 55914 applies to g++ before version 4.9 */

/* the bug causes g++ not to expand parameter packs
   (variadic template arguments) within a lambda */

/* std::bind can be used as a workaround, but it does
   not support perfect forwarding (so can't be used
   with move-only objects like unique_ptr) */

/* this is a workaround based on
   http://code-slim-jim.blogspot.jp/2012/11/perfect-forwarding-bind-compatable-with.html */

namespace BindWorkAround
{
    /* general list type */
    template <class Object, typename Car, typename... Cdr>
    struct list
    {
        Car car_;                    /* first/this element */
        list<Object, Cdr...> cdr_;   /* rest of the elements */

        /* construct by initializing this element and rest */
        list( Car&& car, Cdr&&... cdr )
            : car_( std::forward<Car>( car ) ),
              cdr_( std::forward<Cdr>( cdr )... )
        {}

        /* construct the target object by appending our
           element to the parameters until we reach the end */
        template <typename... Targs>
        Object construct( Targs&&... Fargs )
        {
            return cdr_.construct( std::forward<Targs>( Fargs )...,
                                   std::forward<Car>( car_ ) );
        }
    };

    /* base case: one-element list */
    template <class Object, typename Car>
    struct list<Object, Car>
    {
        Car car_;

        list( Car&& car )
            : car_( std::forward<Car>( car ) )
        {}

        template <typename... Targs>
        Object construct( Targs&&... Fargs )
        {
            /* finally construct the target object */
            return Object( std::forward<Targs>( Fargs )...,
                           std::forward<Car>( car_ ) );
        }
    };

    template <typename Object, typename... Targs>
    class bind
    {
    private:
        list<Object, Targs...> bound_arguments_;

    public:
        bind( Targs&&... Fargs )
            : bound_arguments_( std::forward<Targs>( Fargs )... )
        {}

        Object operator()( void )
        {
            return bound_arguments_.construct();
        }
    };
}

#endif /* BIND_WORKAROUND_HH */
