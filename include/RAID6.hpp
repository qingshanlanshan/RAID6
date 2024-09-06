#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include "parity.hpp"

using std::cerr;
using std::cout;
using std::endl;
using std::fstream;
using std::string;
using std::vector;

namespace RAID6
{
    class RAID6
    {
        // | Stripe | Disk 0 | Disk 1 | Disk 2 | Disk 3 | Disk 4 | Disk 5 |
        // |--------|--------|--------|--------|--------|--------|--------|
        // |   0    |   A0   |   B0   |   C0   |   D0   |   P0   |   Q0   |
        // |   1    |   A1   |   B1   |   C1   |   P1   |   Q1   |   D1   |
        // |   2    |   A2   |   B2   |   P2   |   Q2   |   D2   |   C2   |
        // |   3    |   A3   |   P3   |   Q3   |   D3   |   C3   |   B3   |
        // |   4    |   P4   |   Q4   |   D4   |   C4   |   B4   |   A4   |
        // |   5    |   Q5   |   D5   |   C5   |   B5   |   A5   |   P5   |
    public:
        void print()
        {
            cout << "RAID6" << endl;
            cout << "path: " << path << endl;
            cout << "num_disks: " << num_disks << endl;
            cout << "block_size: " << block_size << endl;
        }

        int init(string path, int num_disks, int num_blocks, int block_size)
        {
            if (path.back() != '/')
            {
                path += "/";
            }
            this->path = path;
            this->num_disks = num_disks;
            this->block_size = block_size;
            this->num_blocks = num_blocks;

            create_folders(path, num_disks);
            parity = new Parity(num_disks);

            // write config file
            fstream config_file(get_config_path(), std::ios::out);
            if (!config_file.is_open())
            {
                cerr << "Error: failed to open config file" << endl;
                return -1;
            }
            config_file << num_disks << endl;
            config_file << num_blocks << endl;
            config_file << block_size << endl;
            config_file.close();
            return 0;
        }

        ~RAID6()
        {
            delete parity;
        }

        // recover the data from the config file
        int load(string path)
        {
            if (path.back() != '/')
            {
                path += "/";
            }
            this->path = path;

            // read config file
            fstream config_file(get_config_path(), std::ios::in);
            if (!config_file.is_open())
            {
                cerr << "Error: failed to open config file" << endl;
                return -1;
            }
            config_file >> num_disks;
            config_file >> num_blocks;
            config_file >> block_size;
            config_file.close();
            return 0;
        }

        // to be tested
        int recover(vector<std::pair<int,int>> block_list, int case_num)
        {
            if (case_num == 1)
            {
                // 1. one data block is missing
                // recover from P
                auto disk=block_list[0].first;
                auto block=block_list[0].second;
                rebuild_single_p(disk, block);
            }
            else if (case_num == 2)
            {
                // 2. one parity block is missing
                // just recalculate the parity
                auto disk=block_list[0].first;
                auto block=block_list[0].second;
                char parity_block[block_size];
                cal_parity(block, 0, parity_block);
                write(get_parity_disk(block, 0), block, 0, block_size, parity_block);
            }
            else if (case_num == 3)
            {
                // 3. two data blocks are missing   
                assert(block_list.size()==2);
                assert(block_list[0].second==block_list[1].second);
                rebuild_double(block_list[0].first, block_list[1].first, block_list[0].second);
            }
            else if (case_num == 4)
            {
                // 4. two parity blocks are missing
                // assert(block_list.size()==2);
                for (int i = 0; i < 2; i++)
                {
                    auto disk=block_list[i].first;
                    auto block=block_list[i].second;
                    char parity_block[block_size];
                    cal_parity(block, i, parity_block);
                    write(get_parity_disk(block, i), block, 0, block_size, parity_block);
                }
            }
            else if (case_num == 5)
            {
                // 5. one data block and one parity block are missing
                assert(block_list.size()==2);
                assert(block_list[0].second==block_list[1].second);
                assert(is_parity_block(block_list[1].first, block_list[1].second));
                // determine the policy of the missing parity block
                int policy=0;
                if(get_parity_disk(block_list[0].second, 0)!=block_list[1].first)
                    policy=1;
                
                char policy_block[block_size];
                if(policy==0)
                {
                    rebuild_single_q(block_list[0].first, block_list[0].second);
                    cal_parity(block_list[1].second, 0, policy_block);
                }
                else
                {
                    rebuild_single_p(block_list[0].first, block_list[0].second);
                    cal_parity(block_list[1].second, 1, policy_block);   
                }
                write(block_list[1].first, block_list[1].second, 0, block_size, policy_block);
            }
            return 0;
        }
        
