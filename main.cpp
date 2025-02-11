#include <iostream>

void fibonacci(int n) {
    long long a = 0, b = 1, temp;
    for (int i = 0; i < n; ++i) {
        std::cout << a << " ";
        temp = a + b;
        a = b;
        b = temp;
    }
    std::cout << std::endl;
}

int main() {
    int n;
    std::cout << "Enter the number of Fibonacci terms: ";
    std::cin >> n;

    if (n <= 0) {
        std::cout << "Please enter a positive integer." << std::endl;
    } else {
        fibonacci(n);
    }

    return 0;
}
