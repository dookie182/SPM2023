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
#include "../include/utimer.cpp"
#include "../include/ThreadPool.hpp"
#include "../include/SafeQueue.h"
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
		cout << root->ch << " ";
		return;
	}

	index++;

	if (str[index] =='0')
		decode(root->left, index, str);
	else
		decode(root->right, index, str);
}

unordered_map<string, string> huffmanTreeBuilder(unordered_map<string, int> m, priority_queue<Node*, vector<Node*>,compare>& queue, Node* &root){

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

	root = queue.top();
	unordered_map<string, string> huffmanMap;
	encode(root, "", huffmanMap);
	for (auto pair: huffmanMap) {
		cout << pair.first << " " << pair.second << '\n';
	}


	return huffmanMap;

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
	unordered_map<string, int> temp;
	
	// Map Reduce on a local temp Map;
	for(string elem; getline(input,elem,' ');){
		temp[elem] = temp[elem] + 1;
	}
	
	// Storing Data in original Map;
	for (auto pair: temp) {
		locker.lock();
		m[pair.first] = pair.second;
		locker.unlock();
	}
}

string write(string line, unordered_map<string, string> huffmanMap) {

	string output;
    istringstream input;
    input.str(line);
	
	// Map Reduce on a local temp Map;
	for(string elem; getline(input,elem,' ');){
		output+= huffmanMap[elem];
	}
	
	return output;
}

int main(){
	utimer t0("Complete Execution");
	vector<int> v;
	vector<future<void>> future_arr;
	vector<future<string>> future_arr2;

	string fname = "../data/test.txt";
	string compressedFname = "../data/asciiText2_compressed.txt";

	string line, toWrite,str;
	fstream file (fname, ios::in);
	fstream compressed_file (compressedFname, ios::out);

	Node* root;

	//int dim = file.tellg();
	mutex mutual_exclusion;
	priority_queue<Node*, vector<Node*>,compare> queue;
	unordered_map<string, int> m;
	unordered_map<string, string> huffmanMap;
	ThreadPool pool(64);
	pool.init();

	utimer t2("Pushing Tasks");
	while(!file.eof()){
		getline(file,line);
		future_arr.push_back(pool.submit(read,line, ref(m), ref(mutual_exclusion)));
	}
	utimer t1("Huffman Encoding");

	for (auto& elem : future_arr){
		elem.get();
		

	}
	huffmanMap = huffmanTreeBuilder(m,ref(queue),root);
	
	file.clear();
	file.seekg(0);


	while(!file.eof()){
		getline(file,line);
		future_arr2.push_back(pool.submit(write,line, huffmanMap));
	}


	for (auto& elem : future_arr2){  
		toWrite = elem.get();
		str = str + toWrite;
		compressed_file << toWrite;
	}
	compressed_file.close();

	// traverse the Huffman Tree again and this time
	// decode the encoded string
	int index = -1;
	cout << "\nDecoded string is: \n";
	while (index < (int)str.size() - 2) {
		decode(root, index, str);
	}
	cout << endl;

	
	//print_map("Map:",m);
	pool.shutdown();
	file.close();

return 0;
}