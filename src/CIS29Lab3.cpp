//============================================================================
// Name        : Lab3
// Author      : Branden Lee
// Date        : 5/15/2018
// Description : Decoding Code3of9 Symbology in XML files with threads
//============================================================================

/* 2018-06-04 Revision 2
 * 2018-06-03: The professor gave the comment:
 * I ran your code three times, and got the same output. This means your carts are not being processed asynchronously.
 * Fix for more points.
 *
 * In response to this, I want to point out that output file "cartsList.txt" DID CHANGE each time the program was run.
 * Maybe it was not evident in the beginning of the file, but toward the end of the file the output would differ after each run
 *
 * Despite this, In this revision I have added smart pointers for the sake of learning and changed the cart processing algorithm
 * a bit to make the output results differ a lot more than previously after each run.
 * */

#include <string>
#include <fstream>
#include <iostream>		// std::cout
#include <sstream>
#include <iomanip>
#include <vector>		// std::vector
#include <bitset>
#include <list>
#include <regex>
#include <stack>
#include <queue>
#include <cstring>
#include <thread>
#include <future>
#include <mutex>		// std::mutex
#include <memory>		// std::shared_ptr
using namespace std;

#define BUFFER_SIZE 256
// Usually the same as the processor cores (4 recommended)
#define CART_PROCESSING_THREADS 7

mutex mutexGlobal;		// mutex for critical section

/**
 @class FileHandler
 simply reads or writes the decoded or encoded message to a file\n
 */
class FileHandler {
public:
	FileHandler() {
	}
	~FileHandler() {
	}
	/** takes a file stream by reference and opens a file.\n
	 * the reason we do not return the string of the entire ASCII file
	 * is because we want to stream and not waste memory
	 @pre None
	 @post None
	 @param string File name to open
	 @return True on file open successful and false in not
	 */
	bool readStream(string fileName, ifstream& fileStream);
	bool writeStream(string fileName, ofstream& fileStream);
	bool writeString(string fileName, string stringValue);
	bool close(ifstream& fileStream);
};

/**
 @class HashTable
 A simple hash table \n
 */
template<class K, class T>
class HashTable {
private:
	vector<pair<K, T>*> table;
	unsigned int insertAttempts;
public:
	HashTable();
	HashTable(unsigned int size);
	~HashTable() {
	}
	bool insert(K key, T val);
	T at(K key);
	T atIndex(unsigned int index);
	unsigned int hash(K key);
	unsigned int hash(unsigned int key);
	unsigned int size();
	bool resize(unsigned int key);
};

/**
 @class XMLNode
 XML document node \n
 */
class XMLNode {
private:
	string name; // tag name inside the angled brackets <name>
	string value; // non-child-node inside node <>value</>
	vector<shared_ptr<XMLNode>> childNodes;
	/* 2018-06-04 Revision 2
	 * XMLNode.parentNode was removed because it could lead to circular references
	 * */
public:
	XMLNode();
	XMLNode(string name_);
	/* 2018-06-04 Revision 2
	 * destructor removed. smart pointers will do this.
	 * */
	void valueAppend(string str);
	/* not a comprehensive list of definitions for all getters/setters
	 * it is not vital to the program to have all setters
	 */
	string getName();
	string getValue();
	/* 2018-06-04 Revision 2
	 * XMLNode.getParent() was removed because it could lead to circular references
	 * */
	shared_ptr<XMLNode> addChild(string str);
	shared_ptr<XMLNode> getChild(unsigned int index);
	bool findChild(string name_, shared_ptr<XMLNode>& returnNode,
			unsigned int index);
	unsigned int childrenSize();
	unsigned int findChildSize(string name_);
};

/**
 @class XMLParser
 XML document parser \n
 */
class XMLParser {
private:
	regex tagOpenRegex, tagEndRegex;
public:
	XMLParser();
	/* 2018-06-04 Revision 2
	 * destructor removed. smart pointers will do this.
	 * */
	bool documentStream(istream& streamIn, shared_ptr<XMLNode> xmlDoc);
	bool bufferSearch(string& streamBuffer, shared_ptr<XMLNode> xmlNodeCurrent,
			stack<shared_ptr<XMLNode>>& documentStack, unsigned int& mode);
	bool nodePop(string& tagString, shared_ptr<XMLNode> xmlNodeCurrent,
			stack<shared_ptr<XMLNode>>& documentStack);
	bool nodePush(string& tagString, shared_ptr<XMLNode> xmlNodeCurrent,
			stack<shared_ptr<XMLNode>>& documentStack);
};

/**
 @class Code39CharTable
 A table to convert Code 39 integers to characters \n
 */
class Code39CharTable {
private:
	vector<unsigned int> charIntToCode39IntTable;
	vector<char> Code39IntToCharTable;
public:
	Code39CharTable();
	~Code39CharTable() {
	}
	void buildCode39IntToCharTable();
	bool intToChar(unsigned int intIn, char& charOut);
	bool charToInt(char charIn, unsigned int& intOut);
};

