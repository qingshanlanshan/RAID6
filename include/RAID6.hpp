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

        // untested
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
        int get();
        int put();

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
            return 0;
        }
    };
}