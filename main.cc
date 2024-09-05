#include "include/RAID6.hpp"
#include <iostream>

// test the RAID6 class
int main()
{
    RAID6::Parity parity(4);
    char data0[4] = {0b00000001, 0b00000010, 0b00000011, 0b00000100};
    char data1[4] = {0b00000101, 0b00000110, 0b00000111, 0b00001000};
    char data2[4];
    parity.calculate_parity("XOR", 4, {data0,data1}, data2);
    for(int i=0; i<4; i++)
    {
        std::cout << (int)data2[i] << " ";
    }
    std::cout << std::endl;
    parity.calculate_parity("RS", 4, {data0,data1}, data2);
    for(int i=0; i<4; i++)
    {
        std::cout << (int)data2[i] << " ";
    }
    std::cout << std::endl;
    return 0;
}