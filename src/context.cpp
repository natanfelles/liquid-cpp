#include "context.h"
#include "optimizer.h"

namespace Liquid {

    bool NodeType::optimize(Optimizer& optimizer, Node& node, Variable store) const {
        node = render(optimizer.renderer, node, store);
        return true;
    }

    bool Context::VariableNode::optimize(Optimizer& optimizer, Node& node, Variable store) const {
        Variable storePointer = optimizer.renderer.getVariable(node, store);
        if (!storePointer.exists())
            return false;
        node = Node(optimizer.renderer.parseVariant(storePointer));
        return true;
    }

    Node NodeType::render(Renderer& renderer, const Node& node, Variable store) const {
        if (!userRenderFunction)
            return Node();
        userRenderFunction(LiquidRenderer{&renderer}, LiquidNode{const_cast<Node*>(&node)}, store, userData);
        return renderer.returnValue;
    }

    Node FilterNodeType::getOperand(Renderer& renderer, const Node& node, Variable store) const {
        return renderer.retrieveRenderedNode(*node.children[0].get(), store);
    }
    Node DotFilterNodeType::getOperand(Renderer& renderer, const Node& node, Variable store) const {
        return renderer.retrieveRenderedNode(*node.children[0].get(), store);
    }


    Node NodeType::getArgument(Renderer& renderer, const Node& node, Variable store, int idx) const {
        int offset = node.type->type == NodeType::Type::TAG ? 0 : 1;
        if (idx >= (int)node.children[offset]->children.size())
            return Node();
        assert(node.children[offset]->type->type == NodeType::Type::ARGUMENTS);
        return renderer.retrieveRenderedNode(*node.children[offset]->children[idx].get(), store);
    }
    int NodeType::getArgumentCount(const Node& node) const {
        int offset = node.type->type == NodeType::Type::TAG ? 0 : 1;
        assert(node.children[offset]->type->type == NodeType::Type::ARGUMENTS);
        return node.children[offset]->children.size();
    }

    Node NodeType::getChild(Renderer& renderer, const Node& node, Variable store, int idx) const {
        if (idx >= (int)node.children.size())
            return Node();
        return renderer.retrieveRenderedNode(*node.children[idx].get(), store);
    }
    int NodeType::getChildCount(const Node& node) const {
        return node.children.size();
    }


    Node Context::ConcatenationNode::render(Renderer& renderer, const Node& node, Variable store) const {
        string s;
        if (++renderer.currentRenderingDepth > renderer.maximumRenderingDepth) {
            --renderer.currentRenderingDepth;
            renderer.error = LIQUID_RENDERER_ERROR_TYPE_EXCEEDED_DEPTH;
            return Node();
        }
        for (auto& child : node.children) {
            s.append(renderer.retrieveRenderedNode(*child.get(), store).getString());
            if (renderer.error != LIQUID_RENDERER_ERROR_TYPE_NONE)
                return Node();
            if (renderer.control != Renderer::Control::NONE) {
                --renderer.currentRenderingDepth;
                return Node(s);
            }
        }
        --renderer.currentRenderingDepth;
        return Node(s);
    }
    bool Context::ConcatenationNode::optimize(Optimizer& optimizer, Node& node, Variable store) const {
        if (++optimizer.renderer.currentRenderingDepth > optimizer.renderer.maximumRenderingDepth) {
            --optimizer.renderer.currentRenderingDepth;
            return false;
        }
        string s;
        vector<unique_ptr<Node>> newChildren;
        for (auto& child : node.children) {
            if (child->type) {
                if (!s.empty())
                    newChildren.push_back(make_unique<Node>(Variant(move(s))));
                newChildren.push_back(move(child));
            } else
                s.append(child->getString());
        }
        if (newChildren.size() == 0) {
            node = Variant(move(s));
        } else {
            newChildren.push_back(make_unique<Node>(Variant(move(s))));
            node.children = move(newChildren);
        }
        --optimizer.renderer.currentRenderingDepth;
        return true;
    }

};