class Product {
private:
	string name;
	double price;
public:
	Product(string name_, double price_);
	~Product() {
	}
	string getName();
	double getPrice();
	string toString();
};

class ProductTable {
private:
	shared_ptr<Code39CharTable> code39CharTable;
	HashTable<string, shared_ptr<Product>> code39ItemToProductTable;
public:
	ProductTable();
	~ProductTable() {
	}
	/*
	 * @param key The code39 Binary String of first 5 characters of product name
	 * @param val pointer to the product
	 */
	bool insert(string key, shared_ptr<Product> val);
	/*
	 * Generates the key automatically
	 * @param val pointer to the product
	 */
	bool insert(shared_ptr<Product> val);
	/*
	 * WARNING: could throw an exception
	 */
	shared_ptr<Product> at(string key);
	string toString();

};

class Cart {
private:
	vector<shared_ptr<Product>> productList;
	unsigned int cartNumber;
	double priceTotal;
public:
	Cart();
	Cart(unsigned int cartNumber_);
	~Cart() {
	}
	void insert(shared_ptr<Product> productPtr);
	void calculatePriceTotal();
	string toString();
};

class CartList {
private:
	vector<shared_ptr<Cart>> cartList;
public:
	CartList() {
	}
	~CartList() {
	}
	void insert(shared_ptr<Cart> cartPtr);
	string toString();
};

/**
 * @class CartLane
 * A range of carts are stored in each lane container. An external method will set
 * the range of cart numbers to be assigned to each lane. Lanes are designed so that they
 * are independent of each other. A lane may finish processing all carts before the others leaving it empty.
 **/
class CartLane {
private:
	shared_ptr<queue<shared_ptr<XMLNode>>> cartXMLNodeQueuePtr;
	shared_ptr<CartList> cartListPtr;
	shared_ptr<ProductTable> productTablePtr;
public:
	CartLane() {
	}
	~CartLane() {
	}
	void init(shared_ptr<queue<shared_ptr<XMLNode>>> cartXMLNodeQueuePtr_,
			shared_ptr<CartList> cartListPtr_,
			shared_ptr<ProductTable> productTablePtr_);
	bool process();
};

/**
 @class Code39Item
 Converts a Code 39 Item to the a product \n
 */
class Code39Item {
private:
	shared_ptr<Code39CharTable> code39CharTable;
	string binaryString, codeString;
	queue<int> intQueue;
public:
	Code39Item(shared_ptr<Code39CharTable> code39CharTablePtr);
	~Code39Item() {
	}
	void setBinaryString(string binaryString_);
	void setCodeString(string codeString_);
	bool toCodeString(string& code39CharItem);
	bool toBinaryString(string& code39CharItem);
};

/**
 @class Parser
 Miscellaneous utilities for parsing data structures \n
 */
class Parser {
public:
	Parser();
	~Parser() {
	}
	bool productListXMLNodetoObject(shared_ptr<XMLNode> productListXMLNodePtr,
			shared_ptr<ProductTable> productTablePtr);
	bool cartListXMLNodetoObject(shared_ptr<XMLNode> cartListXMLNodePtr,
			shared_ptr<CartList> cartListPtr,
			shared_ptr<ProductTable> productTablePtr);
};

/*
 * FileHandler Implementation
 */
bool FileHandler::close(ifstream& fileStream) {
	fileStream.close();
	return true;
}

bool FileHandler::readStream(string fileName, ifstream& fileStream) {
	fileStream.open(fileName, ios::binary);
	if (fileStream.is_open()) {
		return true;
	}
	return false;
}

bool FileHandler::writeStream(string fileName, ofstream& fileStream) {
	fileStream.open(fileName);
	if (fileStream.is_open()) {
		return true;
	}
	return false;
}

bool FileHandler::writeString(string fileName, string stringValue) {
	ofstream fileStream;
	fileStream.open(fileName);
	if (fileStream.is_open()) {
		fileStream << stringValue;
		fileStream.close();
		return true;
	}
	return false;
}

/*
 * HashTable Implementation
 */
template<class K, class T>
HashTable<K, T>::HashTable() {
	table.resize(100);
	insertAttempts = 10;
}