        int check()
        {
            for (int block = 0; block < num_blocks; ++block)
            {
                vector<char *> data;
                char old_parity_blocks[2][block_size];
                char new_parity_blocks[2][block_size];
                for (int disk = 0; disk < num_disks; ++disk)
                {
                    if (!is_parity_block(disk, block))
                    {
                        char* data_block = new char[block_size];
                        read(disk, block, 0, block_size, data_block);
                        data.push_back(data_block);
                    }
                }
                read(get_parity_disk(block, 0), block, 0, block_size, old_parity_blocks[0]);
                read(get_parity_disk(block, 1), block, 0, block_size, old_parity_blocks[1]);
                parity->calculate_parity(policies[0], block_size, data, new_parity_blocks[0]);
                parity->calculate_parity(policies[1], block_size, data, new_parity_blocks[1]);

                for (int i = 0; i < data.size(); ++i)
                {
                    delete[] data[i];
                }

                // TODO: determine which parity is correct
                for (int i = 0; i < 2; ++i)
                {
                    for (int j = 0; j < block_size; ++j)
                    {
                        if (old_parity_blocks[i][j] != new_parity_blocks[i][j])
                        {
                            cerr << "Error: parity check failed" << endl;
                            return -1;
                        }
                    }
                }
            }
            return 0;
        }

        // wrapper functions for read and write
        // should be able to handle parity and data larger than block size
        int get(int disk, size_t position, int data_len, char *data)
        {
            // get the starting block and offset
            int block, offset;
            data_position_to_block_offset(disk, position, block, offset);

            // start reading data
            int data_offset = 0;
            while (data_len > 0)
            {
                int len = std::min(data_len, block_size - offset);
                read(disk, block, offset, len, data + data_offset);
                data_len -= len;
                data_offset += len;
                block++;
                offset = 0;
            }
            return 0;
        }

        // TODO: handle new block creation, #blocks should be written to config file
        int put(int disk, size_t position, int data_len, char *data)
        {
            // get the starting block and offset
            int block, offset;
            data_position_to_block_offset(disk, position, block, offset);

            // start writing data
            int data_offset = 0;
            while (data_len > 0)
            {
                int len = std::min(data_len, block_size - offset);
                int rs_index = 0;
                for (int i = 0; i < disk; ++i)
                {
                    if (!is_parity_block(i, block))
                        rs_index++;
                }
                // load old data
                char old_data[block_size];
                read(disk, block, offset, len, old_data);

                // calculate parity
                for (int policy = 0; policy < 2; policy++)
                {
                    char old_parity[block_size];
                    read(get_parity_disk(block, policy), block, offset, len, old_parity);
                    parity->update_block_parity(policies[policy], len, old_data, data + data_offset, old_parity, rs_index);
                    write(get_parity_disk(block, policy), block, offset, len, old_parity);
                }

                // write data
                write(disk, block, offset, len, data + data_offset);
                data_len -= len;
                data_offset += len;
                block++;
                offset = 0;
            }
            return 0;
        }

        int put_no_parity(int disk, size_t position, int data_len, char *data)
        {
            write(disk, position / block_size, position % block_size, data_len, data);
            return 0;
        }
    private:
        string path;
        int num_disks;
        int num_blocks;
        int block_size;
        string policies[2] = {"XOR", "RS"};
        Parity *parity;
        string get_disk_path(int disk)
        {
            return path + "disk" + std::to_string(disk);
        }

        string get_config_path()
        {
            return path + "config";
        }

        int get_parity_disk(int block, int policy)
        {
            return ((block + 1) * (num_disks - 1) - 1 + policy) % num_disks;
        }

