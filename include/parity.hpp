#include <iostream>
#include <string>
#include <vector>
#include <fstream>

using std::cerr;
using std::cout;
using std::endl;
using std::fstream;
using std::string;
using std::vector;

namespace RAID6
{
    class Parity
    {
    public:
        static void XOR_parity(int block_size, vector<char *> data, char *parity)
        {
            memcpy(parity, data[0], block_size);
            for (int i = 1; i < data.size(); i++)
            {
                for (int j = 0; j < block_size; j++)
                {
                    parity[j] ^= data[i][j];
                }
            }
        }

        static void calculate_parity(string policy, size_t len, vector<char *> data, char *parity)
        {
            if (policy == "XOR")
            {
                XOR_parity(len, data, parity);
            }
            else if (policy == "RS")
            {
                // TODO
                // urgent
            }
        }
    };
}
