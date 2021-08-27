#include <iostream>
#include <random>
#include <algorithm>
#include <ctime>
#include "chatlogic.h"
#include "graphnode.h"
#include "graphedge.h"
#include "chatbot.h"

// constructor WITHOUT memory allocation
ChatBot::ChatBot()
{
    // invalidate data handles
    _image = nullptr;
    _chatLogic = nullptr;
    _rootNode = nullptr;
}

// constructor WITH memory allocation
ChatBot::ChatBot(std::string filename)
{
    std::cout << "ChatBot Constructor: Create object at " << this << std::endl;

    // invalidate data handles
    _chatLogic = nullptr;
    _rootNode = nullptr;

    // load image into heap memory
    _image = new wxBitmap(filename, wxBITMAP_TYPE_PNG);
}

// 1. destructor
ChatBot::~ChatBot()
{
    std::cout << "ChatBot Destructor: Destroy object at " << this << std::endl;

    // deallocate heap memory
    if (_image != NULL) // Attention: wxWidgets used NULL and not nullptr
    {
        delete _image;
        _image = NULL;
    }
}

// 2. copy constructor with exclusive ownership policy
ChatBot::ChatBot(const ChatBot &source)
{
    std::cout << "ChatBot Copy Constructor: Copy object at " << &source << " to " << this << std::endl;

    // create deep copy for owned data
    _image = new wxBitmap();
    *_image = *source._image;

    // create shallow copy for non-owned data
    _currentNode = source._currentNode;
    _rootNode = source._rootNode;
    _chatLogic = source._chatLogic;

    // make sure that this copy can be handled by chatLogic
    _chatLogic->SetChatbotHandle(this);
}

// 3. copy assignment operator
ChatBot &ChatBot::operator=(const ChatBot &source)
{
    std::cout << "ChatBot Copy Assignment Operator: Copy object at " << &source << " to " << this << std::endl;
    
    // protect against self-assignment
    if (this == &source) { return *this; } 

    // delete owned data if it exists in this._image
    if (_image != NULL) { delete _image; }

    // create deep copy for owned data
    _image = new wxBitmap();
    *_image = *source._image;

    // create shallow copy for non-owned data
    _currentNode = source._currentNode;
    _rootNode = source._rootNode;
    _chatLogic = source._chatLogic;

    // make sure that this copy can be handled by chatLogic
    _chatLogic->SetChatbotHandle(this);

    return *this;
}

// 4. move constructor
ChatBot::ChatBot(ChatBot &&source)
{
    std::cout << "ChatBot Move Constructor: Move object from " << &source << " to " << this << std::endl;
    
    // create a shallow copy of image as this is the **move** constructor and we want to avoid deep copies where possible
    _image = source._image;
    
    // invalidate the data handle of source to NULL so that the heap memory allocated to _image is not deallocated when the destructor is called for source
    source._image = NULL;

    // create shallow copy for non-owned data
    _currentNode = source._currentNode;
    _rootNode = source._rootNode;
    _chatLogic = source._chatLogic;

    // make sure that this copy can be handled by chatLogic
    _chatLogic->SetChatbotHandle(this);

    // invalidate source handles for non-owned data
    source._currentNode = nullptr;
    source._rootNode = nullptr;
    source._chatLogic = nullptr;
}

// 5. move assignment operator with exclusive ownership policy
ChatBot &ChatBot::operator=(ChatBot &&source)
{
    std::cout << "ChatBot Move Assignment Operator: Move object from " << &source << " to " << this << std::endl;

    // protect against self-assignment
    if (this == &source) { return *this; } 

    // delete owned data if it exists in this._image
    if (_image != NULL) { delete _image; }

    // create a shallow copy of image as this is the **move** constructor and we want to avoid deep copies where possible
    // invalidate the data handle of source to NULL so that the heap memory allocated to _image is not deallocated when the destructor is called for source
    _image = source._image;
    source._image = NULL;

    // create shallow copy for non-owned data
    _currentNode = source._currentNode;
    _rootNode = source._rootNode;
    _chatLogic = source._chatLogic;

    // make sure that this copy can be handled by chatLogic
    _chatLogic->SetChatbotHandle(this);

    // invalidate source handles for non-owned data
    source._currentNode = nullptr;
    source._rootNode = nullptr;
    source._chatLogic = nullptr;
    
    return *this;
}

void ChatBot::ReceiveMessageFromUser(std::string message)
{
    // loop over all edges and keywords and compute Levenshtein distance to query
    typedef std::pair<GraphEdge *, int> EdgeDist;
    std::vector<EdgeDist> levDists; // format is <ptr,levDist>

    for (size_t i = 0; i < _currentNode->GetNumberOfChildEdges(); ++i)
    {
        GraphEdge *edge = _currentNode->GetChildEdgeAtIndex(i);
        for (auto keyword : edge->GetKeywords())
        {
            EdgeDist ed{edge, ComputeLevenshteinDistance(keyword, message)};
            levDists.push_back(ed);
        }
    }

    // select best fitting edge to proceed along
    GraphNode *newNode;
    if (levDists.size() > 0)
    {
        // sort in ascending order of Levenshtein distance (best fit is at the top)
        std::sort(levDists.begin(), levDists.end(), [](const EdgeDist &a, const EdgeDist &b)
                  { return a.second < b.second; });
        newNode = levDists.at(0).first->GetChildNode(); // after sorting the best edge is at first position
    }
    else
    {
        // go back to root node
        newNode = _rootNode;
    }

    // tell current node to move chatbot to new node
    _currentNode->MoveChatbotToNewNode(newNode);
}

void ChatBot::SetCurrentNode(GraphNode *node)
{
    // update pointer to current node
    _currentNode = node;

    // select a random node answer (if several answers should exist)
    std::vector<std::string> answers = _currentNode->GetAnswers();
    std::mt19937 generator(int(std::time(0)));
    std::uniform_int_distribution<int> dis(0, answers.size() - 1);
    std::string answer = answers.at(dis(generator));

    // send selected node answer to user
    _chatLogic->SendMessageToUser(answer);
}

int ChatBot::ComputeLevenshteinDistance(std::string s1, std::string s2)
{
    // convert both strings to upper-case before comparing
    std::transform(s1.begin(), s1.end(), s1.begin(), ::toupper);
    std::transform(s2.begin(), s2.end(), s2.begin(), ::toupper);

    // compute Levenshtein distance measure between both strings
    const size_t m(s1.size());
    const size_t n(s2.size());

    if (m == 0)
        return n;
    if (n == 0)
        return m;

    size_t *costs = new size_t[n + 1];

    for (size_t k = 0; k <= n; k++)
        costs[k] = k;

    size_t i = 0;
    for (std::string::const_iterator it1 = s1.begin(); it1 != s1.end(); ++it1, ++i)
    {
        costs[0] = i + 1;
        size_t corner = i;

        size_t j = 0;
        for (std::string::const_iterator it2 = s2.begin(); it2 != s2.end(); ++it2, ++j)
        {
            size_t upper = costs[j + 1];
            if (*it1 == *it2)
            {
                costs[j + 1] = corner;
            }
            else
            {
                size_t t(upper < corner ? upper : corner);
                costs[j + 1] = (costs[j] < t ? costs[j] : t) + 1;
            }

            corner = upper;
        }
    }

    int result = costs[n];
    delete[] costs;

    return result;
}