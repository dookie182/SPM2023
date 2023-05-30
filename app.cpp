#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include "utimer.cpp"


using namespace std;

struct Node {
    char ch;
	int freq;
	Node *left, *right;
};


void encode(){

}

void decode(){

}

void huffmanTreeBuilder(){

}


void loadText(){
  
	string fname = "dataset.txt";

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
 
			while(getline(str, word, '\n'))
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

    loadText();

return 0;
}