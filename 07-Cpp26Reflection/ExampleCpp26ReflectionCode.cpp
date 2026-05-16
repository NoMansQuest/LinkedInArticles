#include <iostream>
#include <meta>

// IMPORTANT
// ---------------------------------------------------------------------
// This code can be tested in https://compiler-explorer.com/.
// In order to do so, please follow these steps:
//      1) Select 'x86-64 gcc (trunk)' as your compiler
//      2) Use the following compiler arguments: -std=c++26 -freflection

// Clean sample data packet
struct TelemetryPacket {
    int packet_id = 101;
    float velocity = 299.79f;
    bool system_ready = true;
};

// Universal serialization using C++26 reflection
template <typename T>
void serialize_object(const T& obj) {
    constexpr auto type_meta = ^^T; // reflect the type (noticed the 'cat-ears' sign?)
    constexpr auto ctx = std::meta::access_context::unchecked();
    constexpr static auto members = std::define_static_array(std::meta::nonstatic_data_members_of(type_meta, ctx));

    std::cout << "Serializing: " << std::meta::identifier_of(type_meta) << " {\n";

    // 'template for' expansion, this loop is unrolled during compile.
    template for (constexpr auto field : members)
    {        
        std::cout << "  \"" << std::meta::identifier_of(field) << "\": ";
        
        // Splice: access the member through the reflection
        const auto& value = obj.[:field:];
        std::cout << value << ",\n";
    }

    std::cout << "}\n";
}

int main() {
    TelemetryPacket packet{};
    serialize_object(packet);
}