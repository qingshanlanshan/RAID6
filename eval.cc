#include <fstream>
#include <chrono>
#include <cstring>
#include <iostream>
#include <vector>
#include "include/RAID6.hpp"

using namespace std;

int main() {
    // Declare variables only once outside the test cases
    auto raid6_list = vector<RAID6::RAID6 *>();
    char data[8192];
    memset(data, 0, 8192);

    // Test 1: num_disk & put/get time per block
    ofstream test1_file("output_num_disks.csv");
    if (!test1_file.is_open()) {
        cerr << "Error opening file!" << endl;
        return 1;
    }
    test1_file << "num_disks,put_time_per_block,get_time_per_block\n";

    for (int num_disk = 3; num_disk <= 10; num_disk++) {
        RAID6::RAID6 *raid6 = new RAID6::RAID6();
        raid6->init("data_" + to_string(num_disk) + "/", num_disk, 10, 4096);
        raid6_list.push_back(raid6);
    }

    for (auto &raid6 : raid6_list) {
        // Measure put time
        auto start = chrono::steady_clock::now();
        for (int disk = 0; disk < raid6->num_disks; disk++) {
            raid6->put(0, disk, 4096, data);
        }
        auto end = chrono::steady_clock::now();
        auto put_time_per_block = chrono::duration_cast<chrono::microseconds>(end - start).count() / raid6->num_disks;

        // Measure get time
        start = chrono::steady_clock::now();
        for (int disk = 0; disk < raid6->num_disks; disk++) {
            raid6->get(disk, 0, 4096, data);
        }
        end = chrono::steady_clock::now();
        auto get_time_per_block = chrono::duration_cast<chrono::microseconds>(end - start).count() / raid6->num_disks;

        // Write num_disks, put_time_per_block, get_time_per_block to CSV
        test1_file << raid6->num_disks << "," << put_time_per_block << "," << get_time_per_block << "\n";
    }

    test1_file.close();

    // Clean up dynamically allocated RAID6 objects for Test 1
    for (auto &raid6 : raid6_list) {
        delete raid6;
    }
    raid6_list.clear();  // Clear the vector to reuse it in other tests

    // Test 2: block_size & put/get time per byte
    ofstream test2_file("output_block_size.csv");
    if (!test2_file.is_open()) {
        cerr << "Error opening file!" << endl;
        return 1;
    }
    test2_file << "block_size,put_time_per_byte,get_time_per_byte\n";

    for (int block_size = 8; block_size <= 8192; block_size *= 2) {
        RAID6::RAID6 *raid6 = new RAID6::RAID6();
        raid6->init("data_block_" + to_string(block_size) + "/", 6, 10, block_size);
        raid6_list.push_back(raid6);
    }

    for (auto &raid6 : raid6_list) {
        // Measure put time
        auto start = chrono::steady_clock::now();
        for (int i = 0; i < 1000; ++i) {
            for (int disk = 0; disk < raid6->num_disks; disk++) {
                raid6->put(disk, 0, raid6->block_size, data);
            }
        }
        auto end = chrono::steady_clock::now();
        auto put_time_per_byte = chrono::duration_cast<chrono::microseconds>(end - start).count() / raid6->block_size;

        // Measure get time
        start = chrono::steady_clock::now();
        for (int i = 0; i < 1000; ++i) {
            for (int disk = 0; disk < raid6->num_disks; disk++) {
                raid6->get(disk, 0, raid6->block_size, data);
            }
        }
        end = chrono::steady_clock::now();
        auto get_time_per_byte = chrono::duration_cast<chrono::microseconds>(end - start).count() / raid6->block_size;

        // Write block_size, put_time_per_byte, get_time_per_byte to CSV
        test2_file << raid6->block_size << "," << put_time_per_byte << "," << get_time_per_byte << "\n";
    }

    test2_file.close();

    /*// Clean up dynamically allocated RAID6 objects for Test 2
    for (auto &raid6 : raid6_list) {
        delete raid6;
    }
    raid6_list.clear();  // Clear the vector to reuse it in the next test*/

    // Test 3: reconstruct time per byte vs block size
    ofstream test3_file("output_recovery_time.csv");
    if (!test3_file.is_open()) {
        cerr << "Error opening file!" << endl;
        return 1;
    }
    test3_file << "block_size,test_case,recover_time_per_byte\n";

      auto recover_blocks = vector<vector<pair<int,int>>>{
        {{0,0}}, // one data block
        {{0,0}, {1,0}}, // two data blocks
        {{4,0}}, // parity p
        {{5,0}}, // parity q
        {{4,0}, {5,0}}, // two parity blocks
        {{0,0}, {4,0}}, // one data block and parity p
        {{0,0}, {5,0}}, // one data block and parity q
        };
      for (auto &raid6 : raid6_list)
      {
          for (int test_case=0;test_case<recover_blocks.size();++test_case)
          {
              auto start = chrono::steady_clock::now();
              for (int i=0;i<1000;++i)
                raid6->recover(recover_blocks[test_case]);
              auto end = chrono::steady_clock::now();
              auto recover_time_per_byte = chrono::duration_cast<chrono::microseconds>(end - start).count() / raid6->block_size;
              cout << "block_size: " << raid6->block_size << " case: " << test_case << " recover time per KB: " << recover_time_per_byte << "us" << endl;
              // Write block_size, test_case, recover_time_per_byte to CSV
              test3_file << raid6->block_size << "," << test_case << "," << recover_time_per_byte << "\n";
        
          }
      }

    test3_file.close();

    // Clean up dynamically allocated RAID6 objects for Test 3
    for (auto &raid6 : raid6_list) {
        delete raid6;
    }

    return 0;
}

