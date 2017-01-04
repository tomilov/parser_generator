#include "tuple.hpp"

#include <versatile.hpp>

#include <type_traits>
#include <utility>
#include <system_error>
#include <sstream>
#include <stdexcept>
#include <regex>
#include <tuple>

#define PP { std::cout << __PRETTY_FUNCTION__ << std::endl; }

namespace versatile
{
namespace parser_generator
{

struct filler { template< typename type > operator type (); };

template< typename aggregate, typename index_sequence = std::index_sequence<>, typename = void >
struct aggregate_arity
        : index_sequence
{

};

template< typename aggregate, std::size_t ...indices >
struct aggregate_arity< aggregate, std::index_sequence< indices... >, std::void_t< decltype(aggregate{(void(indices), std::declval< filler >())..., std::declval< filler >()}) > >
    : aggregate_arity< aggregate, std::index_sequence< indices..., sizeof...(indices) > >
{

};

struct unused_type {};

template< typename type, typename = void >
struct is_unused_type
        : std::false_type
{

};

template<>
struct is_unused_type< unused_type, void >
        : std::true_type
{

};

template< typename type, typename = void >
struct is_parser
        : std::false_type
{

};

template< typename type >
struct is_parser< type, std::void_t< typename type::parser_type > >
        : std::true_type
{

};

template< typename iterator_type, typename skipper_type = unused_type >
struct dispatcher;

void as_parser(...) { ; }

template< typename grammar_type, typename attribute_type = decltype(as_parser(std::declval< grammar_type && >())) >
struct get_attribute
        : identity< type >
{

};

template< typename type >
using get_attribute_t = typename get_attribute< type >::type;

template< typename iterator_type, typename skipper_type >
struct dispatcher
{
    
    iterator_type & beg;
    iterator_type end;
    
    skipper_type skipper;
    bool preskip = true;

    bool success = true; // enum { success, hard_fail, soft_fail, };
    
    template< typename grammar_type >
    operator grammar_type ()
    {
        if constexpr (is_unused_type< grammar_type >::value) {
            return {};
        } else {
            if (!success) {
                return {};
            }
            if constexpr (!is_unused_type< std::remove_cv_t< std::remove_reference_t< skipper_type > > >::value) {
                if (preskip) {
                    skipper = dispatcher< iterator_type, unused_type >{beg, end, {}, false};
                }
            }
            if constexpr (is_visitable_v< grammar_type >) {
                return dispatch_variant< grammar_type >(typename grammar_type::types_t{});
            } else {
                using arity_type = aggregate_arity< grammar_type >;
                if constexpr (0 < arity_type::size()) {
                    return dispatch_aggregate< grammar_type >(arity_type{});
                } else {
                    return dispatch(identity< attribute >{}, *this);
                }
            }
        }
    }

    template< typename grammar_type, typename first, typename ...rest >
    grammar_type
    dispatch_variant(identity< first, rest... >)
    {
        success = true;
        grammar_type a{in_place< first >, *this};
        if (!success) {
            if constexpr (sizeof...(rest) == 0) {
                return {};
            } else {
                return dispatch_variant< grammar_type, rest... >();
            }
        }
        return a;
    }

    template< typename grammar_type, std::size_t ...indices >
    grammar_type
    dispatch_aggregate(std::index_sequence< indices... >)
    {
        return {(void(indices), *this)...};
    }

};

template< typename iterator_type, typename skipper_type = unused_type >
dispatcher< iterator_type, skipper_type >
parse(iterator_type & beg, iterator_type const & end, skipper_type && skipper = {})
{
    return {beg, end, std::forward< skipper_type >(skipper)};
}

template< typename from, typename to >
struct intercept
{

    using parser_type = intercept;

    using type = get_attribute_t< from >;
    using value_type = get_attribute_t< to >;

    type source;
    value_type destination;

    template< typename iterator_type, typename skipper_type >
    void
    operator = (dispatcher< iterator_type, skipper_type > & _dispatcher)
    {
        if (!_dispatcher.success) {
            return;
        }
        auto beg = _dispatcher.beg;
        source = _dispatcher;
        if (!_dispatcher.success) {
            return;
        }
        auto end = std::exchange(_dispatcher.end, std::exchange(_dispatcher.beg, std::move(beg)));
        destination = _dispatcher;
        if (std::exchange(_dispatcher.end, std::move(end)) != _dispatcher.beg) {
            _dispatcher.success = false;
        }
    }

    operator value_type const & () const &
    {
        return destination;
    }

