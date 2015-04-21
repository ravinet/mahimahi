/* -*-mode:c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

#ifndef BIND_WORKAROUND_HH
#define BIND_WORKAROUND_HH

/* g++ bug 55914 applies to g++ before version 4.9 */

/* the bug causes g++ not to expand parameter packs
   (variadic template arguments) within a lambda */

/* std::bind can be used as a workaround, but it can't
   be used with move-only objects like unique_ptr */

/* this is a workaround based on
   http://stackoverflow.com/questions/7858817/unpacking-a-tuple-to-call-a-matching-function-pointer */

/* see also
   http://stackoverflow.com/questions/4871273/passing-rvalues-through-stdbind */

#include <tuple>

namespace BindWorkAround
{
    /* recursive case */
    template <int First, int... Rest>
    struct ints_0_to_N : ints_0_to_N<First-1, First, Rest...> {};

    /* base case */
    template <int... Rest>
    struct ints_0_to_N<0, Rest...>
    {
        typedef ints_0_to_N<0, Rest...> type;
        virtual ~ints_0_to_N() {} /* for g++ 4.8 -Weffc++ :-( */
    };

    template <typename Object, typename... Targs>
    class bind
    {
    private:
        std::tuple<Targs...> bound_arguments_;

        template <int... S>
        Object construct( ints_0_to_N<S...> )
        {
            return Object( std::move( std::get<S>( bound_arguments_ ) )... );
        }

    public:
        bind( Targs&&... Fargs )
            : bound_arguments_( std::forward<Targs>( Fargs )... )
        {}

        Object operator()( void )
        {
            return construct( typename ints_0_to_N<sizeof...( Targs ) - 1>::type() );
        }
    };
}

#endif /* BIND_WORKAROUND_HH */