        bool is_parity_block(int disk, int block)
        {
            return get_parity_disk(block, 0) == disk || get_parity_disk(block, 1) == disk;
        }

        void data_position_to_block_offset(int disk, size_t position, int &block, int &offset)
        {
            offset = position % block_size;
            int num_data_block = position / block_size;
            block = num_data_block / (num_disks - 2) * num_disks;
            int count = num_data_block % (num_disks - 2);
            for (int i = 0; i < num_disks && count != 0; i++)
            {
                block++;
                if (is_parity_block(disk, block))
                    count--;
            }
        }

        int create_folders(string path, int num_disks)
        {
            // delete the directory if it already exists
            string command = "rm -rf " + path;
            system(command.c_str());

            // Create the directory
            command = "mkdir -p " + path;
            system(command.c_str());

            // Create config file
            string config_path = path + "config";
            fstream config_file(config_path, std::ios::out);
            if (!config_file.is_open())
            {
                cerr << "Error: failed to create config file" << endl;
                return -1;
            }

            // Create file for each disk
            for (int i = 0; i < num_disks; i++)
            {
                string disk_path = get_disk_path(i);
                fstream file(disk_path, std::ios::out | std::ios::binary);
                if (!file.is_open())
                {
                    cerr << "Error: failed to create disk" << endl;
                    return -1;
                }
                // write num_blocks of zeros
                char zeros[block_size];
                memset(zeros, 0, block_size);
                for (int j = 0; j < num_blocks; j++)
                {
                    file.write(zeros, block_size);
                }
                file.close();
            }
            return 0;
        }

        int write(int disk, int block, int offset, int data_len, char *data)
        {
            if (offset + data_len > block_size)
            {
                cerr << "Error: offset + data_len is greater than block_size" << endl;
                cerr << "offset: " << offset << " data_len: " << data_len << " block_size: " << block_size << endl;
                return -1;
            }
            // TODO: avoid frequent open and close
            fstream file(get_disk_path(disk), std::ios::in | std::ios::out | std::ios::binary);
            if (!file.is_open())
            {
                cerr << "Error: failed to open disk" << endl;
                return -1;
            }
            file.seekp(block * block_size + offset);
            file.write(data, data_len);
            file.close();
            return 0;
        }
        int read(int disk, int block, int offset, int data_len, char *data)
        {
            if (offset + data_len > block_size)
            {
                cerr << "Error: offset + data_len is greater than block_size" << endl;
                cerr << "offset: " << offset << " data_len: " << data_len << " block_size: " << block_size << endl;
                return -1;
            }

            fstream file(get_disk_path(disk), std::ios::in | std::ios::out | std::ios::binary);
            if (!file.is_open())
            {
                cerr << "Error: failed to open disk" << endl;
                return -1;
            }
            file.seekg(block * block_size + offset);
            file.read(data, data_len);
            file.close();
            return 0;
        }

        int cal_parity(int block, int policy, char *parity_block)
        {
            vector<char *> data;
            for (int i = 0; i < num_disks; ++i)
            {
                if (!is_parity_block(i, block))
                {
                    char* data_block = new char[block_size];
                    if (read(i, block, 0, block_size, data_block))
                        return -1;
                    data.push_back(data_block);
                }
            }
            parity->calculate_parity(policies[policy], block_size, data, parity_block);
            for (int i = 0; i < data.size(); ++i)
            {
                delete[] data[i];
            }
            return 0;
        }