template<class K, class T>
bool HashTable<K, T>::insert(K key, T val) {
	unsigned int attempts = insertAttempts;
	K keyOriginal = key;
	unsigned int keyInt = hash(key);
	bool flag = false;
	for (; attempts > 0; attempts--) {
		if (table[keyInt] == nullptr) {
			table[keyInt] = new pair<K, T>(keyOriginal, val);
			flag = true;
			break;
		}
		keyInt = hash(keyInt);
	}
	return flag;
}
template<class K, class T>
T HashTable<K, T>::at(K key) {
	unsigned int attempts = insertAttempts;
	K keyOriginal = key;
	unsigned int keyInt = hash(key);
	T ret { };
	for (; attempts > 0; attempts--) {
		if (table[keyInt] != nullptr && table[keyInt]->first == keyOriginal) {
			ret = table[keyInt]->second;
			break;
		}
		keyInt = hash(keyInt);
	}
	if (attempts == 0) {
		throw out_of_range("");
	}
	return ret;
}
template<class K, class T>
T HashTable<K, T>::atIndex(unsigned int index) {
	pair<K, T>* temp = table[index];
	if (temp != nullptr) {
		return temp->second;
	} else {
		throw invalid_argument("");
	}
}
template<class K, class T>
unsigned int HashTable<K, T>::hash(K key) {
	unsigned int hash = 1315423911;
	unsigned int i = 0;
	while (key[i++]) {
		hash ^= ((hash << 5) + key[i] + (hash >> 2));
	}
	return (hash & 0x7FFFFFFF) % table.size();
}
template<class K, class T>
unsigned int HashTable<K, T>::hash(unsigned int key) {
	// stackoverflow.com/questions/664014/what-integer-hash-function-are-good-that-accepts-an-integer-hash-key
	key = ((key >> 16) ^ key) * 0x45d9f3b;
	key = ((key >> 16) ^ key) * 0x45d9f3b;
	key = (key >> 16) ^ key;
	return key % table.size();
}
template<class K, class T>
unsigned int HashTable<K, T>::size() {
	return static_cast<unsigned int>(table.size());
}

template<class K, class T>
bool HashTable<K, T>::resize(unsigned int key) {
	bool flag = false;
	if (key > table.size()) {
		table.resize(key);
		flag = true;
	}
	return flag;
}

/*
 * XMLNode Implementation
 */
XMLNode::XMLNode() {
	name = "";
	value = "";
}
XMLNode::XMLNode(string name_) {
	name = name_;
	value = "";
}
void XMLNode::valueAppend(string str) {
	value.append(str);
}
string XMLNode::getName() {
	return name;
}
string XMLNode::getValue() {
	return value;
}
shared_ptr<XMLNode> XMLNode::addChild(string str) {
	shared_ptr<XMLNode> childNode = make_shared<XMLNode>(str);
	childNodes.push_back(childNode);
	return childNode;
}
shared_ptr<XMLNode> XMLNode::getChild(unsigned int index) {
	shared_ptr<XMLNode> nodeReturn;
	try {
		nodeReturn = childNodes.at(index);
	} catch (...) {
		// nothing
	}
	return nodeReturn;
}
bool XMLNode::findChild(string name_, shared_ptr<XMLNode>& returnNode,
		unsigned int index) {
	unsigned int findCount, i, n;
	bool returnValue = false;
	findCount = 0;
	n = static_cast<unsigned int>(childNodes.size());
	for (i = 0; i < n; i++) {
		if (childNodes[i]->name == name_) {
			if (findCount == index) {
				returnNode = childNodes[i];
				returnValue = true;
				break;
			}
			findCount++;
		}
	}
	return returnValue;
}
unsigned int XMLNode::childrenSize() {
	return static_cast<unsigned int>(childNodes.size());
}
unsigned int XMLNode::findChildSize(string name_) {
	unsigned int findCount, i, n;
	findCount = 0;
	n = static_cast<unsigned int>(childNodes.size());
	for (i = 0; i < n; i++) {
		if (childNodes[i]->name == name_) {
			findCount++;
		}
	}
	return findCount;
}

/*
 * XMLParser Implementation
 */
