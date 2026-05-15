#include <iostream>
#include <reflection>
#include <string_view>

// A sample data packet (no macros, no hidden boilerplate, 100% clean)
struct TelemetryPacket {
    int packet_id = 101;
    float velocity = 299.79f;
    bool system_ready = true;
};

// The Universal Serialization Engine
template <typename T>
void serialize_object(const T& obj) {
    // 1. The '^' Operator: Lift the class into the meta-world
    constexpr auto type_meta = ^T;
    
    std::cout << "Serializing: " << std::meta::get_name(type_meta) << " {\n";

    // 2. The 'template for': Unroll the members at compile-time
    template for (constexpr auto field : std::meta::get_members(type_meta)) {
        
        // Ensure we are only looking at variables (fields), not functions
        if constexpr (std::meta::is_variable(field)) {
            
            // 3. The 'typename [: :]' Operator: Extract the type dynamically
            using FieldType = typename [: std::meta::get_type(field) :];
            
            // Print the metadata name
            std::cout << "  \"" << std::meta::get_name(field) << "\": ";
            
            // 4. The '[: :]' Operator: Reach through the mirror to get the value
            // obj.[:field:] compiles down directly to obj.packet_id, obj.velocity, etc.
            const FieldType& value = obj.[:field:];
            
            std::cout << value << ",\n";
        }
    }
    std::cout << "}\n";
}

// 5. The '[:expand:]' Operator: Combining reflection with classic Template Packs
template <auto... FieldMetas, typename T>
void print_field_count_via_pack(const T& obj) {
    // We expand a collection of reflection handles into a real C++ parameter pack
    // allowing us to use a C++17 fold expression to count them.
    constexpr size_t count = (... + (std::meta::is_variable(FieldMetas) ? 1 : 0));
    std::cout << "Total reflective fields parsed: " << count << "\n";
}

int main() {
    TelemetryPacket packet;

    // Execute the full reflection pipeline
    serialize_object(packet);

    // Trigger the pack expansion demonstration
    constexpr auto members = std::meta::get_members(^TelemetryPacket);
    [:expand(members):] >> [&]<auto... M> {
        print_field_count_via_pack<M...>(packet);
    };

    return 0;
}