#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>
#include <algorithm>
#include <ctime>

using namespace std;

void OneThreadFunc(int rand, int& count, int& max, vector<int>& array) {
	for (int i = 0; i < array.size(); i++) {
		if (array[i] > rand) count++;
		if (array[i] > max) max = array[i];
	}
}

void MutexFunc(int rand, int start, int end,  int& count, int& max, vector<int>& array, mutex& mutCount, mutex& mutMax) {

	for (int i = start; i < end; i++) {
		if (array[i] > rand) {
			//mutCount.lock();
			lock_guard<mutex> lg(mutCount);
			count++;
			//mutCount.unlock();
		}
		lock_guard<mutex> lg(mutMax);
		if (array[i] > max) max = array[i];
	}
}

void СMPXCHGFunc() {

}

int main()
{
	srand(time(nullptr));
	int arraySizes[5] = { 1000, 10000, 100000, 1000000, 10000000 };
	int threadNums[6] = { 2, 8, 16, 32, 64, 128 };
	int count = 0;
	int randNum = rand() % 10001;
	int maxVal = INT32_MIN;
	cout << "Rand number = " << randNum << endl << endl;

	for (const auto& size : arraySizes) {
		cout << "Array size: " << size << endl;
		vector<int> array(size);
		for (int i = 0; i < size; i++)
			array[i] = rand() % 10001;

		auto start = chrono::high_resolution_clock::now();
		OneThreadFunc(randNum, count, maxVal, ref(array));
		auto end = chrono::high_resolution_clock::now();
		auto time = chrono::duration_cast<chrono::nanoseconds>(end - start).count() * 1e-9;
		cout << "One thread time: " << time << " seconds, ";
		cout << "max = " << maxVal << ", count = " << count << endl;
		for (const auto& threadNum : threadNums) {
			count = 0;
			maxVal = INT32_MIN;
			mutex MUTcount, MUTmax;
			vector<thread> Threads(threadNum);
			int ElementsForOneThread = size / threadNum + ((size % threadNum == 0) ? 0 : 1);

			auto start = chrono::high_resolution_clock::now();
			for (int i = 0; i < threadNum; i++) {
				int startElement = ElementsForOneThread * i;
				int endElement = (i == threadNum - 1) ? size : startElement + ElementsForOneThread;
				Threads.push_back(thread(MutexFunc, randNum, startElement, endElement, ref(count), ref(maxVal), ref(array), ref(MUTcount), ref(MUTmax)));
			}
			for (auto& t : Threads) {
				if (t.joinable()) {
					t.join();
				}
			}
			auto end = chrono::high_resolution_clock::now();
			auto time = chrono::duration_cast<chrono::nanoseconds>(end - start).count() * 1e-9;
			cout << threadNum << " thread mutex time: " << time << " seconds, ";
			cout << "max = " << maxVal << ", count = " << count << endl;
		}
		for (const auto& threadNum : threadNums) {
			count = 0;
			maxVal = INT32_MIN;
			vector<thread> Threads(threadNum);
			int ElementsForOneThread = size / threadNum + ((size % threadNum == 0) ? 0 : 1);

			auto start = chrono::high_resolution_clock::now();
			for (int i = 0; i < threadNum; i++) {
				int startElement = ElementsForOneThread * i;
				int endElement = (i == threadNum - 1) ? size : startElement + ElementsForOneThread;
				//Threads.push_back(thread(СMPXCHGFunc()));
			}
			for (auto& t : Threads) {
				if (t.joinable()) {
					t.join();
				}
			}
			auto end = chrono::high_resolution_clock::now();
			auto time = chrono::duration_cast<chrono::nanoseconds>(end - start).count() * 1e-9;
			cout << threadNum << " thread CMPXCHG time: " << time << " seconds, ";
			cout << "max = " << maxVal << ", count = " << count << endl;
		}
		cout << endl << endl;
	}

	return 0;
}