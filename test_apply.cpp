#include <tuple>
#include <iostream>

int main() {
    auto t = std::make_tuple(1, 2);
    // std::apply might be missing in gcc 6.3 c++17 mode
    std::apply([](int a, int b) { std::cout << a+b; }, t);
    return 0;
}
