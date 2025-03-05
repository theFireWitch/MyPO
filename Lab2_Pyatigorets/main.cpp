#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>
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
			lock_guard<mutex> lg(mutCount);
			count++;
		}
		lock_guard<mutex> lg(mutMax);
		if (array[i] > max) max = array[i];
	}
}

void СMPXCHGFunc(int rand, int start, int end, atomic<int>& count, atomic<int>& max, vector<int>& array) {
	int expected;
	int New;
	for (int i = start; i < end; i++) {
		if (array[i] > rand) {
			do {
				expected = count.load();
				New = expected + 1;
			} while (!count.compare_exchange_strong(expected, New));
		}
		while ((array[i] > (expected = max.load())) && !max.compare_exchange_strong(expected, array[i])) {};
	}
}

int main()
{
	srand(time(nullptr));
	int arraySizes[5] = { 1000, 10000, 100000, 1000000, 10000000 };
	int threadNums[6] = { 4, 8, 16, 32, 64, 128 };
	int count = 0;
	int randNum = rand() % 10001;
	int maxVal = INT32_MIN;
	cout << "Rand number = " << randNum << endl << endl;

	for (const auto& size : arraySizes) {
		cout << "Array size: " << size << endl;
		vector<int> array(size);
		for (int i = 0; i < size; i++)
			array[i] = rand() % 10001;
		count = 0;
		maxVal = INT32_MIN;

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
				int endElement = (i == threadNum - 1) ? size : (startElement + ElementsForOneThread > size) ? size : startElement + ElementsForOneThread;
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
			atomic <int> atomic_count = 0;
			atomic <int> atomic_maxVal = INT32_MIN;
			vector<thread> Threads(threadNum);
			int ElementsForOneThread = size / threadNum + ((size % threadNum == 0) ? 0 : 1);

			auto start = chrono::high_resolution_clock::now();
			for (int i = 0; i < threadNum; i++) {
				int startElement = ElementsForOneThread * i;
				int endElement = (i == threadNum - 1) ? size : (startElement + ElementsForOneThread > size) ? size : startElement + ElementsForOneThread;
				Threads.push_back(thread(СMPXCHGFunc, randNum, startElement, endElement, ref(atomic_count), ref(atomic_maxVal), ref(array)));
			}
			for (auto& t : Threads) {
				if (t.joinable()) {
					t.join();
				}
			}
			auto end = chrono::high_resolution_clock::now();
			auto time = chrono::duration_cast<chrono::nanoseconds>(end - start).count() * 1e-9;
			cout << threadNum << " thread CMPXCHG time: " << time << " seconds, ";
			cout << "max = " << atomic_maxVal << ", count = " << atomic_count << endl;
		}
		cout << endl << endl;
	}
	return 0;
}