        int rebuild_double(int disk_x, int disk_y, int block)
        {
            // disk idx to data idx
            int idx_x = 0, idx_y = 0;
            vector<char *> data;
            for (int i=0;i<num_disks;++i)
            {
                if (is_parity_block(i, block))
                    continue;
                if (i < disk_x)
                    idx_x++;
                if (i < disk_y)
                    idx_y++;
                char* data_block = new char[block_size];
                if(i==disk_x||i==disk_y)
                {
                    memset(data_block, 0, block_size);
                }
                else
                {
                    if (read(i, block, 0, block_size, data_block))
                        return -1;
                }
                data.push_back(data_block);
            }
            int idx_p = get_parity_disk(block, 0);
            int idx_q = get_parity_disk(block, 1);

            // g^(y-x)
            int coef_yx = parity->gf_pow_02(idx_y - idx_x);
            // g^(-x)
            int coef_x = parity->gf_pow_02(-idx_x);
            // g^(y-x)+01
            int coef_yx_01 = coef_yx ^ 0x01;
            // (g^(y-x)+01)^-1
            int coef_yx_01_inv = parity->gf_inverse(coef_yx_01);

            int A = parity->gf_multiply(coef_yx_01_inv, coef_yx);
            int B = parity->gf_multiply(coef_yx_01_inv, coef_x);
            char data_x[block_size], data_y[block_size];

            char parity_p[block_size], parity_q[block_size];
            read(idx_p, block, 0, block_size, parity_p);
            read(idx_q, block, 0, block_size, parity_q);

            char parity_p_xy[block_size], parity_q_xy[block_size];
            parity->calculate_parity(policies[0], block_size, data, parity_p_xy);
            parity->calculate_parity(policies[1], block_size, data, parity_q_xy);
            
            // D_x = A*(P+P_xy)+B*(Q+Q_xy)
            char middle1[block_size], middle2[block_size];
            parity->XOR_block(parity_p, parity_p_xy, block_size, middle1);
            parity->XOR_block(parity_q, parity_q_xy, block_size, middle2);
            parity->gf_multiply_byte_block(middle1, A, block_size, middle1);
            parity->gf_multiply_byte_block(middle2, B, block_size, middle2);
            parity->XOR_block(middle1, middle2, block_size, data_x);

            // D_y = (P+P_xy)+D_x
            parity->XOR_block(parity_p, parity_p_xy, block_size, middle1);
            parity->XOR_block(middle1, data_x, block_size, data_y);

            write(disk_x, block, 0, block_size, data_x);
            write(disk_y, block, 0, block_size, data_y);

            for (int i = 0; i < data.size(); ++i)
            {
                delete[] data[i];
            }
            return 0;
        }

        // rebuild data from parity P
        int rebuild_single_p(int disk, int block)
        {
            int disk_p = get_parity_disk(block, 0);
            // data other than the broken one
            vector<char *> data;
            for (int i = 0; i < num_disks; ++i)
            {
                if (i != disk && !is_parity_block(i, block))
                {
                    char* data_block = new char[block_size];
                    if (read(i, block, 0, block_size, data_block))
                        return -1;
                    data.push_back(data_block);
                }
            }
            char* parity_block = new char[block_size];
            if (read(disk_p, block, 0, block_size, parity_block))
                return -1;
            data.push_back(parity_block);

            char new_data[block_size];
            parity->calculate_parity(policies[0], block_size, data, new_data);
            for (int i = 0; i < data.size(); ++i)
            {
                delete[] data[i];
            }
            if (write(disk, block, 0, block_size, new_data))
                return -1;
            return 0;
        }
        int rebuild_single_q(int disk, int block)
        {
            int disk_q = get_parity_disk(block, 1);
            vector<char *> data;
            int coef_idx=0, coef_pow=0;
            for (int i = 0; i < num_disks; ++i)
            {
                if (is_parity_block(i, block))
                    continue;
                char* data_block = new char[block_size];
                if(i==disk)
                {
                    memset(data_block, 0, block_size);
                    coef_pow=parity->gf_pow_02(-coef_idx);
                }
                else
                {
                    if (read(i, block, 0, block_size, data_block))
                        return -1;
                }
                data.push_back(data_block);
                coef_idx++;
            }
            // Q_x
            char new_parity[block_size];
            parity->calculate_parity(policies[1], block_size, data, new_parity);
            for (int i = 0; i < data.size(); ++i)
            {
                delete[] data[i];
            }

            // Q
            char parity_block[block_size];
            if (read(disk_q, block, 0, block_size, parity_block))
                return -1;
            
            // Q+Q_x
            parity->XOR_block(parity_block, new_parity, block_size, parity_block);

            // (Q+Q_x)*g^(-x)
            parity->gf_multiply_byte_block(parity_block, coef_pow, block_size, parity_block);

            if (write(disk, block, 0, block_size, parity_block))
                return -1;
            return 0;
        }
    };
}