XMLParser::XMLParser() {
	tagOpenRegex = regex("\\<(.*?)\\>");   // matches an opening tag <tag>
	tagEndRegex = regex("\\<\\/(.*?)\\>");   // matches an ending tag </tag>
}
bool XMLParser::documentStream(istream& streamIn, shared_ptr<XMLNode> xmlDoc) {
	/* Parsing Steps:
	 * 1. create document node. If stack is empty then document node is the parent.
	 * 2. grab first <tag> and add push on stack.
	 *    future nodes will be a child of top of the stack
	 * 3. grab child node and push it to the stack.
	 * 4. value between <child></child> is added to the node on top of the stack
	 * 5. if </tag> found then it is popped off the stack
	 * */
	unsigned int fileSize, filePos, bufferSize, mode;
	string streamBuffer;
	stack<shared_ptr<XMLNode>> documentStack;
	/* 2018-06-04 Revision 2
	 * bottom of the stack is the document node.
	 * */
	//xmlDoc = make_shared<XMLNode>("Root");
	documentStack.push(xmlDoc);
	bufferSize = BUFFER_SIZE;
	fileSize = filePos = mode = 0;
	streamBuffer = "";
	shared_ptr<XMLNode> xmlNodeCurrent = xmlDoc;
	char bufferInChar[BUFFER_SIZE];
	streamIn.seekg(0, ios::end); // set the pointer to the end
	fileSize = (unsigned int) streamIn.tellg(); // get the length of the file
	streamIn.seekg(0, ios::beg); // set the pointer to the beginning
	while (filePos < fileSize) {
		streamIn.seekg(filePos, ios::beg); // seek new position
		if (filePos + bufferSize > fileSize) {
			bufferSize = fileSize - filePos;
			memset(bufferInChar, 0, sizeof(bufferInChar)); // zero out buffer
		}
		memset(bufferInChar, 0, sizeof(bufferInChar)); // zero out buffer
		streamIn.read(bufferInChar, bufferSize);
		streamBuffer.append(bufferInChar, bufferSize);
		bufferSearch(streamBuffer, xmlNodeCurrent, documentStack, mode);
		// advance buffer
		filePos += bufferSize;
	}
	// remaining buffer belongs to current node value
	xmlNodeCurrent->valueAppend(streamBuffer);
	cout << "why " << xmlDoc->childrenSize() << endl;
	return true;
}

bool XMLParser::bufferSearch(string& streamBuffer,
		shared_ptr<XMLNode> xmlNodeCurrent,
		stack<shared_ptr<XMLNode>>& documentStack, unsigned int& mode) {
	unsigned int ticks = 0;
	unsigned int tagOpenPos, tagEndPos, noPos;
	noPos = (unsigned int) string::npos;
	string tagPop, matchGroupString, temp;
	while (ticks < 9999) { // infinite loop protection
		ticks++;
		if (mode == 0) {
			tagOpenPos = (unsigned int) streamBuffer.find("<");
			if (tagOpenPos != noPos) {
				/* opening angle bracket for a tag
				 * we assume that text before this is the value of current xml node
				 */
				mode = 1;
				xmlNodeCurrent->valueAppend(streamBuffer.substr(0, tagOpenPos));
				streamBuffer.erase(0, tagOpenPos);
				tagOpenPos = 0;
			} else {
				break;
			}
		} else if (mode == 1) {
			// expecting ending angle bracket for a tag
			tagEndPos = (unsigned int) streamBuffer.find(">");
			temp = streamBuffer.substr(0, tagEndPos + 1);
			std::smatch m;
			if (tagEndPos != noPos) {
				// let's use regex to grab the tag name between the angled brackets
				// let's first check if we just ended an ending tag </>
				//std::smatch m;
				regex_match(temp, m, tagEndRegex);
				if (!m.empty()) {
					/* extract matched group
					 * a .trim() method would be great...
					 */
					try {
						matchGroupString = m[1].str(); // match group
					} catch (...) {
						matchGroupString = "";
					}
					string s;
					s.append("</").append(matchGroupString).append("> ").append(
							streamBuffer).append("\n");
					cout << s;
					nodePop(matchGroupString, xmlNodeCurrent, documentStack);
				} else {
					// now check if we just ended an opening tag <>
					//std::smatch m;
					regex_match(temp, m, tagOpenRegex);
					if (!m.empty()) {
						try {
							matchGroupString = m[1].str(); // match group
						} catch (...) {
							matchGroupString = "";
						}
						/*
						string s;
						s.append("<").append(matchGroupString).append("> ").append(
								streamBuffer).append("\n");
						cout << s;
						*/
						nodePush(matchGroupString, xmlNodeCurrent,
								documentStack);
					}
				}
				// erase to the end of the ending angle bracket ">"
				streamBuffer.erase(0, tagEndPos + 1);
				mode = 0;
			} else {
				break;
			}
		}
	}
	return true;
}

bool XMLParser::nodePop(string& tagString, shared_ptr<XMLNode> xmlNodeCurrent,
		stack<shared_ptr<XMLNode>>& documentStack) {
	/* pop nodes off stack until end tag is found
	 * can't go lower than the document root
	 */
	string tagPop = "";
	if (tagString.length() > 0) {
		/* 2018-06-04 Revision 2
		 * bottom of the stack is the document node. These reduces number of arguments passed.
		 * */
		while (documentStack.size() > 1 && tagPop != tagString) {
			tagPop = documentStack.top()->getName();
			documentStack.pop();
			/* 2018-06-04 Revision 2
			 * XMLNode.getParent() was removed because it could lead to circular references
			 * */
			xmlNodeCurrent = documentStack.top();
		}
	}
	return true;
}

bool XMLParser::nodePush(string& tagString, shared_ptr<XMLNode> xmlNodeCurrent,
		stack<shared_ptr<XMLNode>>& documentStack) {
	if (tagString.length() > 0) {
		xmlNodeCurrent = xmlNodeCurrent->addChild(tagString);
		documentStack.push(xmlNodeCurrent);
	}
	return true;
}

