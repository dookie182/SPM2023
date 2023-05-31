#include <iostream>
#include <functional>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <future>
#include <thread>
#include "utimer.cpp"
#include "./include/ThreadPool.hpp"
#include "./include/SafeQueue.h"



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

int read(string line){
	vector<string> row;
	string word;

	return 15;

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
	ThreadPool pool(64);
	pool.init();
	while(!file.eof()){
		getline(file, line);
		future<int> future = pool.submit(read,line);
		const int result = future.get();
		cout << result << endl;
	}

return 0;
}