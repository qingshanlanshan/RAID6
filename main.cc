#include "include/RAID6.hpp"
#include <iostream>

// test the RAID6 class
int main()
{
    RAID6::RAID6 *raid6 = new RAID6::RAID6();
    raid6->init("RAID_DISK", 6, 10);
    raid6->print();
    delete raid6;
    raid6 = new RAID6::RAID6();
    raid6->load("RAID_DISK");
    raid6->print();
    char data0[10];
    char data1[10];
    char parity[10];

    memset(data0, 0, 10);
    memset(data1, 1, 10);

    RAID6::Parity::XOR_parity(10, {data0, data1}, parity);
    cout << "parity: ";
    for (int i = 0; i < 10; i++)
    {
        cout << (int)parity[i] << " ";
    }
    return 0;
}