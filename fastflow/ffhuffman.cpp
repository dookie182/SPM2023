#include <iostream>
#include <vector>
#include <thread>
#include <cmath>
#include <cstring>
#include <functional>
#include <unistd.h>
#include <thread>
#include <mutex>
#include "../include/utimer.cpp"

#include <ff/ff.hpp>		// change 1: include fastflow library code

bool pf = false; 



using namespace std; 

mutex mutual_exclusion;

typedef struct __task {
  unordered_map<char,int>* m;
  string line;
  int nw;
} TASK; 

typedef struct __task_2 {
  unordered_map<char,string>* m;
  string line;
  int nw;
} ENCODE_TASK; 

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

void clean_memory (Node* root){

    if(root == nullptr){
        return;
    }    
    clean_memory(root->left);
    clean_memory(root->right);
    delete(root);

}


class emitter : public ff::ff_monode_t<TASK> {
  private: 
    string fname;
    unordered_map<char,int> *m;
    int nw; 
  public:
    emitter(int nw, string fname, unordered_map<char,int> *m):nw(nw),fname(fname),m(m) {}

    TASK * svc(TASK *) {
        string line;
        string text_block;
        fstream file (fname, ios::in);
        file.seekg(0);
        int file_lenght = file.tellg();
        int chunk_size = file_lenght/nw;

        do{
          getline(file,line);
          text_block += line;
          if(text_block.size() >= chunk_size){
            auto t = new TASK(m,text_block,nw);
            ff_send_out(t);
            text_block = "";
          }

        }while(!file.eof());     
        file.close();
        return(EOS);
    }
  };
  
class emitter_2 : public ff::ff_monode_t<ENCODE_TASK> {
  private: 
    string fname;
    unordered_map<char,string> *m;
    int nw; 
  public:
    emitter_2(int nw, string fname, unordered_map<char,string> *m):nw(nw),fname(fname),m(m) {}

    ENCODE_TASK * svc(ENCODE_TASK *) {
        string line;
        string text_block;
        fstream file (fname, ios::in);
        file.seekg(0);
        int file_lenght = file.tellg();
        int chunk_size = file_lenght/nw;

        do{
          getline(file,line);
          text_block += line;
          if(text_block.size() >= chunk_size){
            auto t = new ENCODE_TASK(m,text_block,nw);
            ff_send_out(t);
            text_block = "";
          }

        }while(!file.eof());     
        file.close();
        return(EOS);
    }
  };
  

class collector : public ff::ff_node_t<TASK> {
private: 
  TASK * tt; 
  string compressed_fname;
  unordered_map<char,string> *m;
  int nw; 

public: 
  TASK * svc(TASK * t) {
    
     free(t);
     return(GO_ON);
  }

};

class collector_2 : public ff::ff_node_t<ENCODE_TASK> {
private: 
  TASK * tt; 
  string compressedFname;
public: 
  collector_2(string compressedFname):compressedFname(compressedFname){}

  ENCODE_TASK * svc(ENCODE_TASK * t) {
    fstream compressed_file (compressedFname, fstream::app);
    compressed_file << t->line;
    free(t);
    compressed_file.close();
    return(GO_ON);
  }

};

class writer : public ff::ff_node_t<ENCODE_TASK> {
private: 
  unordered_map <char,string> *m;
  string text_block;
  int nw;
public:
  writer(unordered_map <char,string> *m, int nw):m(m),nw(nw) {}

  ENCODE_TASK * svc(ENCODE_TASK * t) {
    unordered_map<char,string> *m = t->m;
    auto line = t->line;
    auto nw = t->nw; 
    string tmp;

    for(char ch : line){
      tmp += (*m)[ch];
    }
    t->line = tmp;
    return t;
  }
};

class worker : public ff::ff_node_t<TASK> {
private: 
  unordered_map <char,int> *m;
  string text_block;
  int nw;
public:
  worker(unordered_map <char,int> *m, int nw):m(m),nw(nw) {}

  TASK * svc(TASK * t) {
    unordered_map<char,int> *m = t->m;
    auto line = t->line;
    auto nw = t->nw; 
  	
    unique_lock locker{mutual_exclusion, std::defer_lock};

    unordered_map <char,int> temp;

    for(char ch : line){
      temp[ch] += 1;
    }

    for (auto elem : temp){
      locker.lock();
      (*m)[elem.first] += elem.second;
      locker.unlock();
    }

    return t;
  }
};

void print_map(std::string_view comment, const unordered_map<char, int>& m)
{
    std::cout << comment;
    for (const auto& [key, value] : m)
        std::cout << '[' << key << "] = " << value << "; "<<endl;
    std::cout << '\n';
}
	
int main(int argc, char * argv[]) {

  // if(argc == 2 && strcmp(argv[1],"-help")==0) {
  //   cout << "Usage is: " << argv[0] << " n seed printflag" << endl; 
  //   return(0);
  // }

  string fname = (argc > 1 ? argv[1] : "../data/dataset.txt");  // Input File Name
  string compressedFname = (argc > 1 ? argv[2] : "../data/asciiText2_compressed.txt");  // Output File Name
  int nw = (argc > 3 ? atoi(argv[3]) : 2);   // par degree
  pf=(argc > 4 ? (argv[4][0]=='t' ? true : false) : true);

  unordered_map<char,int> m;
  priority_queue<Node*, vector<Node*>,compare> queue;
	unordered_map<char, string> huffmanMap;  
  Node* root;
  string str,line;
	cout << "-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_" << endl;
  cout << "Working with " << nw << " workers " << endl; 

  long usecs; 
  {
    utimer t0("Parallel computation",&usecs); 
    vector<unique_ptr<ff::ff_node>> workers(nw); 

    for(int i=0; i<nw; i++) 
      workers[i] = make_unique<worker>(&m,nw);

    auto e = emitter(nw,fname,&m);
    auto c = collector(); 
    // cout << "---> " << workers.size() << endl; 
    ff::ff_Farm<> mf(move(workers),e,c);    
    mf.run_and_wait_end();

  
    huffmanMap = huffmanTreeBuilder(m,ref(queue),root);

    vector<unique_ptr<ff::ff_node>> writers(nw); 

    for(int i=0; i<nw; i++)
      writers[i] = make_unique<writer>(&huffmanMap,nw);


    auto e_writer = emitter_2(nw,fname,&huffmanMap);
    auto c_writer = collector_2(compressedFname);

    ff::ff_OFarm<> mf_encode(move(writers));
    ff::ff_Pipe<> pipe(e_writer, mf_encode, c_writer); 

    pipe.run_and_wait_end();
  }

  // 	utimer t4("Decoding String");
  // fstream file2 (compressedFname, ios::in);
  // file2.seekg(0);
  //  while(!file2.eof()){
	// 	getline(file2,line);
  //   str += line;
	// 	}
	
	// // traverse the Huffman Tree again and this time
	// // decode the encoded string
	// int index = -1;
	// cout << "\nDecoded string is: \n";
	// while (index < (int)str.size() - 2) {
	// 	decode(root, index, str);
	// }
	// cout << endl;

  //print_map("Map:", m);

  clean_memory(root);

  cout << "End Parallel Computation: (spent " << usecs << " usecs with "<< nw << " threads)"  << endl;
  return(0); 
}