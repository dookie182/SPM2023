#include <iostream>
#include <functional>
#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <string_view>
#include <sstream>
#include <future>
#include <queue>
#include <thread>
#include <mutex>
#include "utimer.cpp"
#include "./include/ThreadPool.hpp"
#include "./include/SafeQueue.h"
#include <sstream>
#include <unordered_map>



using namespace std;

struct Node {
    string ch;
	int freq;
	Node *left, *right;
};

struct compare
{
	bool operator()(Node* left, Node* right)
	{
		// highest priority item has lowest frequency
		return left->freq > right->freq;
	}
};

Node* createNode(string ch, int freq, Node* left, Node*right){

	Node* newNode = new Node();
	newNode->ch = ch;
	newNode->freq = freq;
	newNode->left = left;
	newNode->right = right;
	return newNode;

}


void encode(Node* root, string str, unordered_map<string, string> &huffmanMap)
{
	if (root == nullptr)
		return;

	// found a leaf node
	if (!root->left && !root->right) {
		huffmanMap[root->ch] = str;
	}

	encode(root->left, str + "0", huffmanMap);
	encode(root->right, str + "1", huffmanMap);
}

void decode(Node* root, int &index, string str)
{
	if (root == nullptr) {
		return;
	}

	// found a leaf node
	if (!root->left && !root->right)
	{
		cout << root->ch;
		return;
	}

	index++;

	if (str[index] =='0')
		decode(root->left, index, str);
	else
		decode(root->right, index, str);
}

void huffmanTreeBuilder(unordered_map<string, int> m, priority_queue<Node*, vector<Node*>,compare>& queue, priority_queue<Node*, vector<Node*>,compare>& tempQueue){

//	Generating Leaf Nodes with associated priorities;
	for (auto pair: m) {
		queue.push(createNode(pair.first, pair.second, nullptr, nullptr));
	}

	while (queue.size() != 1)
	{
		// Remove the two nodes of highest priority
		Node *left = queue.top(); queue.pop();
		Node *right = queue.top();	queue.pop();
		
		// Create a new internal node with these two nodes
		// as children and with frequency equal to the sum
		// of the two nodes' frequencies. Add the new node
		// to the priority queue.
		int sum = left->freq + right->freq;
		queue.push(createNode(" ", sum, left, right));
	}

	Node* root = queue.top();
	unordered_map<string, string> huffmanMap;
	encode(root, "", huffmanMap);
	for (auto pair: huffmanMap) {
		cout << pair.first << " " << pair.second << '\n';
	}


}

void print_map(std::string_view comment, const unordered_map<std::string, int>& m)
{
    std::cout << comment;
    // iterate using C++17 facilities
    for (const auto& [key, value] : m)
        std::cout << '[' << key << "] = " << value << "; "<<endl;
 
// C++11 alternative:
//  for (const auto& n : m)
//      std::cout << n.first << " = " << n.second << "; "; 
    std::cout << '\n';
}

void read(string line, unordered_map<string, int>& m, mutex& mutual_exclusion) {

	unique_lock locker{mutual_exclusion, std::defer_lock};
    istringstream input;
    input.str(line);
    for (string elem;  getline(input,elem, ' ');){
		locker.lock();
		m[elem] = m[elem] + 1;
		locker.unlock();
		}

}

int main(){
	utimer t0("Complete Execution");
	vector<int> v;
	vector<future<void>> future_arr;
	string fname = "./data/asciiText.txt";
	string line;
	fstream file (fname, ios::in);
	mutex mutual_exclusion;
	priority_queue<Node*, vector<Node*>,compare> queue;
	priority_queue<Node*, vector<Node*>,compare> tempQueue;


	unordered_map<string, int> m;


	ThreadPool pool(64);
	pool.init();

	while(!file.eof()){
		getline(file,line);
		future_arr.push_back(pool.submit(read,line, ref(m), ref(mutual_exclusion)));
		//auto future = pool.submit(read,line, ref(m));
		//future.get();
	}
	utimer t1("Huffman Encoding");

	for (auto& elem : future_arr){
		elem.get();
	}

	huffmanTreeBuilder(m,ref(queue),ref(tempQueue));
	
	//print_map("Map:",m);
	pool.shutdown();
	file.close();

return 0;
}