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
            for (int i = 0; i < block_size; i++)
            {
                parity[i] = 0;
                for (int j = 0; j < data.size(); j++)
                {
                    parity[i] ^= data[j][i];
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
            }
        }
    };
}
