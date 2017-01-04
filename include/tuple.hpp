#pragma once

#include <type_traits>
#include <utility>

namespace parget
{

template< std::size_t index,
          typename type >
struct tuple_leaf
{

    type value_;

};

template< std::size_t index,
          typename type >
constexpr
std::add_const_t< type > &
get(tuple_leaf< index, type > const & _leaf) noexcept
{
    return _leaf.value_;
}

template< std::size_t index,
          typename type >
constexpr
type &
get(tuple_leaf< index, type > & _leaf) noexcept
{
    return _leaf.value_;
}

template< std::size_t index,
          typename type >
constexpr
std::remove_reference_t< type > &&
get(tuple_leaf< index, type > && _leaf) noexcept
{
    return std::move(_leaf.value_);
}

template< std::size_t index,
          typename type >
constexpr
std::remove_reference_t< std::add_const_t< type > > &&
get(tuple_leaf< index, type > const && _leaf) noexcept
{
    return std::move(_leaf.value_);
}

template< std::size_t index,
          typename type >
constexpr
type &&
forward_element(tuple_leaf< index, type > & _leaf) noexcept
{
    return std::forward< type >(_leaf.value_);
}

template< std::size_t index,
          typename type >
type
tuple_element_f(tuple_leaf< index, type > const *);

template< std::size_t index,
          typename tuple >
using tuple_element_t = std::decay_t< decltype(tuple_element_f< index >(std::declval< tuple * >())) >;

template< typename indices,
          typename ...types >
struct tuple_base;

template< std::size_t ...indices,
          typename ...types >
struct tuple_base< std::index_sequence< indices... >, types... >
        : tuple_leaf< indices, types >...
{

    using tuple_base_t = tuple_base;
    using indices_t = std::index_sequence< indices... >;

    template< typename ...arguments,
              typename = std::enable_if_t< (sizeof...(types) == sizeof...(arguments)) > >
    constexpr
    tuple_base(arguments &&... _arguments)
        : tuple_leaf< indices, types >{std::forward< arguments >(_arguments)}...
    { ; }

    template< typename type >
    constexpr
    operator type () const &
    {
        return {get< indices >(*this)...};
    }

    template< typename type >
    constexpr
    operator type () &
    {
        return {get< indices >(*this)...};
    }

    template< typename type >
    constexpr
    operator type () const &&
    {
        return {std::move(get< indices >(*this))...};
    }

    template< typename type >
    constexpr
    operator type () &&
    {
        return {std::move(get< indices >(*this))...};
    }

};

template< typename ...types >
struct tuple
        : tuple_base< std::index_sequence_for< types... >, types... >
{

    using tuple_t = tuple;

    using tuple::tuple_base_t::tuple_base_t;
    using tuple::tuple_base_t::operator = ;

};

template< typename ...tuples >
struct tuple_size
        : std::integral_constant< std::size_t, (tuple_size< tuples >::value + ...) >
{

};

template< typename ...types >
struct tuple_size< tuple< types... > >
        : std::integral_constant< std::size_t, sizeof...(types) >
{

};

template< typename ...arguments >
constexpr
tuple< std::decay_t< arguments >... >
make_tuple(arguments &&... _arguments)
{
    return {std::forward< arguments >(_arguments)...};
}

template< typename ...arguments >
constexpr
tuple< arguments &... >
tie(arguments &... _arguments)
{
    return {_arguments...};
}

template< typename ...arguments >
constexpr
tuple< arguments &&... >
forward_as_tuple(arguments &&... _arguments)
{
    return {std::forward< arguments >(_arguments)...};
}

template< typename ...index_sequences >
struct flatten_indices;

template<>
struct flatten_indices<>
{

    using type = std::index_sequence<>;

};

template< std::size_t ...last >
struct flatten_indices< std::index_sequence< last... > >
{

    using type = std::index_sequence< last... >;

};

template< std::size_t ...first,
          std::size_t ...second,
          typename ...rest >
struct flatten_indices< std::index_sequence< first... >, std::index_sequence< second... >, rest... >
        : flatten_indices< std::index_sequence< first..., second... >, rest... >
{

};

template< typename ...tuples >
struct flatten_types;

template<>
struct flatten_types<>
{

    using type = tuple<>;

};

template< typename ...last >
struct flatten_types< tuple< last... > >
{

    using type = tuple< last... >;

};

template< typename ...first,
          typename ...second,
          typename ...rest >
struct flatten_types< tuple< first... >, tuple< second... >, rest... >
        : flatten_types< tuple< first..., second... >, rest... >
{

};

template< std::size_t index,
          typename indices >
struct duplicate;

template< std::size_t index,
          std::size_t ...indices >
struct duplicate< index, std::index_sequence< indices... > >
{

    using type = std::index_sequence< (static_cast< void >(indices), index)... >;

};

template< typename ...tuples >
struct indices_cat
{

    template< typename indices >
    struct _inner_indices;

    template< std::size_t ...indices >
    struct _inner_indices< std::index_sequence< indices... > >
    {

        using type = typename flatten_indices< typename duplicate< indices, std::make_index_sequence< tuple_size< tuples >::value > >::type... >::type;

    };

    using inner_indices = typename _inner_indices< std::index_sequence_for< tuples... > >::type;
    using outer_indices = typename flatten_indices< std::make_index_sequence< tuple_size< tuples >::value >... >::type;

};

template< typename result_type,
          typename tuples,
          std::size_t ...outer_indices,
          std::size_t ...inner_indices >
constexpr
result_type
_tuple_cat(tuples && _tuples,
           std::index_sequence< outer_indices... >,
           std::index_sequence< inner_indices... >)
{
    return {forward_element< outer_indices >(get< inner_indices >(_tuples))...};
}

template< typename ...tuples,
          typename result_type = typename flatten_types< tuples... >::type,
          typename indices = indices_cat< tuples... > >
constexpr
result_type
tuple_cat(tuples &&... _tuples)
{
    return _tuple_cat< result_type, tuple< tuples &&... > >((forward_as_tuple)(std::forward< tuples >(_tuples)...), typename indices::outer_indices{}, typename indices::inner_indices{});
}

}
