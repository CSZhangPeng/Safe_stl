﻿#include "vector.hpp"
#include <vector>

void testcase_vector_8()
{
    std::printf("Running testcase8\n\n");
    std::printf("behaviour of saber::vector\n\n");
    {
        saber::vector<int> vec {2, 3, 5, 7};
        int new_element = 9;

        auto insert_pos = vec.cbegin() + 1;
        vec.insert(insert_pos, new_element);

        for (auto it = vec.cbegin(); it != vec.cend(); ++it)
        {
            std::printf("%d ", *it);
        }
        putchar('\n');
    }
    std::printf("behaviour of std::vector\n\n");
    {
        std::vector<int> vec {2, 3, 5, 7};

        auto insert_pos = vec.cbegin() + 1;
        vec.insert(insert_pos, 9);

        for (auto it = vec.cbegin(); it != vec.cend(); ++it)
        {
            std::printf("%d ", *it);
        }
        putchar('\n');
    }
}