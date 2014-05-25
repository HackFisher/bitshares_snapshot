#define BOOST_TEST_MODULE MultiInherientTest
#include <boost/test/unit_test.hpp>
#include <boost/program_options.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/scope_exit.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/range/algorithm/find.hpp>
#include <boost/iterator/counting_iterator.hpp>

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/graph/exterior_property.hpp>
#include <boost/graph/floyd_warshall_shortest.hpp>
#include <boost/graph/eccentricity.hpp>
#include <boost/graph/connected_components.hpp>
#include <boost/graph/copy.hpp>

class A {
public:
    virtual void testA() = 0;

    void write() { testA();}
};

class B {
public:
    virtual void testB() = 0;

    void read() { testB();}
};

class C: public virtual A, public virtual B {
};

class D: public virtual C
{
    virtual void testA() {}

    virtual void testB() {}
};

BOOST_AUTO_TEST_CASE( test_multi_inherit )
{
    D d;
    d.write();
    d.read();
}