/*
 * Code39CharTable Implementation
 */
Code39CharTable::Code39CharTable() {
	// Size of 128 to potentially hold ascii codes up to 128
	charIntToCode39IntTable.resize(128);
	try {
		charIntToCode39IntTable[(unsigned int) ' '] = 196;
		charIntToCode39IntTable[(unsigned int) '-'] = 133;
		charIntToCode39IntTable[(unsigned int) '+'] = 138;
		charIntToCode39IntTable[(unsigned int) '$'] = 168;
		charIntToCode39IntTable[(unsigned int) '%'] = 42;
		charIntToCode39IntTable[(unsigned int) '*'] = 148;
		charIntToCode39IntTable[(unsigned int) '.'] = 388;
		charIntToCode39IntTable[(unsigned int) '/'] = 162;
		charIntToCode39IntTable[(unsigned int) '0'] = 52;
		charIntToCode39IntTable[(unsigned int) '1'] = 289;
		charIntToCode39IntTable[(unsigned int) '2'] = 97;
		charIntToCode39IntTable[(unsigned int) '3'] = 352;
		charIntToCode39IntTable[(unsigned int) '4'] = 49;
		charIntToCode39IntTable[(unsigned int) '5'] = 304;
		charIntToCode39IntTable[(unsigned int) '6'] = 112;
		charIntToCode39IntTable[(unsigned int) '7'] = 37;
		charIntToCode39IntTable[(unsigned int) '8'] = 292;
		charIntToCode39IntTable[(unsigned int) '9'] = 100;
		charIntToCode39IntTable[(unsigned int) 'A'] = 265;
		charIntToCode39IntTable[(unsigned int) 'B'] = 73;
		charIntToCode39IntTable[(unsigned int) 'C'] = 328;
		charIntToCode39IntTable[(unsigned int) 'D'] = 25;
		charIntToCode39IntTable[(unsigned int) 'E'] = 280;
		charIntToCode39IntTable[(unsigned int) 'F'] = 88;
		charIntToCode39IntTable[(unsigned int) 'G'] = 13;
		charIntToCode39IntTable[(unsigned int) 'H'] = 268;
		charIntToCode39IntTable[(unsigned int) 'I'] = 76;
		charIntToCode39IntTable[(unsigned int) 'J'] = 28;
		charIntToCode39IntTable[(unsigned int) 'K'] = 259;
		charIntToCode39IntTable[(unsigned int) 'L'] = 67;
		charIntToCode39IntTable[(unsigned int) 'M'] = 322;
		charIntToCode39IntTable[(unsigned int) 'N'] = 19;
		charIntToCode39IntTable[(unsigned int) 'O'] = 274;
		charIntToCode39IntTable[(unsigned int) 'P'] = 82;
		charIntToCode39IntTable[(unsigned int) 'Q'] = 7;
		charIntToCode39IntTable[(unsigned int) 'R'] = 262;
		charIntToCode39IntTable[(unsigned int) 'S'] = 70;
		charIntToCode39IntTable[(unsigned int) 'T'] = 22;
		charIntToCode39IntTable[(unsigned int) 'U'] = 385;
		charIntToCode39IntTable[(unsigned int) 'V'] = 193;
		charIntToCode39IntTable[(unsigned int) 'W'] = 448;
		charIntToCode39IntTable[(unsigned int) 'X'] = 145;
		charIntToCode39IntTable[(unsigned int) 'Y'] = 400;
		charIntToCode39IntTable[(unsigned int) 'Z'] = 208;
	} catch (...) {
		// nothing
	}
	buildCode39IntToCharTable();
}

void Code39CharTable::buildCode39IntToCharTable() {
	unsigned int i, n, n1;
	/* extend to lower case characters */
	n = ((int) 'Z') + 1;
	for (i = (int) 'A'; i < n; i++) {
		if (charIntToCode39IntTable[i]
				&& (n1 = charIntToCode39IntTable[i]) > 0) {
			charIntToCode39IntTable[(unsigned int) tolower(char(i))] =
					charIntToCode39IntTable[i];
		}
	}
	/* 2^9 since the longest Code 39 Binary is 9 bits */
	Code39IntToCharTable.resize(512);
	// build a binary int to char map
	n = (unsigned int) charIntToCode39IntTable.size();
	for (i = 0; i < n; i++) {
		if (charIntToCode39IntTable[i]
				&& (n1 = charIntToCode39IntTable[i]) > 0) {
			// we are worried of bits above 9
			try {
				Code39IntToCharTable[n1] = char(i);
			} catch (...) {
				// nothing
			}
		}
	}
}

bool Code39CharTable::intToChar(unsigned int intIn, char& charOut) {
	bool returnValue = false;
	try {
		charOut = Code39IntToCharTable[intIn];
		returnValue = true;
	} catch (...) {
		// nothing
	}
	return returnValue;
}

