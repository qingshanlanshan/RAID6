# RAID6
Course Assignment of CE7490

This is a toy project implementing RAID6. Folders are used instead of real storage nodes.

This is a header-only library. Include the headers and run the example code below.

```
#include <cstring>
#include <iostream>
#include "include/RAID6.hpp"

using namespace std;
int main()
{
    // init
    RAID6::RAID6 raid6;
    int block_size=16;
    raid6.init("data/", 6, 6, block_size);

    // write data
    char data_ones[block_size];
    memset(data_ones, 0x01, block_size);
    char data_ff[block_size];
    memset(data_ff, 0xff, block_size);
    raid6.put(0, 0, block_size, data_ones);
    raid6.put(1, 0, block_size, data_ff);
    cout << "disk 0 block 0: ";
    for (int i = 1; i < block_size; i++)
    {
        cout << (int)data_ones[i] << " ";
    }
    cout << endl;
    cout << "disk 1 block 0: ";
    for (int i = 0; i < block_size; i++)
    {
        cout << (int)data_ff[i] << " ";
    }
    cout << endl;

    cout<<"introduce error"<<endl;
    char err[1] = {0};
    raid6.put_no_parity(0, 0, 1, err);
    raid6.get(0, 0, block_size, data_ones);
    cout << "disk 0 block 0: ";
    for (int i = 0; i < block_size; i++)
    {
        cout << (int)data_ones[i] << " ";
    }
    cout << endl;

    raid6.put_no_parity(1, 0, 1, err);
    raid6.get(1, 0, block_size, data_ones);
    cout << "disk 1 block 0: ";
    for (int i = 0; i < block_size; i++)
    {
        cout << (int)data_ones[i] << " ";
    }
    cout << endl;

    cout<<"recover"<<endl;

    raid6.recover({{0, 0}, {1, 0}});
    raid6.get(0, 0, block_size, data_ones);
    cout << "disk 0 block 0: ";
    for (int i = 0; i < block_size; i++)
    {
        cout << (int)data_ones[i] << " ";
    }
    cout << endl;
    raid6.get(1, 0, block_size, data_ones);
    cout << "disk 1 block 0: ";
    for (int i = 0; i < block_size; i++)
    {
        cout << (int)data_ones[i] << " ";
    }
    cout << endl;
    return 0;
}
```
