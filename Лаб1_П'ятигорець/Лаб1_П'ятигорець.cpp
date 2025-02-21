#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <algorithm>

using namespace std;

int randGen() {
	return rand() % 1001;
}

int NullGen() {
	return 0;
}

void MultyThreadFunc(int start, int end, vector<int> A, vector<int> B, int k, vector<int>& C) {
	for (int i = start; i < end; i++) {
		C[i] = A[i] - k * B[i];
	}
}

bool Test();

int main() {
	int arraySizes[5] = { 512, 5120, 10240, 102400, 1024000 };
	int threadNums[7] = {2, 4, 8, 16, 32, 64, 128};
	if (Test()) {
		return 0;
	}
	for (const auto& size : arraySizes) {
		cout << "Array size: " << size << endl;
		vector<int> arrayA(size);
		vector<int> arrayB(size);
		vector<int> arrayC(size);
		int k = rand() % 10;
		generate(arrayA.begin(), arrayA.end(), randGen);
		generate(arrayB.begin(), arrayB.end(), randGen);
		
		auto start = chrono::high_resolution_clock::now();
		MultyThreadFunc(0, size, arrayA, arrayB, k, arrayC);
		auto end = chrono::high_resolution_clock::now();
		auto time = chrono::duration_cast<chrono::nanoseconds>(end - start).count() * 1e-9;
		cout << "One thread time: " << time << " seconds, " << endl;
		
		for (const auto& threadNum : threadNums) {
			generate(arrayC.begin(), arrayC.end(), NullGen);
			vector<thread> Threads(threadNum);
			int rowsForOneThread = size / threadNum;
			
			auto start = chrono::high_resolution_clock::now();
			for (int i = 0; i < threadNum; i++) {

				int startRow = rowsForOneThread * i;
				int endRow = startRow + rowsForOneThread;

				Threads.push_back(thread(MultyThreadFunc, startRow, endRow, ref(arrayA), ref(arrayB), k, ref(arrayC)));
			}
			for (auto& t : Threads) {
				if (t.joinable()) {
					t.join();
				}
			}
			auto end = chrono::high_resolution_clock::now();
			auto time = chrono::duration_cast<chrono::nanoseconds>(end - start).count() * 1e-9;
			cout << threadNum << " thread time: " << time << " seconds, " << endl;
			
		}
		cout << endl << endl;
	}
	return 0;
}

bool Test() {
	const int size = 64;
	vector<int> arrayA(size);
	vector<int> arrayB(size);
	vector<int> arrayC(size);
	vector<int> arrayCpred(size);
	int k = rand() % 10;
	generate(arrayA.begin(), arrayA.end(), randGen);
	generate(arrayB.begin(), arrayB.end(), randGen);

	for (int i = 0; i < size; i++) {
		arrayCpred[i] = arrayA[i] - k * arrayB[i];
	}

	int threadsnum = 4;
	vector<thread> Threads(threadsnum);
	int rowsForOneThread = size / threadsnum;

	for (int i = 0; i < threadsnum; i++) {
		int startRow = rowsForOneThread * i;
		int endRow = startRow + rowsForOneThread;

		Threads.push_back(thread(MultyThreadFunc, startRow, endRow, ref(arrayA), ref(arrayB), k, ref(arrayC)));
	}
	for (auto& t : Threads) {
		if (t.joinable()) {
			t.join();
		}
	}
	if (arrayC == arrayCpred) {
		cout << "Everything working!" << endl;
		return 0;
	}
	else {
		cout << "Arrays are not equal!" << endl;
		return 1;
	}
}

/*for (const auto& i : arrayC) {
	cout << i << " ";
}
cout << endl;*/
