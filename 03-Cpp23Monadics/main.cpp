/*
 * MIT License
 *
 * Copyright (c) 2026 Nasser Ghoseiri

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include <iostream>
#include <optional>
#include <string>

struct User {
    int id;
    std::string role;
};

int main() {    
    // 1. Mock functions returning std::optional
    auto fetch_user = [](int id) -> std::optional<User> {
        if (id == 42) return User{42, "admin"};
        return std::nullopt;
    };

    auto get_permissions = [](const User& u) -> std::optional<std::string> {
        if (u.role == "admin") return std::string("ALL_ACCESS");
        return std::nullopt;
    };

    auto format_token = [](const std::string& perm) {
        return "TOKEN_" + perm;
    };

    // 2. The Monadic Query (Dot Notation / Method Chaining)
    // This is the C++23 way, very similar to LINQ's .Select().SelectMany()
    auto final_token = fetch_user(42)
        .and_then(get_permissions)          // "Bind" - flattens optional<optional<T>>
        .transform(format_token)            // "Map" - automatically wraps result in optional
        .value_or("GUEST_TOKEN");           // Default value if any step returned nullopt

    std::cout << "Result: " << final_token << std::endl;

    return 0;
}