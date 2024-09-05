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
            for (int i = 0; i < num_disks - 2; i++)
            {
                rs_coefficients.push_back(gf_pow_02(i));
            }
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
            // TODO
            // urgent
            assert(data.size() == rs_coefficients.size());
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
                // TODO
                // urgent
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
                // TODO
                // urgent
                update_RS_parity(len, old_data, new_data, parity, rs_index);
            }
        }

    private:
        // GF(2^8) primitive polynomial for RAID-like systems
        const unsigned char GF_2_8_POLY = 0x1D;

        // Precomputed multiplication tables for GF(2^8)
        unsigned char gf_table[256][256];

        vector<int> rs_coefficients;

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

        // Function to multiply using the precomputed tables
        inline unsigned char gf_multiply(unsigned char a, unsigned char b)
        {
            return gf_table[a][b];
        }

        unsigned char gf_pow_02(unsigned int n)
        {
            unsigned char result = 0x01; // 0x02^0 = 1 in GF(2^8)
            unsigned char base = 0x02;
            for (unsigned int i = 0; i < n; ++i)
            {
                result = gf_multiply(result, base);
            }
            return result;
        }

        // Function to calculate Reed-Solomon (RS) parity using precomputed tables
        void calculate_rs_parity(const std::vector<std::vector<unsigned char>> &stripe_data,
                                 std::vector<unsigned char> &rs_parity,
                                 size_t block_size,
                                 const std::vector<unsigned char> &rs_coefficients)
        {
            rs_parity.assign(block_size, 0); // Initialize RS parity block to zero

            // Iterate over each byte in the block
            for (size_t byte_idx = 0; byte_idx < block_size; ++byte_idx)
            {
                // Calculate RS parity byte by byte across all data disks
                for (size_t disk_idx = 0; disk_idx < stripe_data.size(); ++disk_idx)
                {
                    unsigned char data_byte = stripe_data[disk_idx][byte_idx];
                    unsigned char coefficient = rs_coefficients[disk_idx];
                    unsigned char product = gf_multiply(coefficient, data_byte);
                    rs_parity[byte_idx] ^= product; // XOR for GF addition
                }
            }
        }
    };
}
