#include <iostream>
#include <meta>  // C++26 reflection

// IMPORTANT
// ---------------------------------------------------------------------
// This code can be tested in https://compiler-explorer.com/.
// In order to do so, please follow these steps:
//      1) Select 'x86-64 gcc (trunk)' as your compiler
//      2) Use the following compiler arguments: -std=c++26 -freflection

class EngineComponent {
public:
    // Fields (Data Members)
    int horsepower = 450;
    float temperature = 90.5f;
    bool turbo_engaged = false;

    // Methods (Member Functions)
    void start_engine() {}
    void stop_engine() {}

    // The "X-Ray" function - Architectural Inspection using Reflection
    void inspect_internals() const {
        constexpr auto type = ^^EngineComponent;
        constexpr auto ctx = std::meta::access_context::unchecked();
        
        // Note: 'members' must be defined as 'static'
        constexpr static auto members = std::define_static_array(std::meta::members_of(type, ctx));

        std::cout << "--- Architectural Inspection: EngineComponent ---\n";

        template for (constexpr auto m : members) {            
            if constexpr (std::meta::is_nonstatic_data_member(m)) {
                std::cout << "[Field]  " 
                          << std::meta::identifier_of(m)
                          << " (Type: " 
                          << std::meta::display_string_of(std::meta::type_of(m)) 
                          << ")\n";
            }
            else if constexpr (std::meta::is_function(m)) {
                // Skip special member functions (constructors, destructor, etc.)
                if constexpr (std::meta::is_special_member_function(m)) {
                    continue;
                }

                if constexpr (std::meta::has_identifier(m)) {
                    std::cout << "[Method] " 
                              << std::meta::identifier_of(m) << "()\n";
                } else {
                    std::cout << "[Method] <unnamed or special function>\n";
                }
            }
        }
    }
};

int main() {
    EngineComponent comp;
    comp.inspect_internals();
}