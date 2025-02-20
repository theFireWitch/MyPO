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

void OneThread(vector<int> A, vector<int> B, int k, vector<int>& C) {
	for (int i = 0; i < A.size(); i++) {
		C[i] = A[i] - k * B[i];
	}
}

void MultyThread(int start, int end, vector<int> A, vector<int> B, int k, vector<int>& C) {
	for (int i = 0; i < A.size(); i++) {
		C[i] = A[i] - k * B[i];
	}
}

int main() {
	vector<int> arraySizes = { 512, 5120, 10240, 51200, 102400 };
	vector<int> treadsNum = { 2, 4, 8, 16, 32, 64, 128 };

	for (const auto& size : arraySizes) {
		vector<int> arrayA(size);
		vector<int> arrayB(size);
		vector<int> arrayC(size);
		int k = rand() % 10;
		generate(arrayA.begin(), arrayA.end(), randGen);
		generate(arrayB.begin(), arrayB.end(), randGen);

		auto start = chrono::high_resolution_clock::now();
		OneThread(arrayA, arrayB, k, arrayC);
		auto end = chrono::high_resolution_clock::now();
		auto time = chrono::duration_cast<chrono::nanoseconds>(end - start).count() * 1e-9;
		cout << "One thread time: " << time << endl;

	}

	return 0;
}


//for (const auto& i : arrayC) {
//	cout << i << " ";
//}
//cout << endl;