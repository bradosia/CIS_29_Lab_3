//============================================================================
// Name        : Lab3
// Author      : Branden Lee
// Date        : 5/15/2018
// Description : Decoding Code3of9 Symbology in XML files with threads
//============================================================================

#include <string>
#include <fstream>
#include <iostream>     // std::cout
#include <sstream>
#include <iomanip>
#include <vector>       // std::vector
#include <bitset>
#include <list>
#include <regex>
#include <stack>
#include <queue>
#include <cstring>
#include <thread>
#include <future>
#include <mutex>          // std::mutex
using namespace std;

#define BUFFER_SIZE 256
#define CART_PROCESSING_THREADS 7

mutex mutexGlobal;           // mutex for critical section

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
	vector<XMLNode*> childNodes;
	XMLNode* parentNode;
public:
	XMLNode();
	XMLNode(string name_, XMLNode* parentNode_);
	~XMLNode() {
	}
	void valueAppend(string str);
	/* not a comprehensive list of definitions for all getters/setters
	 * it is not vital to the program to have all setters
	 */
	string getName();
	string getValue();
	XMLNode* getParent();
	XMLNode* addChild(string str);
	XMLNode* getChild(unsigned int index);
	bool findChild(string name_, XMLNode*& returnNode, unsigned int index);
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
	~XMLParser() {
	}
	bool documentStream(istream& streamIn, XMLNode& xmlDoc);
	bool bufferSearch(string& streamBuffer, XMLNode& xmlDoc,
			XMLNode*& xmlNodeCurrent, stack<string>& documentStack,
			unsigned int mode);
	bool nodePop(string& tagString, XMLNode& xmlDoc, XMLNode*& xmlNodeCurrent,
			stack<string>& documentStack);
	bool nodePush(string& tagString, XMLNode& xmlDoc, XMLNode*& xmlNodeCurrent,
			stack<string>& documentStack);
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
	Code39CharTable code39CharTable;
	HashTable<string, Product*> code39ItemToProductTable;
public:
	ProductTable();
	~ProductTable() {
	}
	/*
	 * @param key The code39 Binary String of first 5 characters of product name
	 * @param val pointer to the product
	 */
	bool insert(string key, Product* val);
	/*
	 * Generates the key automatically
	 * @param val pointer to the product
	 */
	bool insert(Product* val);
	/*
	 * WARNING: could throw an exception
	 */
	Product* at(string key);
	string toString();

};

class Cart {
private:
	vector<Product*> productList;
	unsigned int cartNumber;
	double priceTotal;
public:
	Cart();
	Cart(unsigned int cartNumber_);
	~Cart() {
	}
	void insert(Product* productPtr);
	void calculatePriceTotal();
	string toString();
};

class CartList {
private:
	vector<Cart*> cartList;
public:
	CartList() {
	}
	~CartList() {
	}
	void insert(Cart* cardPtr);
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
	XMLNode* nodeXMLCarts;
	CartList* cartListObject;
	ProductTable* productTableObject;
	unsigned int indexStart;
	unsigned int indexStop;
public:
	CartLane() {
		nodeXMLCarts = NULL;
		cartListObject = NULL;
		productTableObject = NULL;
		indexStart = indexStop = 0;
	}
	~CartLane() {
	}
	void init(XMLNode*& nodeXMLCarts_, CartList& cartListObject_,
			ProductTable& productTableObject_);
	void range(unsigned int indexStart_, unsigned int indexStop_);
	bool process();
};

/**
 @class Code39Item
 Converts a Code 39 Item to the a product \n
 */
