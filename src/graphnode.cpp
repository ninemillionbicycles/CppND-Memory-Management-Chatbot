#include "graphedge.h"
#include "graphnode.h"

GraphNode::GraphNode(int id) { _id = id; }

GraphNode::~GraphNode() {}

void GraphNode::AddToken(std::string token)
{
    _answers.emplace_back(token);
}

void GraphNode::AddEdgeToParentNode(GraphEdge *edge)
{
    _parentEdges.emplace_back(edge);
}

void GraphNode::AddEdgeToChildNode(std::unique_ptr<GraphEdge> edge)
{
    _childEdges.emplace_back(std::move(edge));
}

void GraphNode::MoveChatbotHere(ChatBot chatbot)
{
    _chatBot = std::move(chatbot);
    _chatBot.SetCurrentNode(this);
}

void GraphNode::MoveChatbotToNewNode(GraphNode *newNode)
{
    newNode->MoveChatbotHere(std::move(_chatBot));
}

GraphEdge *GraphNode::GetChildEdgeAtIndex(int index)
{
    return _childEdges[index].get();
}