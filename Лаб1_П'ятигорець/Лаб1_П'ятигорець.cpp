#include <iostream>
#include <iomanip>
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
void OneThread(vector<int> A, vector<int> B, int k, vector<int>& C) {
	for (int i = 0; i < A.size(); i++) {
		C[i] = A[i] - k * B[i];
	}
}

void MultyThreadFunc(int start, int end, vector<int> A, vector<int> B, int k, vector<int>& C) {
	for (int i = start; i < end; i++) {
		C[i] = A[i] - k * B[i];
	}
}

int main() {
	vector<int> arraySizes = { 512, 5120, 10240, 51200, 102400 };
	vector<int> threadNums = { 2, 4, 8, 16, 32, 64, 128 };

	for (const auto& size : arraySizes) {
		cout << "Array size: " << size << endl;
		vector<int> arrayA(size);
		vector<int> arrayB(size);
		int k = rand() % 10;
		generate(arrayA.begin(), arrayA.end(), randGen);
		generate(arrayB.begin(), arrayB.end(), randGen);

		vector<int> arrayC(size);
		
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


/*for (const auto& i : arrayC) {
	cout << i << " ";
}
cout << endl;*/

//, 5120, 10240, 51200, 102400 
// , 8, 16, 32, 64, 128 