bool Code39CharTable::charToInt(char charIn, unsigned int& intOut) {
	bool returnValue = false;
	try {
		intOut = charIntToCode39IntTable[(unsigned int) charIn];
		returnValue = true;
	} catch (...) {
		// nothing
	}
	return returnValue;
}

/*
 * Product Implementation
 */
Product::Product(string name_, double price_) {
	name = name_;
	price = price_;
}
string Product::getName() {
	return name;
}
double Product::getPrice() {
	return price;
}
string Product::toString() {
	// I'm using string stream so I don't need to reinvent console spacing.
	stringstream str;
	str << left << setw(20) << name << " " << fixed << setprecision(2) << price;
	return str.str();
}

/*
 * ProductTable Implementation
 */
ProductTable::ProductTable() {
	code39CharTable = make_shared<Code39CharTable>();
	code39ItemToProductTable.resize(1700);
}
bool ProductTable::insert(string key, shared_ptr<Product> val) {
	return code39ItemToProductTable.insert(key, val);
}
bool ProductTable::insert(shared_ptr<Product> val) {
	bool returnValue = false;
	Code39Item code39Item(code39CharTable);
	code39Item.setCodeString(val->getName().substr(0, 5));
	string code39BinaryString;
	if (code39Item.toBinaryString(code39BinaryString)) {
		/*cout << val->getName().substr(0, 5) << " = " << code39BinaryString
		 << endl;*/
		returnValue = code39ItemToProductTable.insert(code39BinaryString, val);
	}
	return returnValue;
}
shared_ptr<Product> ProductTable::at(string key) {
	return code39ItemToProductTable.at(key);
}
string ProductTable::toString() {
	string str = "";
	str.append(" Product Table \n---------------\n");
	stringstream headString, endString;
	headString << left << setw(20) << "Product Name" << " Price" << endl;
	str.append(headString.str());
	unsigned int i, n;
	n = static_cast<unsigned int>(code39ItemToProductTable.size());
	for (i = 0; i < n; i++) {
		try {
			str.append(code39ItemToProductTable.atIndex(i)->toString()).append(
					"\n");
		} catch (...) {

		}
	}
	return str;
}
/*
 * Cart Implementation
 */
Cart::Cart() {
	cartNumber = 0;
	priceTotal = 0;
}
Cart::Cart(unsigned int cartNumber_) {
	cartNumber = cartNumber_;
	priceTotal = 0;
}
void Cart::insert(shared_ptr<Product> productPtr) {
	productList.push_back(productPtr);
}
void Cart::calculatePriceTotal() {
	unsigned int i, n;
	n = (unsigned int) productList.size();
	priceTotal = 0;
	for (i = 0; i < n; i++) {
		priceTotal += productList.at(i)->getPrice();
	}
}
string Cart::toString() {
	string str = "";
	stringstream headString, endString;
	calculatePriceTotal();
	str.append("Cart #").append(to_string(cartNumber)).append("\n");
	headString << left << setw(20) << "Product Name" << " Price" << endl;
	str.append(headString.str());
	unsigned int i, n;
	n = (unsigned int) productList.size();
	try {
		for (i = 0; i < n; i++) {
			str.append(productList[i]->toString()).append("\n");
		}
	} catch (...) {
		//nothing
	}
	endString << left << setfill('-') << setw(30) << "" << endl << setfill(' ')
			<< setw(21) << "Total Price" << fixed << setprecision(2)
			<< priceTotal << endl << setfill('-') << setw(30) << "";
	str.append(endString.str());
	return str;
}
/*
 * CartList Implementation
 */
void CartList::insert(shared_ptr<Cart> cartPtr) {
	cartList.push_back(cartPtr);
}
string CartList::toString() {
	string str = "";
	str.append(" Cart List \n-----------\n");
	unsigned int i, n;
	n = static_cast<unsigned int>(cartList.size());
	try {
		for (i = 0; i < n; i++) {
			str.append(cartList[i]->toString()).append("\n\n");
		}
	} catch (...) {
		//nothing
	}
	return str;
}
/*
 * CartLane Implementation
 */
