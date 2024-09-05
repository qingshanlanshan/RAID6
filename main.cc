#include "include/RAID6.hpp"
#include <iostream>

using namespace std;
int main()
{
    RAID6::RAID6 raid6;
    int block_size=16;
    raid6.init("data/", 6, 6, block_size);

    char data_ones[block_size];
    memset(data_ones, 0x01, block_size);
    char data_ff[block_size];
    memset(data_ff, 0xff, block_size);
    raid6.put(0, 0, block_size, data_ones);
    raid6.put(1, 0, block_size, data_ff);
    for (int i = 0; i < block_size; i++)
    {
        cout << (int)data_ones[i] << " ";
    }
    cout << endl;

    char err[1] = {0};
    raid6.put_no_parity(0, 0, 1, err);
    raid6.get(0, 0, block_size, data_ones);
    for (int i = 0; i < block_size; i++)
    {
        cout << (int)data_ones[i] << " ";
    }
    cout << endl;

    raid6.recover({{0, 0}}, 1);
    raid6.get(0, 0, block_size, data_ones);
    for (int i = 0; i < block_size; i++)
    {
        cout << (int)data_ones[i] << " ";
    }
    cout << endl;
    return 0;
}