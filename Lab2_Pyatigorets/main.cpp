#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>
#include <algorithm>

using namespace std;

int randGen() {
	return rand() % 1001;
}

int main()
{
	int arraySizes[5] = { 1000, 10000, 100000, 1000000, 10000000 };
	int threadNums[6] = { 2, 8, 16, 32, 64, 128 };
	int count = 0;
	int randNum = rand() % 1001;
	int maxVal = INT32_MIN;

	for (const auto& size : arraySizes) {
		cout << "Array size: " << size << endl;
		vector<int> array(size);
		generate(array.begin(), array.end(), randGen);

		auto start = chrono::high_resolution_clock::now();
		//Func();
		auto end = chrono::high_resolution_clock::now();
		auto time = chrono::duration_cast<chrono::nanoseconds>(end - start).count() * 1e-9;
		cout << "One thread time: " << time << " seconds" << endl;

		for (const auto& threadNum : threadNums) {
			
			vector<thread> Threads(threadNum);
			
			int ElementsForOneThread = size / threadNum + ((size % threadNum == 0) ? 0 : 1);
			auto start = chrono::high_resolution_clock::now();
			for (int i = 0; i < threadNum; i++) {

				int startElement = ElementsForOneThread * i;
				int endElement = (i == threadNum - 1) ? size : startElement + ElementsForOneThread;
				//Threads.push_back(thread());
			}
			for (auto& t : Threads) {
				if (t.joinable()) {
					t.join();
				}
			}
			auto end = chrono::high_resolution_clock::now();
			auto time = chrono::duration_cast<chrono::nanoseconds>(end - start).count() * 1e-9;
			cout << threadNum << " thread time: " << time << " seconds" << endl;

		}
		cout << endl << endl;
	}

	return 0;
}