    operator value_type () &&
    {
        return std::move(destination);
    }

};

template< typename left, typename right >
struct binary_parser
{

    using parser_type = binary_parser;

    using lhs_type = get_attribute_t< left >;
    using rhs_type = get_attribute_t< right >;

    lhs_type lhs_;
    rhs_type rhs_;

    template< typename type >
    operator type () const &
    {
        return {lhs_, rhs_};
    }

    template< typename type >
    operator type () &&
    {
        return {std::move(lhs_), std::move(rhs_)};
    }

};

}
}

#include <ostream>
#include <iostream> 
#include <iterator>
#include <algorithm>

#include <cstdlib>

using namespace versatile::parser_generator;

template< typename char_type, char_type ...chars >
struct string_literal
{

    static constexpr std::size_t size = sizeof...(chars);
    static constexpr char_type value[size] = {chars...};

};

template< typename char_type, char_type ...chars >
constexpr std::size_t string_literal< char_type, chars... >::size;

template< typename char_type, char_type ...chars >
constexpr char_type string_literal< char_type, chars... >::value[size];

template< typename char_type, char_type ...chars >
constexpr
string_literal< char_type, chars... >
operator ""_s ()
{
    return {};
}

template< typename type >
struct is_string_literal
        : std::false_type
{

};

template< typename char_type, char_type ...chars >
struct is_string_literal< string_literal< char_type, chars... > >
        : std::true_type
{

};

template< typename char_type, char_type ...chars >
struct regex
        : string_literal< char_type, chars... >
{

};

template< typename char_type, char_type ...chars >
constexpr
regex< char_type, chars... >
operator ""_regex ()
{
    return {};
}

template< typename type >
struct is_regex
        : std::false_type
{

};

template< typename char_type, char_type ...chars >
struct is_regex< regex< char_type, chars... > >
        : std::true_type
{

};

template< typename type, typename = void >
struct is_inputable
        : std::false_type
{

};

template< typename type >
struct is_inputable< type, std::void_t< decltype(std::declval< std::istringstream & >() >> std::declval< type & >()) > >
    : std::true_type
{

          };

/*else if constexpr (is_string_literal< grammar_type >::value) {
    iterator_type first = beg;
    for (auto const & c : grammar_type::value) {
        if ((beg != end) && (c == *beg)) {
            ++beg;
        } else {
            beg = first;
            success = false;
            break;
        }
    }
    return {};
} else if constexpr (is_regex< grammar_type >::value) {
    std::regex r{std::cbegin(grammar_type::value), std::cend(grammar_type::value)};
    std::smatch m;
    if (std::regex_search(beg, end, m, r)) {
        if (m.size() == 1) {
            assert(0 < m.length());
            assert(m.prefix().length() == 0);
            std::advance(beg, m.length());
        } else {
            std::runtime_error("error: regex contain captures");
        }
    } else {
        success = false;
    }
    return {};
} else if constexpr (is_inputable< grammar_type >::value) {
    grammar_type attr{};
    std::istringstream iss({beg, end});
    iss.unsetf(std::istringstream::skipws);
    if (iss >> attr) {
        if (iss.eof()) {
            beg = end; // tellg is -1
        } else {
            std::advance(beg, iss.tellg());
        }
    } else {
        success = false;
    }
    return attr;
} */

template< typename ast, typename skipper_type = unused_type >
ast
run(std::string input, skipper_type && skipper = {})
{
    auto it = std::cbegin(input);
    auto const end = std::cend(input);
    auto parser = parse(it, end, skipper);
    ast a = {};
    a = parser;
    if (it != end) {
        std::cout << "End of input is not reached. Rest of input:" << std::endl;
        std::copy(it, end, std::ostreambuf_iterator< typename std::string::value_type >(std::cout));
        std::cout << std::endl;
    }
    return a;
}

template< typename char_type, char_type ...chars >
std::ostream &
operator << (std::ostream & _out, string_literal< char_type, chars... > _string_literal)
{
    std::copy_n(std::cbegin(_string_literal.value), _string_literal.size, std::ostreambuf_iterator< char_type >(_out));
    return _out;
}

//#include <insituc/debug/demangle.hpp>
#include <typeinfo>

static_assert(is_parser< intercept< int, int > >::value, "not a parser");

template< std::size_t >
struct S
{

};

template< typename grammar_type, typename dispatcher_type >
constexpr
grammar_type
dispatch(versatile::identity< std::string >, dispatcher_type &)
{
    return {};
}

