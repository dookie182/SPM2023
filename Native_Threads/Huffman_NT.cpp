#include <iostream>
#include "../include/utimer.cpp"
#include <fstream>
#include <vector>
#include <string>
#include <string_view>
#include <queue>
#include <thread>
#include <mutex>
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

	return huffmanMap;

}

// Function to count occurrences of symbols in text
void computeOcc(string line, unordered_map<char, int>* m) {
	
	unordered_map<char, int> temp;
	
	// Storing elems in a local temp Map;
	for(char elem: line){
		temp[elem] ++;
	}

	// Storing Data in original Map;
	mutual_exclusion.lock();
	for (const auto &pair: temp) {
		(*m)[pair.first] += pair.second;
	}
	mutual_exclusion.unlock();

}

// Function to generate binary encoded string
void encodeFile(string line, unordered_map<char, string> huffmanMap, vector<string> *partial_encoding, int index) {

	string to_write;

	for (auto elem : line){
		to_write += huffmanMap[elem];
	}

	(*partial_encoding)[index] = to_write;

}

// Function to compress binary string
void compressFile(string line, vector<string> *partial_writes, int index) {
	string tmp, to_compress;

	for (auto elem : line){
		tmp += elem;
		if (tmp.size() == 8){
			bitset<8> c(tmp);
			to_compress += static_cast<char>(c.to_ulong());
			tmp.clear();
		}
	}

	(*partial_writes)[index] = to_compress;
}

// Function to organize strings in vector
void organize(int nw, vector<string> *partial_encoding){
  
  string tail;
  int groups = 0;

  for (int i = 0; i < nw; i++){
  if(tail.size() > 0){
    (*partial_encoding)[i] = tail + (*partial_encoding)[i];
    tail.clear();
  }
  if((*partial_encoding)[i].size() % 8 != 0){
      groups = (*partial_encoding)[i].size()/8;
      groups = groups * 8;
      if(i != nw - 1){
        tail = (*partial_encoding)[i].substr(groups,(*partial_encoding)[i].size() - groups);
      }else{
        tail = (*partial_encoding)[i].substr(groups,(*partial_encoding)[i].size() - groups);
        int n_zero = 8 - tail.size();
        auto last = string(n_zero, '0') + tail;
        (*partial_encoding)[i] = (*partial_encoding)[i]+last;
        tail.clear();
      }
    }
  }
}

// Function to start execution with par degree equal to nw
void start_exec(int nw, string fname, string compressedFname){
	vector<thread> tids;
	vector<thread> tids2;
	vector<thread> tids3;
	vector<string> partial_encoding(nw);
	vector<string> partial_writes(nw);

    // File Management
    fstream file (fname, ios::in);
	fstream compressed_file (compressedFname, ios::out);
	file.clear();
	file.seekg(0);
    int file_lenght = file.tellg();
	int begin = 0;
    string line, tmp, to_send;

	priority_queue<Node*, vector<Node*>,compare> queue;
	unordered_map<char, int> m;
	Node* root;


    unordered_map<char, string> huffmanMap;
	cout << "-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_" << endl;
	cout << "Working with " << nw << " workers " << endl; 

    // Reading File
    {utimer t_read("Reading File:");
        do{
            getline(file,line);
			tmp += line;	
		}while(!file.eof());   
    }

    {utimer t_send_task("Sending Tasks:");

		int chunk_size = tmp.size() / nw;    

        while(begin < tmp.size()){
			if (tmp.size() - begin > chunk_size){
				to_send = tmp.substr(begin,chunk_size);
				tids.push_back(thread(computeOcc,to_send, &m));
				begin += chunk_size;
			}else{
				to_send = tmp.substr(begin,tmp.size() - begin );
				tids.push_back(thread(computeOcc,to_send, &m));
				begin = tmp.size();
			}
        }
		for (auto& elem : tids){
			elem.join();
		}

    }

	{utimer t_build_tree("Building and Encoding Huffman Tree");
		huffmanMap = huffmanTreeBuilder(m,ref(queue),root);
	}

	{
	utimer t_encode("Encoding File");
	
		begin = 0;
		int length = tmp.length();
		int chunk_size = length / nw;    
		int j = 0;
        for(j = 0; j < nw; j++){
			to_send = tmp.substr(0,chunk_size);
			tids2.push_back(thread(encodeFile,to_send,huffmanMap,&partial_encoding,j));
			begin += chunk_size;
		}

		for (auto& elem : tids2){
			elem.join();
		}
	}

	organize(nw,&partial_encoding);

	{
		utimer t_compress("Compressing File");

		for(int j = 0; j < nw; j++){
			tids3.push_back(thread(compressFile,partial_encoding[j],&partial_writes,j));
		}

		for (auto& elem : tids3){
			elem.join();
		}
	}
	
	{	
		utimer t_write("Writing compressed File");
		for (auto elem : partial_writes){
			compressed_file << elem;
		}
	}

	// Closing Files 
	file.close();
	compressed_file.close();

}


int main(int argc, char * argv[]){
	
	string fname = (argc > 1 ? argv[1] : "../data/dataset.txt");  // Input File Name
	string compressedFname = (argc > 1 ? argv[2] : "../data/dataset_compressed_NT.txt");  // Output File Name

	long usecs;
	long time_seq;
	long best_time_seq = 2959428;
	double speedup,scalability,efficiency;

	for (int i = 1; i <= 64; i*=2){

		{
			utimer t0("Completion Time:", &usecs);
			// Starting Execution with i Workers
			start_exec(i,fname,compressedFname);
		}
		
		if (i == 1){
			// Storing total sequential time computation
			time_seq = usecs;
		}
		if(usecs != 0 && i != 1) {
			// Evaluating Statistics
			cout << "--------------------------- Computed Statistics ------------------------------------" << endl;
			scalability = time_seq / (double)usecs;
			speedup = best_time_seq / (double)usecs;
			efficiency = (time_seq / i) / (double)usecs;
			cout << "SpeedUp with " << i << " Threads:"<< speedup << endl;
			cout << "Scalability with " << i << " Threads:"<< scalability << endl;
			cout << "Efficiency with " << i << " Threads:"<< efficiency << endl;
		}
	}
return 0;	
}