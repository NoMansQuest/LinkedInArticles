#include <iostream>
#include <boost/fusion/adapted/struct/adapt_struct.hpp>
#include <boost/fusion/include/algorithm.hpp>

// 1. Define a regular old C++ struct
struct LegacyComponent {
    int horsepower;
    float temperature;
};

// 2. The Boost Magic: You had to manually "register" the struct 
// in the global namespace using preprocessor macros. 
// This generated hidden template specializations under the hood.
BOOST_FUSION_ADAPT_STRUCT(
    LegacyComponent,
    (int, horsepower)
    (float, temperature)
)

// 3. A Pre-C++11 "Functor" used to emulate a loop over the fields                                         
struct PrintField {
    template <typename T>
    void operator()(const T& value) const {
        // We could print the VALUE, but notice the limitation:
        // We couldn't easily print the variable's NAME ("horsepower") 
        // without even more insane macro hacking!
        std::cout << "Field Value: " << value << "\n";
    }
};

int main() {
    LegacyComponent comp = {450, 90.5f};

    std::cout << "--- Pre-C++11 Boost.Fusion Inspection ---\n";

    // 4. Boost.Fusion's "for_each" algorithm unrolls the struct members 
    // at compile-time and passes each one to our functor.
    boost::fusion::for_each(comp, PrintField());

    return 0;
}