int
main()
{
#if 0
    auto skipper = R"(^(?:(?:\s+)|(?://[^\r\n]*)|(?:/\*[^*]*\*+(?:[^/*][^*]*\*+)*/))+)"_regex;
    auto a1 = run< ast1 >(" 0123  @\t12E-1\nx rest", skipper);
    std::cout << a1.a << ' ' << a1.s.x << ' ' << a1.s.c << std::endl;
    auto a2 = run< ast2 >("333 -3.3 11", skipper);
    (void)a2;
    //auto a3 = run< Y >("2.4", skipper);
    //(void)a3;
    auto a4 = run< Z >("2.333", skipper);
    std::cout << a4.d << std::endl;
    auto v = run< versatile::versatile< char, double > >("2.1", skipper);
    std::cout << v << std::endl;
    auto v2 = run< versatile::versatile< double, char > >("2.1", skipper);
    std::cout << v2 << std::endl;
    auto v3 = run< versatile::variant< decltype("AS"_s), decltype("SA"_s), decltype("SAA"_s) > >("SA", skipper);
    std::cout << v3 << ' ' << v3.which() << std::endl;
    auto v4 = run< versatile::versatile< ast::binary_expression, ast::operand > >("-1 * +1", skipper);
    std::cout << v4 << std::endl;
    auto c = run< int >("-123");
    std::cout << c << std::endl;
    std::cout << "--------------" << std::endl;
    auto i = run< intercept< int, double > >("12.3");
    std::cout << static_cast< double >(i) << std::endl;
    std::cout << "--------------" << std::endl;
#endif
    using namespace parget;
    {
        struct { int a; int b; } s = std::add_const_t< tuple< int, int > >(2, 8);
        std::cout << s.a << ' ' << s.b << std::endl;
        int a_ = tuple< int >(232);
        std::cout << a_ << std::endl;
        struct { int a; } s1 = tuple< int >(333);
        std::cout << s1.a << std::endl;
        struct S { S(int a) { std::cout << a << std::endl; } } s2 = tuple< int >(444);
        (void)s2;
    }
    {
        struct { S< 0 > && s0; S< 1 > && s1; S< 2 > && s2; S< 3 > && s3; } s = tuple_cat(forward_as_tuple(S< 0 >{}, S< 1 >{}), make_tuple(S< 2 >{}), make_tuple(S< 3 >{}));
        (void)s;
        {
            auto t = make_tuple(1);
            static_assert(std::is_same_v< decltype(forward_element< 0 >(t)), int && >);

            static_assert(std::is_same_v< decltype(get< 0 >(t)), int & >);
            static_assert(std::is_same_v< decltype(get< 0 >(std::as_const(t))), int const & >);
            static_assert(std::is_same_v< decltype(get< 0 >(std::move(t))), int && >);
            static_assert(std::is_same_v< decltype(get< 0 >(std::move(std::as_const(t)))), int const && >);
        }
        {
            int k{};
            auto t = make_tuple(k);
            static_assert(std::is_same_v< decltype(get< 0 >(t)), int & >);
            static_assert(std::is_same_v< decltype(get< 0 >(std::as_const(t))), int const & >);
            static_assert(std::is_same_v< decltype(get< 0 >(std::move(t))), int && >);
            static_assert(std::is_same_v< decltype(get< 0 >(std::move(std::as_const(t)))), int const && >);
        }
        {
            auto t = forward_as_tuple(1);
            static_assert(std::is_same_v< decltype(forward_element< 0 >(t)), int && >);

            static_assert(std::is_same_v< decltype(get< 0 >(t)), int & >);
            static_assert(std::is_same_v< decltype(get< 0 >(std::as_const(t))), int & >);
            static_assert(std::is_same_v< decltype(get< 0 >(std::move(t))), int && >);
            static_assert(std::is_same_v< decltype(get< 0 >(std::move(std::as_const(t)))), int && >);
        }
        {
            int k{};
            auto t = forward_as_tuple(k);
            static_assert(std::is_same_v< decltype(forward_element< 0 >(t)), int & >);

            static_assert(std::is_same_v< decltype(get< 0 >(t)), int & >);
            static_assert(std::is_same_v< decltype(get< 0 >(std::as_const(t))), int & >);
            static_assert(std::is_same_v< decltype(get< 0 >(std::move(t))), int && >);
            static_assert(std::is_same_v< decltype(get< 0 >(std::move(std::as_const(t)))), int && >);
        }
    }
    PP;
    return EXIT_SUCCESS;
}