void CartLane::init(shared_ptr<queue<shared_ptr<XMLNode>>> cartXMLNodeQueuePtr_,
		shared_ptr<CartList> cartListPtr_,
		shared_ptr<ProductTable> productTablePtr_) {
	cartXMLNodeQueuePtr = cartXMLNodeQueuePtr_;
	cartListPtr = cartListPtr_;
	productTablePtr = productTablePtr_;
}
bool CartLane::process() {
	unsigned int i, n, cartNumber;
	shared_ptr<XMLNode> cartNodePtr, itemNodePtr;
	shared_ptr<Cart> cartPtr;
	bool flag = false;
	/* keep processing cart XML nodes from the queue until
	 * the queue is empty */
	while (true) {
		/* test if the queue is empty and exit loop */
		mutexGlobal.lock();
		flag = cartXMLNodeQueuePtr->empty();
		mutexGlobal.unlock();
		if (!flag) {
			break;
		}
		cartNodePtr = cartXMLNodeQueuePtr->front();
		/* extract the cart number
		 * stoi could throw exceptions
		 */
		try {
			cartNumber = static_cast<unsigned int>(stoi(
					cartNodePtr->getName().substr(4,
							cartNodePtr->getName().length())));
		} catch (...) {
			cartNumber = 0;
		}
		shared_ptr<Cart> cartPtr = make_shared<Cart>(cartNumber);
		// Assume all children are items
		n = cartNodePtr->childrenSize();
		for (i = 0; i < n; i++) {
			itemNodePtr = cartNodePtr->getChild(i);
			if (itemNodePtr != nullptr) {
				try {
					cartPtr->insert(
							productTablePtr->at(itemNodePtr->getValue()));
				} catch (...) {
					//nothing
				}
			}
		}
		// cart items populated. now insert.
		mutexGlobal.lock();
		cartListPtr->insert(cartPtr);
		mutexGlobal.unlock();
	}
	return true;
}
/*
 * Code39Item Implementation
 */
Code39Item::Code39Item(shared_ptr<Code39CharTable> code39CharTablePtr) {
	code39CharTable = code39CharTablePtr;
}

void Code39Item::setBinaryString(string binaryString_) {
	unsigned int i, n;
	n = static_cast<unsigned int>(binaryString_.size());
	// must have at least one code39 char
	if (n / 9 > 0 && n % 9 == 0) {
		for (i = 0; i < n; i++) {
			bitset<9> bits(binaryString_.substr(i * 9, 9));
			intQueue.push((unsigned int) bits.to_ulong());
		}
	}
}
void Code39Item::setCodeString(string codeString_) {
	unsigned int i, n, temp;
	n = static_cast<unsigned int>(codeString_.size());
	for (i = 0; i < n; i++) {
		code39CharTable->charToInt(codeString_[i], temp);
		intQueue.push(temp);
	}
}

bool Code39Item::toCodeString(string& code39CharItem) {
	bool returnValue = false;
	if (!intQueue.empty()) {
		char temp[2];
		temp[1] = 0;
		code39CharItem = "";
		while (!intQueue.empty()) {
			code39CharTable->intToChar(intQueue.front(), temp[0]);
			code39CharItem.append(temp);
			intQueue.pop();
		}
		returnValue = true;
	}
	return returnValue;
}
bool Code39Item::toBinaryString(string& code39CharItem) {
	bool returnValue = false;
	if (!intQueue.empty()) {
		code39CharItem = "";
		while (!intQueue.empty()) {
			bitset<9> bits(intQueue.front());
			code39CharItem.append(bits.to_string());
			intQueue.pop();
		}
		returnValue = true;
	}
	return returnValue;
}

/*
 * Parser Implementation
 */
Parser::Parser() {
}
bool Parser::productListXMLNodetoObject(
		shared_ptr<XMLNode> productListXMLNodePtr,
		shared_ptr<ProductTable> productTablePtr) {
	bool returnValue = false;
	unsigned int i, n;
	shared_ptr<XMLNode> nodeBarcodeList, nodeProduct, nodeName, nodePrice;
	// BarcodeList level
	if (productListXMLNodePtr->findChild("BarcodeList", nodeBarcodeList, 0)) {
		returnValue = true;
		n = nodeBarcodeList->findChildSize("Product");
		for (i = 0; i < n; i++) {
			if (nodeBarcodeList->findChild("Product", nodeProduct, i)) {
				if (nodeProduct->findChild("Name", nodeName, 0)
						&& nodeProduct->findChild("Price", nodePrice, 0)) {
					if (!productTablePtr->insert(
							make_shared<Product>(nodeName->getValue(),
									stod(nodePrice->getValue())))) {
						// too many hash collisions. discard product.
						cout
								<< "ERROR! Too many collisions. Product Discarded: "
								<< nodeName->getValue() << endl;
					}
				}
			}
		}
	}
	return returnValue;
}
bool Parser::cartListXMLNodetoObject(shared_ptr<XMLNode> cartListXMLNodePtr,
		shared_ptr<CartList> cartListPtr,
		shared_ptr<ProductTable> productTablePtr) {
	bool returnValue = false;
	unsigned int i, n;
	shared_ptr<XMLNode> nodeXMLCarts;
	vector<unique_ptr<thread>> threadPtrList;
	shared_ptr<queue<shared_ptr<XMLNode>>> cartXMLNodeQueuePtr = make_shared<queue<shared_ptr<XMLNode>>>();
	// XMLCarts level
	cout << "testlol " << cartListXMLNodePtr->childrenSize() << endl;
	if (cartListXMLNodePtr->findChild("XMLCarts", nodeXMLCarts, 0)) {
		if (nodeXMLCarts) {
			returnValue = true;
			// create a cart queue
			n = nodeXMLCarts->childrenSize();
			cout << "test " << n << endl;
			for (i = 0; i < n; i++) {
				cout << "test " << nodeXMLCarts->getChild(i)->getName() << endl;
				cartXMLNodeQueuePtr->push(nodeXMLCarts->getChild(i));
			}
		}
		// create the lanes
		vector<shared_ptr<CartLane>> cartLaneList;
		for (i = 0; i < CART_PROCESSING_THREADS; i++) {
			cartLaneList.push_back(make_shared<CartLane>());
			cartLaneList[i]->init(cartXMLNodeQueuePtr, cartListPtr,
					productTablePtr);
			// cartLane[i].process();
			threadPtrList.push_back(
					make_unique<thread>(&CartLane::process, cartLaneList[i]));
		}
		// block threads
		n = static_cast<unsigned int>(threadPtrList.size());
		for (i = 0; i < n; i++) {
			threadPtrList[i]->join();
		}
	}
	return returnValue;
}

