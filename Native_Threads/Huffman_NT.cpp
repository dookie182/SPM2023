#include <iostream>
#include "../include/utimer.cpp"
#include "../include/ThreadPool.hpp"
#include "../include/SafeQueue.h"
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
#include <sstream>
#include <bitset>
#include <unordered_map>


using namespace std;

mutex mutual_exclusion;

// Huffman Tree Node
struct Node {
    char ch;
	int freq;
	Node *left, *right;
};

// Compare Function for Huffman Tree 
struct compare
{
	bool operator()(Node* left, Node* right)
	{
		// highest priority item has lowest frequency
		return left->freq > right->freq;
	}
};

// Function to create a new Huffman tree Node
Node* createNode(char ch, int freq, Node* left, Node*right){

	Node* newNode = new Node();
	newNode->ch = ch;
	newNode->freq = freq;
	newNode->left = left;
	newNode->right = right;
	return newNode;

}

// Function to encode the Huffman Tree
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


// Function to encode 
void decode(Node* root, int &index, string str)
{
	if (root == nullptr) {
		return;
	}

	// Leaf node
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



// Function to read from a Text File and count occ.
void readFile(string line, unordered_map<char, int>* m) {
	
	//unique_lock locker{mutual_exclusion, std::defer_lock};
	unordered_map<char, int> temp;
	
	// Map Reduce on a local temp Map;
	for(char elem: line){
		temp[elem] = temp[elem] + 1;
	}

	// Storing Data in original Map;
	for (const auto &pair: temp) {
        mutual_exclusion.lock();
		(*m)[pair.first] = (*m)[pair.first] + pair.second;
        mutual_exclusion.unlock();
	}

}

void writeFile(string line, unordered_map<char, string> huffmanMap,vector <char> *partial_writes) {

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

void start_exec(int nw, string fname, string compressedFname){
    vector<future<void>> future_arr;
    vector<future<void>> future_arr2;

    // File Management
    fstream file (fname, ios::in);
	fstream compressed_file (compressedFname, ios::out);
	file.clear();
	file.seekg(0);
    int file_lenght = file.tellg();
	int begin = 0;
    int chunk_size = file_lenght/nw;
    string line, tmp, to_send;
    vector <char> partial_writes;

	priority_queue<Node*, vector<Node*>,compare> queue;
	unordered_map<char, int> m;
	Node* root;


    unordered_map<char, string> huffmanMap;
	cout << "-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_" << endl;
	cout << "Working with " << nw << " workers " << endl; 

    // ThreadPool Initialization
	ThreadPool pool(nw);
	pool.init();

    // Reading File
    {utimer t0("File Reading:");
        do{
            getline(file,line);
			tmp += line;
		}while(!file.eof());   
    }

    {utimer t1("Sending Tasks:");

		int chunk_size = tmp.size() / nw;    

        for (int i = 0; i < nw; i++){
			to_send = tmp.substr(begin,chunk_size);
            future_arr.push_back(pool.submit(readFile, to_send, &m));
			begin += chunk_size;
        }

		for (auto& elem : future_arr){
			elem.get();
		}
    }
	{utimer t2("Building & Encoding Huffman Tree");
		huffmanMap = huffmanTreeBuilder(m,ref(queue),root);
	}

	{utimer t3("Writing compressed File");
		begin = 0;
        for (int i = 0; i < nw; i++){
			to_send = tmp.substr(begin,chunk_size);
            future_arr2.push_back(pool.submit(writeFile,line, huffmanMap, &partial_writes));
			begin += chunk_size;
        }
	}
	file.close();
	compressed_file.close();

	pool.shutdown();
}

int main(int argc, char * argv[]){
	
	string fname = (argc > 1 ? argv[1] : "../data/dataset.txt");  // Input File Name
	string compressedFname = (argc > 1 ? argv[2] : "../data/dataset_compressed_NT.txt");  // Output File Name
	//nt nw = (argc > 3 ? atoi(argv[3]) : 32);   // par degree
	fstream log("../data/log.txt",fstream::app);

	long usecs;
	long time_seq;
	double speedup;

	for (int i = 1; i <= 64; i*=2){
		{
			utimer t0("End:", &usecs);
			start_exec(i,fname,compressedFname);
		}
		
		if (i == 1){
			time_seq = usecs;
		}
		if(usecs != 0) {
			speedup = time_seq / (double)usecs;
			log << "SpeedUp with " << i << " Threads:"<< speedup << endl;
			cout << "SpeedUp with " << i << " Threads:"<< speedup << endl;

		}
	}
return 0;	
}