#include <iostream>
#include <vector>
#include <bitset>
#include <string>
#include <queue>
#include <fstream>
#include <unordered_map>
#include "../include/utimer.cpp"

#include <ff/ff.hpp>		

using namespace std; 

// Task for counting symbols in input text
typedef struct __task {
  string line;
  unordered_map<char,int> m;
} TASK; 

// Task for encoding and compressing input text
typedef struct __task_2 {
  string line;
} ENCODE_TASK; 


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
		

		int sum = left->freq + right->freq;
		queue.push(createNode('\0', sum, left, right));
	}

	root = queue.top();
	unordered_map<char, string> huffmanMap;
	encode(root, "", huffmanMap);

	return huffmanMap;

}

// First emitter (used in mf and mf_encode)
class emitter : public ff::ff_monode_t<TASK> {
  private: 
    int nw; 
    string *line;
    int chunk_size;
  public:
    emitter(int nw, string *line, int chunk_size):nw(nw),line(line),chunk_size(chunk_size) {}

    TASK * svc(TASK *) {
        string text_block;
        for(int i = 0; i < nw; i++){
          text_block = (*line).substr(i*chunk_size,chunk_size);
          auto t = new TASK(text_block);
          ff_send_out(t);
          text_block.clear();
        }

        return(EOS);
    }
};

// Second emitter (used in mf_write)
class emitter2 : public ff::ff_monode_t<TASK> {
  private: 
    int nw; 
    vector<string> *partial_encoding;
    int chunk_size;
  public:
    emitter2(int nw, vector<string> *partial_encoding, int chunk_size):nw(nw),partial_encoding(partial_encoding),chunk_size(chunk_size) {}

    TASK * svc(TASK *) {
        string text_block;
        for(int i = 0; i < nw; i++){
          text_block = (*partial_encoding)[i];
          auto t = new TASK(text_block);
          ff_send_out(t);
          text_block.clear();
        }

        return(EOS);
    }
};
  
// First Collector (used in mf)
class collector : public ff::ff_node_t<TASK> {
private: 
  unordered_map<char,int>* m;
public: 

  collector(unordered_map<char,int>* m):m(m){}
  TASK * svc(TASK * t) {

    for (auto elem : t->m){
      (*m)[elem.first] += elem.second;
    }

     free(t);
     return(GO_ON);
  }

};

// Second Collector (used in mf_encode and mf_write)
class collector_2 : public ff::ff_node_t<ENCODE_TASK> {
private: 
  vector<string> *partial_results;
  int index = 0;
public: 
  collector_2(vector<string> *partial_results):partial_results(partial_results){}

  ENCODE_TASK * svc(ENCODE_TASK * t) {
    (*partial_results)[index] = t->line;
    index++;

    free(t);
    return(GO_ON);
  }
};

// Writer worker
class writer : public ff::ff_node_t<ENCODE_TASK> {
public:
  ENCODE_TASK * svc(ENCODE_TASK * t) {
    string to_write;
    string to_send;
    
    for (auto elem : t->line){
      to_write += elem;
      if(to_write.size() == 8){
        bitset<8> c(to_write);
        to_send += static_cast<char>(c.to_ulong());
        to_write.clear();
      }
    }

    t->line = to_send;

    return t;
  }
};

// Encoder Worker
class encoder : public ff::ff_node_t<ENCODE_TASK> {
private: 
  unordered_map <char,string> *m;
public:
  encoder(unordered_map <char,string> *m):m(m) {}

  ENCODE_TASK * svc(ENCODE_TASK * t) {
    string to_send;

    for (auto elem : t->line){
      to_send += (*m)[elem];
    }
    t->line = to_send;

    return t;
  }
};

// Count occurrences worker
class worker : public ff::ff_node_t<TASK> {
public:
  TASK * svc(TASK * t) {  	
    unordered_map <char,int> temp;

    for(char ch : t->line){
      temp[ch] ++;
    }
    t->m = temp;

    return t;
  }
};

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
  unordered_map<char,int> m;
  priority_queue<Node*, vector<Node*>,compare> queue;
	unordered_map<char, string> huffmanMap;  
  Node* root;
  string str,line, encoded_text, tail;
  int groups = 0;
  vector<string> partial_encoding(nw);
	vector<string> partial_writes(nw);
  int chunk_size = 0;
	cout << "-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_" << endl;
  cout << "Working with " << nw << " workers " << endl; 

  long usecs; 
  {
    utimer t0("Total Execution", &usecs); 

    fstream file (fname, ios::in);
    file.seekg(0);

    {
      utimer reading_time("Reading File");
      do{
            getline(file,line);
            str += line;
          }while(!file.eof());
    }

    vector<unique_ptr<ff::ff_node>> workers(nw); 

    for(int i=0; i<nw; i++) 
      workers[i] = make_unique<worker>();
    
    chunk_size = str.size() / nw;
    auto e = emitter(nw,&str,chunk_size);
    auto c = collector(&m); 
    ff::ff_Farm<> mf(move(workers),e,c);    

    {
      utimer counting_occ("Counting occ. in text");
      mf.run_and_wait_end();
    }

    {
      utimer building_tree("Building Huffman Tree");
      huffmanMap = huffmanTreeBuilder(m,ref(queue),root);
    }

    // Vector for Encoders workers
    vector<unique_ptr<ff::ff_node>> encoders(nw); 
    
    // Vector for Writers workers
    vector<unique_ptr<ff::ff_node>> writers(nw); 

    for(int i=0; i<nw; i++){
      encoders[i] = make_unique<encoder>(&huffmanMap);
      writers[i] = make_unique<writer>();
    }

    auto c_encoder = collector_2(&partial_encoding);

    ff::ff_OFarm<> mf_encode(move(encoders));
    mf_encode.add_emitter(e);
    mf_encode.add_collector(c_encoder);

    {
      utimer encoding_file("Encoding Text");
      mf_encode.run_and_wait_end();
    }

    organize(nw,&partial_encoding);

    fstream compressed_file (compressedFname, ios::out);
    chunk_size = encoded_text.size() / nw;
    auto e_writer = emitter2(nw,&partial_encoding,chunk_size);
    auto c_writer = collector_2(&partial_writes);
    ff::ff_OFarm<> mf_write(move(writers));
    mf_write.add_emitter(e_writer);
    mf_write.add_collector(c_writer);

    {
      utimer writing_file("Compressing File");
      mf_write.run_and_wait_end();
    }
   	{
      utimer t_write("Writing compressed File");
      for (int i = 0; i < nw; i++){
        
        compressed_file << partial_writes[i];
      }
	  }
    
    compressed_file.close();
    file.close();
  }
}

int main(int argc, char * argv[]) {

  string fname = (argc > 1 ? argv[1] : "../data/dataset.txt");  // Input File Name
  string compressedFname = (argc > 1 ? argv[2] : "../data/dataset_compressed_FF.txt");  // Output File Name

  long usecs, time_seq;
	long best_time_seq = 2959428;
	double speedup,scalability,efficiency;

  for (int i = 1; i <= 64; i*=2){
    {
      utimer t0("Completion Time:", &usecs);
      // Starting Execution with i Workers
		  start_exec(i,fname,compressedFname);
    }
    if(i == 1){
      // Storing total sequential time computation
      time_seq = usecs;
    }
    else{
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
return(0); 
}