/*
 * main & interface
 * Rules For Decoding:
 * - Product and Carts XML are parsed using a generic XML parser
 * - Products are taken from the product XML object and inserted into the product hash table
 * - first 5 letters of product name converted to code39 binary string to use as hash table key
 * - Carts are taken from the cart XML object and reference the product object from the hash table
 * - Cart item code39 binary string used as key for product hash table to find product object
 * - Carts are inserted with product object references into carts list
 * - Carts list calculates the cart price
 * - Carts list converted to a string for writing to a file
 */
int main() {
	FileHandler fh;
	Parser parser;
	XMLParser xmlparser;
	string fileNameProducts, fileNameCarts, fileNameCartsList;
	ifstream fileStreamInProducts, fileStreamInCarts;
	future<bool> parseProductsXMLFuture, parseCartsXMLFuture;
	bool flag = false;
	/* XML input files are here */
	fileNameProducts = "Products.xml";
	fileNameCarts = "Carts.xml";
	fileNameCartsList = "cartsList.txt";
	// parse XML file streams into an XML document node
	shared_ptr<XMLNode> ProductXMLNodePtr = make_shared<XMLNode>("Root");
	shared_ptr<XMLNode> CartXMLNodePtr = make_shared<XMLNode>("Root");
	shared_ptr<ProductTable> productTablePtr = make_shared<ProductTable>();
	shared_ptr<CartList> cartListPtr = make_shared<CartList>();
	if (!fh.readStream(fileNameProducts, fileStreamInProducts)
			|| !fh.readStream(fileNameCarts, fileStreamInCarts)) {
		cout << "Could not read either of the input files." << endl;
	} else {
		cout << "Parsing XML files..." << endl;
		/* we pass file streams instead of a string to this method
		 * because we want to stream the data and decode it as we read.
		 * This way very large files won't lag or crash the program.
		 */
		parseProductsXMLFuture = async(&XMLParser::documentStream, &xmlparser,
				ref((istream&) fileStreamInProducts), ref(ProductXMLNodePtr));
		parseCartsXMLFuture = async(&XMLParser::documentStream, &xmlparser,
				ref((istream&) fileStreamInCarts), ref(CartXMLNodePtr));
		if (parseProductsXMLFuture.get()) {
			cout
					<< "Successfully parsed Product List XML File to Product List Nodes."
					<< endl;
			// create the product table from the product list XML
			if (parser.productListXMLNodetoObject(ProductXMLNodePtr,
					productTablePtr)) {
				//fh.writeString("productList.txt", productTable.toString());
				cout
						<< "Successfully parsed Product List XML Nodes into hash table."
						<< endl;
				flag = true;
			} else {
				cout
						<< "Failed to parse Product List XML Nodes into hash table."
						<< endl;
			}
		}
	}
	// process each cart from the XML file referencing each product from the product table
	if (flag && parseCartsXMLFuture.get()) {
		cout << "Successfully parsed Cart List XML file into cart list nodes."
				<< endl << "Processing Carts..." << endl;
		// close the XML files
		fh.close(fileStreamInProducts);
		fh.close(fileStreamInCarts);
		//cout << "XML Files successfully parsed!" << endl;
		if (parser.cartListXMLNodetoObject(CartXMLNodePtr, cartListPtr,
				productTablePtr)) {
			if (fh.writeString(fileNameCartsList, cartListPtr->toString())) {
				cout << "Carts list written to \"" << fileNameCartsList << "\""
						<< endl;
			}
		}
	}
	cout << "Enter any key to exit..." << endl;
	string temp;
	getline(cin, temp);
	return 0;
}
