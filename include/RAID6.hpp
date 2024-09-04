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

        int init(string path, int num_disks, int block_size)
        {
            if (path.back() != '/')
            {
                path += "/";
            }
            this->path = path;
            this->num_disks = num_disks;
            this->block_size = block_size;

            create_folders(path, num_disks);

            // write config file
            fstream config_file(get_config_path(), std::ios::out);
            if (!config_file.is_open())
            {
                cerr << "Error: failed to open config file" << endl;
                return -1;
            }
            config_file << num_disks << endl;
            config_file << block_size << endl;
            config_file.close();
            return 0;
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
            config_file >> block_size;
            config_file.close();
            return 0;
        }

        int recover(int disk)
        {
            return 0;
        }
        int check()
        {
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
            while(data_len > 0)
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
        // to be tested
        int put(int disk, size_t position, int data_len, char *data)
        {
            // get the starting block and offset
            int block, offset;
            data_position_to_block_offset(disk, position, block, offset);

            // start writing data
            int data_offset = 0;
            while(data_len > 0)
            {
                int len = std::min(data_len, block_size - offset);

                // load old data
                char old_data[block_size];
                read(disk, block, offset, len, old_data);

                // calculate parity
                for(int policy=0; policy<2; policy++)
                {
                    char old_parity[block_size];
                    read(get_parity_disk(block, policy), block, offset, len, old_parity);
                    Parity::calculate_parity(policies[policy], len, {old_data, data+data_offset}, old_data);
                    Parity::calculate_parity(policies[policy], len, {old_parity, old_data}, old_parity);
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

    private:
        string path;
        int num_disks;
        int block_size;
        string policies[2] = {"XOR", "RS"};
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
            return ((block+1)*(num_disks-1)-policy)%num_disks;
        }

        bool is_parity_block(int disk, int block)
        {
            return get_parity_disk(block, 0) == disk || get_parity_disk(block, 1) == disk;
        }

        void data_position_to_block_offset(int disk, size_t position, int &block, int &offset)
        {
            int offset = position % block_size;
            int num_data_block = position / block_size;
            int block = num_data_block/(num_disks-2)*num_disks;
            int count = num_data_block%(num_disks-2);
            for(int i=0; i<num_disks || count==0; i++)
            {
                block++;
                if(is_parity_block(disk, block))
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
                file.close();
            }
            return 0;
        }

        int write(int disk, int block, int offset, int data_len, char *data)
        {
            if(offset + data_len > block_size)
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
            if(offset + data_len > block_size)
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
    };
}