class Code39Item {
private:
	Code39CharTable * code39CharTable;
	string binaryString, codeString;
	queue<int> intQueue;
public:
	Code39Item(Code39CharTable* code39CharTablePtr);
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
	bool productListXMLNodetoObject(XMLNode& productListXMLNode,
			ProductTable& productTableObject);
	bool cartListXMLNodetoObject(XMLNode& cartListXMLNode,
			CartList& cartListObject, ProductTable& productTableObject);
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
	return table.size();
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
	parentNode = nullptr;
}
XMLNode::XMLNode(string name_, XMLNode* parentNode_) {
	name = name_;
	value = "";
	parentNode = parentNode_;
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
XMLNode* XMLNode::getParent() {
	return parentNode;
}
XMLNode* XMLNode::addChild(string str) {
	XMLNode* childNode = new XMLNode(str, this);
	childNodes.push_back(childNode);
	return childNode;
}
XMLNode* XMLNode::getChild(unsigned int index) {
	XMLNode* nodeReturn = nullptr;
	try {
		nodeReturn = childNodes.at(index);
	} catch (...) {
		// nothing
	}
	return nodeReturn;
}
bool XMLNode::findChild(string name_, XMLNode*& returnNode,
		unsigned int index) {
	unsigned int findCount, i, n;
	bool returnValue = false;
	findCount = 0;
	n = (unsigned int) childNodes.size();
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
	return (unsigned int) childNodes.size();
}
unsigned int XMLNode::findChildSize(string name_) {
	unsigned int findCount, i, n;
	findCount = 0;
	n = (unsigned int) childNodes.size();
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
bool XMLParser::documentStream(istream& streamIn, XMLNode& xmlDoc) {
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
	stack<string> documentStack;
	bufferSize = BUFFER_SIZE;
	fileSize = filePos = mode = 0;
	streamBuffer = "";
	XMLNode* xmlNodeCurrent = &xmlDoc;
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
		bufferSearch(streamBuffer, xmlDoc, xmlNodeCurrent, documentStack, mode);
		// advance buffer
		filePos += bufferSize;
	}
	// remaining buffer belongs to current node value
	xmlNodeCurrent->valueAppend(streamBuffer);
	return true;
}

bool XMLParser::bufferSearch(string& streamBuffer, XMLNode& xmlDoc,
		XMLNode*& xmlNodeCurrent, stack<string>& documentStack,
		unsigned int mode) {
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
					//cout << s;
					nodePop(matchGroupString, xmlDoc, xmlNodeCurrent,
							documentStack);
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
						string s;
						s.append("<").append(matchGroupString).append("> ").append(
								streamBuffer).append("\n");
						//cout << s;
						nodePush(matchGroupString, xmlDoc, xmlNodeCurrent,
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

bool XMLParser::nodePop(string& tagString, XMLNode& xmlDoc,
		XMLNode*& xmlNodeCurrent, stack<string>& documentStack) {
	/* pop nodes off stack until end tag is found
	 * can't go higher than the document root
	 */
	string tagPop = "";
	if (tagString.length() > 0) {
		while (!documentStack.empty() && tagPop != tagString) {
			tagPop = documentStack.top();
			documentStack.pop();
			xmlNodeCurrent = xmlNodeCurrent->getParent();
			if (xmlNodeCurrent == nullptr) {
				xmlNodeCurrent = &xmlDoc;
			}
		}
	}
	return true;
}

bool XMLParser::nodePush(string& tagString, XMLNode& xmlDoc,
		XMLNode*& xmlNodeCurrent, stack<string>& documentStack) {
	if (tagString.length() > 0) {
		documentStack.push(tagString);
		xmlNodeCurrent = xmlNodeCurrent->addChild(tagString);
		if (xmlNodeCurrent == nullptr) {
			xmlNodeCurrent = &xmlDoc;
		}
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
	code39ItemToProductTable.resize(1700);
}
bool ProductTable::insert(string key, Product* val) {
	return code39ItemToProductTable.insert(key, val);
}
bool ProductTable::insert(Product* val) {
	bool returnValue = false;
	Code39Item code39Item(&code39CharTable);
	code39Item.setCodeString(val->getName().substr(0, 5));
	string code39BinaryString;
	if (code39Item.toBinaryString(code39BinaryString)) {
		/*cout << val->getName().substr(0, 5) << " = " << code39BinaryString
		 << endl;*/
		returnValue = code39ItemToProductTable.insert(code39BinaryString, val);
	}
	return returnValue;
}
Product* ProductTable::at(string key) {
	return code39ItemToProductTable.at(key);
}
string ProductTable::toString() {
	string str = "";
	str.append(" Product Table \n---------------\n");
	stringstream headString, endString;
	headString << left << setw(20) << "Product Name" << " Price" << endl;
	str.append(headString.str());
	unsigned int i, n;
	n = code39ItemToProductTable.size();
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
void Cart::insert(Product* productPtr) {
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
void CartList::insert(Cart* cartPtr) {
	cartList.push_back(cartPtr);
}
string CartList::toString() {
	string str = "";
	str.append(" Cart List \n-----------\n");
	unsigned int i, n;
	n = cartList.size();
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
void CartLane::init(XMLNode*& nodeXMLCarts_, CartList& cartListObject_,
		ProductTable& productTableObject_) {
	nodeXMLCarts = nodeXMLCarts_;
	cartListObject = &cartListObject_;
	productTableObject = &productTableObject_;
}
void CartLane::range(unsigned int indexStart_, unsigned int indexStop_) {
	indexStart = indexStart_;
	indexStop = indexStop_;
}
bool CartLane::process() {
	unsigned int i, n, j, n1, cartNumber;
	XMLNode *nodeCart, *nodeItem;
	Cart* cartPtr = NULL;
	// Assume all children are carts
	n = indexStop;
	for (i = indexStart; i < n; i++) {
		nodeCart = nodeXMLCarts->getChild(i);
		if (nodeCart != nullptr) {
			/* extract the cart number
			 * stoi could throw exceptions
			 */
			try {
				cartNumber = (unsigned int) stoi(
						nodeCart->getName().substr(4,
								nodeCart->getName().length()));
			} catch (...) {
				cartNumber = 0;
			}
			cartPtr = new Cart(cartNumber);
			// Assume all children are items
			n1 = nodeCart->childrenSize();
			for (j = 0; j < n1; j++) {
				nodeItem = nodeCart->getChild(j);
				if (nodeItem != nullptr) {
					try {
						cartPtr->insert(
								productTableObject->at(nodeItem->getValue()));
					} catch (...) {
						//nothing
					}
				}
			}
		}
		mutexGlobal.lock();
		cartListObject->insert(cartPtr);
		mutexGlobal.unlock();
	}
	return true;
}
/*
 * Code39Item Implementation
 */
Code39Item::Code39Item(Code39CharTable* code39CharTablePtr) {
	code39CharTable = code39CharTablePtr;
}

void Code39Item::setBinaryString(string binaryString_) {
	unsigned int i, n;
	n = binaryString_.size();
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
	n = codeString_.size();
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
bool Parser::productListXMLNodetoObject(XMLNode& productListXMLNode,
		ProductTable& productTableObject) {
	bool returnValue = false;
	unsigned int i, n;
	XMLNode* nodeBarcodeList, *nodeProduct, *nodeName, *nodePrice;
	// BarcodeList level
	if (productListXMLNode.findChild("BarcodeList", nodeBarcodeList, 0)) {
		returnValue = true;
		n = nodeBarcodeList->findChildSize("Product");
		for (i = 0; i < n; i++) {
			if (nodeBarcodeList->findChild("Product", nodeProduct, i)) {
				if (nodeProduct->findChild("Name", nodeName, 0)
						&& nodeProduct->findChild("Price", nodePrice, 0)) {
					if (!productTableObject.insert(
							new Product(nodeName->getValue(),
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
bool Parser::cartListXMLNodetoObject(XMLNode& cartListXMLNode,
		CartList& cartListObject, ProductTable& productTableObject) {
	bool returnValue = false;
	unsigned int i, n, cartNumber, cartLaneRange;
	XMLNode* nodeXMLCarts;
	vector<thread*> threadList;
	// XMLCarts level
	if (cartListXMLNode.findChild("XMLCarts", nodeXMLCarts, 0)) {
		returnValue = true;
		// create the lanes
		CartLane cartLane[CART_PROCESSING_THREADS];
		n = nodeXMLCarts->childrenSize();
		cartLaneRange = n / CART_PROCESSING_THREADS;
		// distribute carts in lanes
		for (i = 0; i < CART_PROCESSING_THREADS; i++) {
			cartLane[i].init(nodeXMLCarts, cartListObject, productTableObject);
			if (i + 1 == CART_PROCESSING_THREADS) {
				cartLane[i].range(i * cartLaneRange, n + 1);
			} else {
				cartLane[i].range(i * cartLaneRange, (i + 1) * cartLaneRange);
			}
			// cartLane[i].process();
			threadList.push_back(new thread(&CartLane::process, &cartLane[i]));
		}
		// block threads
		n = threadList.size();
		for (i = 0; i < n; i++) {
			threadList[i]->join();
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
	XMLNode ProductsXML, CartsXML;
	ProductTable productTable;
	CartList cartList;
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
				ref((istream&) fileStreamInProducts), ref(ProductsXML));
		parseCartsXMLFuture = async(&XMLParser::documentStream, &xmlparser,
				ref((istream&) fileStreamInCarts), ref(CartsXML));
		if (parseProductsXMLFuture.get()) {
			cout
					<< "Successfully parsed Product List XML File to Product List Nodes."
					<< endl;
			// create the product table from the product list XML
			if (parser.productListXMLNodetoObject(ProductsXML, productTable)) {
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
		if (parser.cartListXMLNodetoObject(CartsXML, cartList, productTable)) {
			if (fh.writeString(fileNameCartsList, cartList.toString())) {
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
