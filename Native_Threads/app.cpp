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
    char ch;
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

Node* createNode(char ch, int freq, Node* left, Node*right){

	Node* newNode = new Node();
	newNode->ch = ch;
	newNode->freq = freq;
	newNode->left = left;
	newNode->right = right;
	return newNode;

}


void encode(Node* root, string str, unordered_map<char, string> &huffmanMap)
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

unordered_map<char, string> huffmanTreeBuilder(unordered_map<char, int> m, priority_queue<Node*, vector<Node*>,compare>& queue, Node* &root){

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
		queue.push(createNode('\0', sum, left, right));
	}

	root = queue.top();
	unordered_map<char, string> huffmanMap;
	encode(root, "", huffmanMap);
	// for (auto pair: huffmanMap) {
	// 	cout << pair.first << " " << pair.second << '\n';
	// }


	return huffmanMap;

}

void print_map(std::string_view comment, const unordered_map<char, int>& m)
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

void read(string line, unordered_map<char, int>& m, mutex& mutual_exclusion) {
	
	unique_lock locker{mutual_exclusion, std::defer_lock};
	unordered_map<char, int> temp;
	
	// Map Reduce on a local temp Map;
	for(char elem: line){
		temp[elem] = temp[elem] + 1;
	}

	// Storing Data in original Map;
	for (auto pair: temp) {
		locker.lock();
		m[pair.first] = m[pair.first] + pair.second;
		locker.unlock();
	}
}

string write(string line, unordered_map<char, string> huffmanMap) {

	string output;

	for(char elem : line){
		output+= huffmanMap[elem];
	}
	
	return output;
}

void clean_memory (Node* root){

    if(root == nullptr){
        return;
    }    
    clean_memory(root->left);
    clean_memory(root->right);
    delete(root);

}

void start(int nw, string fname, string compressedFname){
	{ utimer t0("Complete Execution"); 
	vector<future<void>> future_arr;
	vector<future<string>> future_arr2;

	string line, toWrite, str, text_block;
	fstream file (fname, ios::in);
	fstream compressed_file (compressedFname, ios::out);
	int file_lenght = file.tellg();
    int chunk_size = file_lenght/nw;

	Node* root;

	mutex mutual_exclusion;
	priority_queue<Node*, vector<Node*>,compare> queue;
	unordered_map<char, int> m;
	unordered_map<char, string> huffmanMap;
	cout << "-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_" << endl;
	cout << "Working with " << nw << " workers " << endl; 

	ThreadPool pool(nw);
	pool.init();

	{ utimer t1("Reading File & Pushing Tasks");
	do{
          getline(file,line);
          text_block += line;
          if(text_block.size() >= chunk_size){
			future_arr.push_back(pool.submit(read,text_block, ref(m), ref(mutual_exclusion)));
            text_block = "";
          }

        }while(!file.eof());   

	for (auto& elem : future_arr){
		elem.get();
	}

	}

	{ utimer t2("Building & Encoding Huffman Tree");

	huffmanMap = huffmanTreeBuilder(m,ref(queue),root);
	}

	{
	utimer t3("Writing compressed File");

	file.clear();
	file.seekg(0);

	while(!file.eof()){
		getline(file,line);
		future_arr2.push_back(pool.submit(write,line, huffmanMap));
	}

	for (auto& elem : future_arr2){  
		toWrite = elem.get();
		compressed_file << toWrite;
	}
	
	compressed_file.close();

	}
	// utimer t4("Decoding String");
	
	// // traverse the Huffman Tree again and this time
	// // decode the encoded string
	// int index = -1;
	// cout << "\nDecoded string is: \n";
	// while (index < (int)str.size() - 2) {
	// 	decode(root, index, str);
	// }
	// cout << endl;
	
	pool.shutdown();
	file.close();
	clean_memory(root);
}
}

int main(int argc, char * argv[]){
	
	string fname = (argc > 1 ? argv[1] : "../data/dataset.txt");  // Input File Name
	string compressedFname = (argc > 1 ? argv[2] : "../data/dataset_compressed_NT.txt");  // Output File Name
	int nw = (argc > 3 ? atoi(argv[3]) : 64);   // par degree

	for (int i = 1; i <= 64; i*=2){
		start(i,fname,compressedFname);
	}
	
return 0;	
	
}