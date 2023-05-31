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
#include "utimer.cpp"
#include "./include/ThreadPool.hpp"
#include "./include/SafeQueue.h"
#include <sstream>
#include <unordered_map>


using namespace std;

struct Node {
    char ch;
	int freq;
	Node *left, *right;
};

Node* createNode(char ch, int freq, Node* left, Node*right){

	Node* newNode = new Node();
	newNode->ch = ch;
	newNode->freq = freq;
	newNode->left = left;
	newNode->right = right;
	return newNode;

}


void encode(){

}

void decode(){

}

void huffmanTreeBuilder(){

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

void read(string line, unordered_map<string, int>& m) {

    istringstream input;
    input.str(line);
    for (string elem;  getline(input,elem, ' ');){
		m[elem] = m[elem] + 1;
		}

}

int main(){
	utimer t0("Complete Execution");
	vector<int> v;
	string fname = "./data/asciiText.txt";
	string line;
	fstream file (fname, ios::in);
	priority_queue<int> q1;

	unordered_map<string, int> m;


	ThreadPool pool(64);
	pool.init();
	utimer t1("Huffman Encoding");

	while(!file.eof()){
		getline(file,line);
		auto future = pool.submit(read,line, ref(m));
		future.get();
	}
	
	//print_map("Map:",m);
	pool.shutdown();
	file.close();

return 0;
}