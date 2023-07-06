#include <iostream>
#include "../include/utimer.cpp"
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
	mutual_exclusion.lock();
	for (const auto &pair: temp) {
		(*m)[pair.first] = (*m)[pair.first] + pair.second;
	}
	mutual_exclusion.unlock();



}

void encodeFile(string line, unordered_map<char, string> huffmanMap, vector<string> *partial_writes, int index) {

	string to_write,front;

	//string tmp = line.substr(index*chunk_size,chunk_size);
	// vector<string> tmp;

	// for (char elem: line){
	// 	tmp.push_back(huffmanMap[elem]);
	// }

	for (auto elem : line){
		to_write += huffmanMap[elem];
		if (to_write.size() > 8){
			front = to_write.substr(0,8);
			bitset<8> c(front);			
			(*partial_writes)[index] += static_cast<char>(c.to_ulong());
			to_write = to_write.substr(8, to_write.size() - front.size());
	}
	}

	// for(char elem : tmp){
	// 	to_write += huffmanMap[elem];
	// 	if (to_write.size() > 8){
	// 		front = to_write.substr(0,8);
	// 		bitset<8> c(front);			
	// 		(*partial_writes)[index] += static_cast<char>(c.to_ulong());
	// 		to_write = to_write.substr(8, to_write.size() - front.size());
	// }
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
	vector<thread> tids;
	vector<thread> tids2;
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
    {utimer t0("File Reading:");
        do{
            getline(file,line);
			tmp += line;	
		}while(!file.eof());   
    }

    {utimer t1("Sending Tasks:");

		int chunk_size = tmp.size() / nw;    

        while(begin < tmp.size()){
			if (tmp.size() - begin > chunk_size){
				to_send = tmp.substr(begin,chunk_size);
				tids.push_back(thread(readFile,to_send, &m));
				begin += chunk_size;
			}else{
				to_send = tmp.substr(begin,tmp.size() - begin );
				tids.push_back(thread(readFile,to_send, &m));
				begin = tmp.size();
			}
        }
		for (auto& elem : tids){
			elem.join();
		}

    }

	{utimer t2("Building & Encoding Huffman Tree");
		huffmanMap = huffmanTreeBuilder(m,ref(queue),root);
	}

	
		begin = 0;
		int length = tmp.length();
		int chunk_size = length / nw;    
		int j = 0;
        for(j = 0; j < nw; j++){
			to_send = tmp.substr(0,chunk_size);
			tids2.push_back(thread(encodeFile,to_send,huffmanMap,&partial_writes,j));
			begin += chunk_size;
		}

		// 		begin = 0;
		// int length = tmp.length();
		// int chunk_size = length / nw;    
		// int j = 0;
        // for(j = 0; j < nw; j++){
		// 	tids2.push_back(thread(encodeFile,tmp,huffmanMap,&partial_writes,j,chunk_size));
		// }

		for (auto& elem : tids2){
			elem.join();
		}
	
	{utimer t4("Writing compressed File");
		for (auto elem : partial_writes){
			compressed_file << elem;
		}
	}

	// Closing Files 
	file.close();
	compressed_file.close();

}

void foo(){

	long usecs;

	{utimer body("Body Exec", &usecs);
	int tmp = 2;


	for(int i = 0; i < 10000; i++){
		tmp*= tmp;
	}

	}

}

int main(int argc, char * argv[]){
	
	string fname = (argc > 1 ? argv[1] : "../data/dataset.txt");  // Input File Name
	string compressedFname = (argc > 1 ? argv[2] : "../data/dataset_compressed_NT.txt");  // Output File Name
	fstream log("../data/log.txt",fstream::app); // Statistics File

	long usecs, usecs2;
	long time_seq;
	double speedup;
	vector<thread> tids_ex;
	vector<long> thread_timings;


	for (int i = 1; i < 64; i*=2)
			// Timings about Thread Generation
		{
		utimer threads("Fork of threads", &usecs2);

			for (int j = 0; j < i; j++){
					tids_ex.push_back(thread(foo));			
				}

			for (auto& tid : tids_ex){
				if(tid.joinable()){
					tid.join();
				}
			}
		}





	for (int i = 1; i <= 64; i*=2){

		{
			utimer t0("Total Execution:", &usecs);
			// Starting Execution with i Workers
			start_exec(i,fname,compressedFname);
		}
		
		if (i == 1){
			// Storing total sequential time computation
			time_seq = usecs;
		}
		if(usecs != 0) {
			// Evaluating SpeedUp
			speedup = time_seq / (double)usecs;
			log << "Scalability with " << i << " Threads:"<< speedup << endl;
			cout << "Scalability with " << i << " Threads:"<< speedup << endl;

		}
	}
return 0;	
}