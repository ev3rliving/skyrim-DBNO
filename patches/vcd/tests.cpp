#include "dbno_geometry.hpp"
#include <cmath>
#include <cstdio>
#include <initializer_list>
int main() {
    for (float center : {0.0f, 0.83f, 2.0f}) {
        for (float radius : {0.2f, 0.28f, 0.4f}) {
            const auto bottom = DBNO::BottomEndpoint(center, 0.54f, 0.286f, radius);
            if (std::abs((bottom-radius) - (center-0.54f-0.286f)) > 0.00001f) return 1;
        }
    }
    std::puts("Nine crawl geometry cases passed: bottom contact plane preserved.");
}
