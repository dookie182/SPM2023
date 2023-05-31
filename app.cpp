#include <iostream>
#include <functional>
#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <string_view>
#include <sstream>
#include <future>
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
//
// C++98 alternative
//  for (std::map<std::string, int>::const_iterator it = m.begin(); it != m.end(); it++)
//      std::cout << it->first << " = " << it->second << "; ";
 
    std::cout << '\n';
}

void read(string line, unordered_map<string, int>& m) {

    istringstream input2;
    input2.str(line);
    for (string elem; getline(input2,elem, ' '); ){
        cout << elem << '\n';
		m[elem] = m[elem] + 1;
		}

}

  




void loadText(){
  
	string fname = "./data/asciiText.txt";

	vector<vector<string>> content;
	vector<string> row;
	string line, word;
 
	fstream file (fname, ios::in);
	if(file.is_open())
	{
		while(getline(file, line))
		{
			row.clear();
 
			stringstream str(line);
 
			while(getline(str, word, ' '))
				row.push_back(word);
			content.push_back(row);
		}
	}
	else
		cout<<"Could not open the file\n";
 
	for(int i=0;i<content.size();i++)
	{
		for(int j=0;j<content[i].size();j++)
		{
			cout<<content[i][j]<<" ";
		}
		cout<<"\n";
	}
}

int main(){
	utimer t0("Huffman Encoding");
	vector<int> v;
	string fname = "./data/asciiText.txt";
	string line;
	fstream file (fname, ios::in);

	unordered_map<string, int> m{};


	ThreadPool pool(10);
	pool.init();

	while(!file.eof()){
		getline(file,line);
		auto future = pool.submit(read,line, ref(m));
		future.get();
	}
	print_map("Map:",m);
	pool.shutdown();

return 0;
}