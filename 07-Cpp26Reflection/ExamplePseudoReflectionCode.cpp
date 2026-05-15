#include <iostream>

// The True "Old World": Pre-C++11 Manual Runtime Reflection
struct LegacyFieldMetaData {
    const char* name;
    const char* type;
    size_t offset;
};

class LegacyComponent {
public:
    int horsepower;
    float temperature;

    LegacyComponent(int hp, float temp) : horsepower(hp), temperature(temp) {}                                 

    // Pre-C++11: We had to manually calculate memory offsets 
    // and store them in a raw static array.
    static const LegacyFieldMetaData* get_metadata() {
        static const LegacyFieldMetaData meta[] = {
            {"horsepower", "int", offsetof(LegacyComponent, horsepower)},
            {"temperature", "float", offsetof(LegacyComponent, temperature)},
            {NULL, NULL, 0} // Null terminator for the array loop
        };
        return meta;
    }
};

int main() {
    LegacyComponent comp(450, 90.5f);
    
    std::cout << "--- Pre-C++11 Architectural Inspection ---\n";
    const LegacyFieldMetaData* field = LegacyComponent::get_metadata();
    
    while (field->name != NULL) {
        // We had to use raw byte pointer math to get the values at runtime!
        const char* base_ptr = reinterpret_cast<const char*>(&comp);
        
        if (strcmp(field->type, "int") == 0) {
            int value = *reinterpret_cast<const int*>(base_ptr + field->offset);
            std::cout << "[Field] " << field->name << " = " << value << "\n";
        } 
        else if (strcmp(field->type, "float") == 0) {
            float value = *reinterpret_cast<const float*>(base_ptr + field->offset);
            std::cout << "[Field] " << field->name << " = " << value << "\n";
        }
        field++;
    }
    return 0;
}