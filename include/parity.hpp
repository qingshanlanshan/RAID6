#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <cassert>

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
        Parity(int num_disks)
        {
            generate_gf_tables();
            rs_coefficients[0] = 0x01;
            for(int i=1;i<256;++i)
            {
                rs_coefficients[i] = gf_multiply(0x02,rs_coefficients[i-1]);
            }
        }
        
        inline void XOR_block(char* a, char* b, size_t len, char* result)
        {
            for (int i = 0; i < len; i++)
            {
                result[i] = a[i] ^ b[i];
            }
        }
        // Function to multiply using the precomputed tables
        inline unsigned char gf_multiply(unsigned char a, unsigned char b)
        {
            return gf_table[a][b];
        }
        inline char gf_inverse(char a)
        {
            if (a == 0)
            {
                return 0;
            }
            int res=a;
            for(int i=0;i<253;++i)
            {
                res = gf_multiply(res,a);
            }
            return res;
        }
        inline void gf_multiply_byte_block(char* a, char b, size_t len, char* result)
        {
            for (int i = 0; i < len; i++)
            {
                result[i] = gf_multiply(a[i], b);
            }
        }
        unsigned char gf_pow_02(int n)
        {
            while (n < 0)
            {
                n += 255;
            }
            n %= 255;
            return rs_coefficients[n];
        }
        void cal_XOR_parity(size_t block_size, vector<char *> data, char *parity)
        {
            assert(data.size() > 0);
            memset(parity, 0, block_size);
            for (int i = 0; i < data.size(); i++)
            {
                for (int j = 0; j < block_size; j++)
                {
                    parity[j] ^= data[i][j];
                }
            }
        }
        void update_XOR_parity(size_t len, char *old_data, char *new_data, char *parity)
        {
            for (int byte = 0; byte < len; ++byte)
            {
                parity[byte] ^= (old_data[byte] ^ new_data[byte]);
            }
        }

        void cal_RS_parity(size_t len, vector<char *> data, char *parity) {
            memset(parity, 0, len);
            for (int byte = 0; byte < len; ++byte)
            {
                for (int i = 0; i < data.size(); i++)
                {
                    int coeff = rs_coefficients[i];
                    unsigned char contribution = gf_multiply(coeff, data[i][byte]);
                    parity[byte] ^= contribution;
                }
            }
        }

        void update_RS_parity(size_t len, char *old_data, char *new_data, char *parity, int rs_index = 0)
        {
            int coeff = rs_coefficients[rs_index];
            for (int byte = 0; byte < len; ++byte)
            {
                unsigned char old_contribution = gf_multiply(coeff, old_data[0]);
                unsigned char new_contribution = gf_multiply(coeff, new_data[0]);
                parity[byte] ^= (old_contribution ^ new_contribution);
            }
        }

        // calculate parity for a row of data blocks
        void calculate_parity(string policy, size_t len, vector<char *> data, char *parity)
        {
            if (policy == "XOR")
            {
                cal_XOR_parity(len, data, parity);
            }
            else if (policy == "RS")
            {
                cal_RS_parity(len, data, parity);
            }
        }

        // update parity block when a data block is updated
        // rs_index is the data block index in the row
        void update_block_parity(string policy, size_t len, char *old_data, char *new_data, char *parity, int rs_index = 0)
        {
            if (policy == "XOR")
            {
                update_XOR_parity(len, old_data, new_data, parity);
            }
            else if (policy == "RS")
            {
                update_RS_parity(len, old_data, new_data, parity, rs_index);
            }
        }

    private:
        // GF(2^8) primitive polynomial for RAID-like systems
        const unsigned char GF_2_8_POLY = 0x1D;

        // Precomputed multiplication tables for GF(2^8)
        unsigned char gf_table[256][256];

        int rs_coefficients[256];

        // Function to generate precomputed multiplication tables for GF(2^8)
        void generate_gf_tables()
        {
            for (unsigned int x = 0; x < 256; ++x)
            {
                for (unsigned int y = 0; y < 256; ++y)
                {
                    unsigned char a = x;
                    unsigned char b = y;
                    unsigned char product = 0;

                    // Actual GF(2^8) multiplication logic
                    while (b)
                    {
                        if (b & 1)
                        {
                            product ^= a;
                        }
                        if (a & 0x80)
                        {
                            a = (a << 1) ^ GF_2_8_POLY;
                        }
                        else
                        {
                            a <<= 1;
                        }
                        b >>= 1;
                    }

                    gf_table[x][y] = product;
                }
            }
        }

    };
}
