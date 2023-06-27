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
#include <numeric>
#include <iterator>
#include <thread>
#include <mutex>
#include "../include/utimer.cpp"
#include "../include/ThreadPool.hpp"
#include "../include/SafeQueue.h"
#include <sstream>
#include <bitset>
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
    for (const auto& [key, value] : m)
        std::cout << '[' << key << "] = " << value << "; "<<endl;
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
	locker.lock();
	for (auto pair: temp) {
		m[pair.first] = m[pair.first] + pair.second;
	}
	locker.unlock();

}

void write(string line, unordered_map<char, string> huffmanMap,vector <char> *partial_writes, int index) {

	string output,tmp;
	vector <string> tmp_str;

    
	auto dash_fold = [](std::string a, string b)
    {
        return move(a) + move(b);
    };

	// for(char elem : line){
	// 	tmp_str.push_back(huffmanMap[elem]);
	// 	if(tmp_str.size() == 8){
	// 			tmp = accumulate(std::next(tmp_str.begin()), tmp_str.begin() + 8,
    //                                 (tmp_str[0]), // start with first element
    //                                 dash_fold);
	// 			bitset<8> c(tmp);
	// 			(*partial_writes).insert((*partial_writes).begin() + index,static_cast<char>(c.to_ulong()));
	// 			index++;
	// 			tmp_str.clear();
	// 		}
	// }
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
	vector<future<void>> future_arr2;

	string line, toWrite, str, text_block, chunk, total_text;
	fstream file (fname, ios::in);
	fstream compressed_file (compressedFname, ios::out);
	int file_lenght = file.tellg();
    int chunk_size = file_lenght/nw;
	int start = 0;
	Node* root;

	mutex mutual_exclusion;
	priority_queue<Node*, vector<Node*>,compare> queue;
	unordered_map<char, int> m;
	unordered_map<char, string> huffmanMap;
	cout << "-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_" << endl;
	cout << "Working with " << nw << " workers " << endl; 
	vector <string> chunks;
	vector <string> partial_reads;
	vector <char> partial_writes;

	//ThreadPool Initialization
	ThreadPool pool(nw);
	pool.init();

		{ utimer t1("Reading File");
		// do{
		// 	getline(file,line);
		// 	text_block += line;
		// 	//total_text += line;
			
		// 	if(text_block.size() >= chunk_size){
		// 		future_arr.push_back(pool.submit(read,text_block, ref(m), ref(mutual_exclusion)));
		// 		text_block.clear();
		// 	}

		// 	}while(!file.eof());   
		// }
		do{
			getline(file,line);
			partial_reads.push_back(line);
			}while(!file.eof());   
		}

		{utimer t2("Time spent sending tasks");
		
			// for(int i = 0; i < nw; i++){
			// 	chunk = text_block.substr(start,chunk_size - 1);
			// 	start += chunk_size;
			// 	future_arr.push_back(pool.submit(read,chunk, ref(m), ref(mutual_exclusion)));
			// }
		for (string elem : partial_reads){
			future_arr.push_back(pool.submit(read,elem, ref(m), ref(mutual_exclusion)));
		}

			for (auto& elem : future_arr){
				elem.get();
			}
		}

		{ utimer t2("Building & Encoding Huffman Tree");

		huffmanMap = huffmanTreeBuilder(m,ref(queue),root);
		}

		{
		utimer t3("Writing compressed File");

		int index;
		for (auto const& elem : partial_reads){
			index =  index + elem.size();
			future_arr2.push_back(pool.submit(write,elem, huffmanMap, &partial_writes, index));
		}

		// for(int i = 0; i < nw; i++){
		// 	chunk = total_text.substr(start,chunk_size - 1);
		// 	start += chunk_size;
		// 	future_arr2.push_back(pool.submit(write,chunk, huffmanMap));
		// }

		// file.clear();
		// file.seekg(0);

		// while(!file.eof()){
		// 	getline(file,line);
		// 	future_arr2.push_back(pool.submit(write,line, huffmanMap, partial_writes));
		// }

		for (auto& elem : future_arr2){  
			// toWrite = elem.get();
			// compressed_file << toWrite;
			elem.get();
		}
		cout << partial_writes.size();
		for (auto& elem : partial_writes){
			compressed_file << elem;
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
		//clean_memory(root);
	}
}

int main(int argc, char * argv[]){
	
	string fname = (argc > 1 ? argv[1] : "../data/dataset.txt");  // Input File Name
	string compressedFname = (argc > 1 ? argv[2] : "../data/dataset_compressed_NT.txt");  // Output File Name
	int nw = (argc > 3 ? atoi(argv[3]) : 64);   // par degree
	fstream log("../data/log.txt",fstream::app);

	long usecs;
	long time_seq;
	float speedup;

	for (int i = 1; i <= 64; i*=2){
		{
			utimer t0("End:", &usecs);
			start(i,fname,compressedFname);
		}
		if (i == 1){
			time_seq = usecs;
		}
		if(usecs != 0) {
			speedup = time_seq / (float)usecs;
			log << "SpeedUp with " << i << " Threads:"<< speedup << endl;
		}
	}
return 0;	
	
}