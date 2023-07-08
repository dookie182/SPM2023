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

typedef struct __task {
  string line;
  unordered_map<char,int> m;
} TASK; 

typedef struct __task_2 {
  string line;
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
    int nw; 
    string *line;
    int chunk_size;
  public:
    emitter(int nw, string *line, int chunk_size):nw(nw),line(line),chunk_size(chunk_size) {}

    TASK * svc(TASK *) {
        string text_block;
        int chunk_size = (*line).size()/nw;

        for(int i = 0; i < nw; i++){
          text_block = (*line).substr(i*chunk_size,chunk_size);
          auto t = new TASK(text_block);
          ff_send_out(t);
          text_block.clear();
        }

        return(EOS);
    }
};
  

class collector : public ff::ff_node_t<TASK> {
private: 
  TASK * tt; 
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

class collector_2 : public ff::ff_node_t<ENCODE_TASK> {
private: 
  TASK * tt; 
  string *encoded_text;
public: 
  collector_2(string *encoded_text):encoded_text(encoded_text){}

  ENCODE_TASK * svc(ENCODE_TASK * t) {
    *encoded_text += t->line;

    free(t);
    return(GO_ON);
  }
};

class collector_3 : public ff::ff_node_t<ENCODE_TASK> {
private: 
  TASK * tt; 
  fstream* fout;
public: 
  collector_3(fstream* fout):fout(fout){}

  ENCODE_TASK * svc(ENCODE_TASK * t) {
    *fout << t->line;

    free(t);
    return(GO_ON);
  }
};

class writer : public ff::ff_node_t<ENCODE_TASK> {
private: 
  int nw;
public:
  ENCODE_TASK * svc(ENCODE_TASK * t) {
    string to_write;
    string to_send;
    string front;
    
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

class encoder : public ff::ff_node_t<ENCODE_TASK> {
private: 
  unordered_map <char,string> *m;
  int nw;
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

void print_map(std::string_view comment, const unordered_map<char, int>& m)
{
    std::cout << comment;
    for (const auto& [key, value] : m)
        std::cout << '[' << key << "] = " << value << "; "<<endl;
    std::cout << '\n';
}

void start_exec(int nw, string fname, string compressedFname){
  unordered_map<char,int> m;
  priority_queue<Node*, vector<Node*>,compare> queue;
	unordered_map<char, string> huffmanMap;  
  Node* root;
  string str,line, encoded_text;
  int chunk_size = 0;
	cout << "-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_" << endl;
  cout << "Working with " << nw << " workers " << endl; 

  fstream file (fname, ios::in);
  file.seekg(0);

  {
    utimer readingTime("Reading File");
    do{
          getline(file,line);
          str += line;
        }while(!file.eof());
  }

  long usecs; 
  {
    utimer t0("Total Execution", &usecs); 
    vector<unique_ptr<ff::ff_node>> workers(nw); 

    for(int i=0; i<nw; i++) 
      workers[i] = make_unique<worker>();
    
    chunk_size = str.size() / nw;
    auto e = emitter(nw,&str,chunk_size);
    auto c = collector(&m); 
    ff::ff_Farm<> mf(move(workers),e,c);    

    {
      utimer t1("Counting occ. in text");
      mf.run_and_wait_end();
    }

    {
      utimer t2("Building Huffman Tree");
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

    //auto e_encoder = emitter(nw,&str,chunk_size);
    auto c_encoder = collector_2(&encoded_text);

    ff::ff_OFarm<> mf_encode(move(encoders));
    mf_encode.add_emitter(e);
    mf_encode.add_collector(c_encoder);

    {
      utimer t3("Encoding Text");
      mf_encode.run_and_wait_end();
    }

    fstream compressed_file (compressedFname, ios::out);
    chunk_size = encoded_text.size() / nw * 8;
    auto e_writer = emitter(nw,&encoded_text,chunk_size);
    auto c_writer = collector_3(&compressed_file);
    ff::ff_OFarm<> mf_write(move(writers));
    mf_write.add_emitter(e_writer);
    mf_write.add_collector(c_writer);


    {
      utimer t3("Compressing Text");
      mf_write.run_and_wait_end();
    }
    
    compressed_file.close();
    file.close();
  }
}

int main(int argc, char * argv[]) {

  string fname = (argc > 1 ? argv[1] : "../data/dataset.txt");  // Input File Name
  string compressedFname = (argc > 1 ? argv[2] : "../data/asciiText2_compressed.txt");  // Output File Name

  long usecs, time_seq;
  double speedup;

  for (int i = 1; i <= 64; i*=2){
    {
      utimer t0("End:", &usecs);
		  start_exec(i,fname,compressedFname);
    }
    if(i == 1){
      time_seq = usecs;
    }
    else{
      speedup = time_seq / (double) usecs;
      cout << "SpeedUp with " << i << " Threads:"<< speedup << endl;

    }
	}
	  
  return(0); 
}