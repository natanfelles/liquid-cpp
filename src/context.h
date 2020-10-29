#ifndef CONTEXT_H
#define CONTEXT_H

#include "common.h"
#include "parser.h"

namespace Liquid {

    struct Variable {
        enum class Type {
            NIL,
            FLOAT,
            INT,
            STRING,
            ARRAY,
            DICTIONARY,
            OTHER
        };

        virtual Type getType() { return Type::NIL; }
        virtual bool getString(std::string& str) { return false; }
        virtual bool getInteger(long long& i) { return false; }
        virtual bool getFloat(double& i) { return false; }
        virtual bool getDictionaryVariable(Variable*& variable, const std::string& key) { return false;  }
        virtual bool getArrayVariable(Variable*& variable, size_t idx) { return false; }
    };

    struct NodeType {
        enum class Type {
            VARIABLE,
            TAG_ENCLOSED,
            TAG_FREE,
            GROUP,
            OUTPUT,
            ARGUMENTS,
            OPERATOR
        };
        Type type;
        string symbol;
        int maxChildren;
        // For things like if/else, and whatnot. else is a free tag that sits inside the if statement.
        unordered_map<string, unique_ptr<NodeType>> intermediates;

        NodeType(Type type, string symbol = "", int maxChildren = -1) : type(type), symbol(symbol), maxChildren(maxChildren) { }
        NodeType(const NodeType&) = default;
        NodeType(NodeType&&) = default;
        ~NodeType() { }

        // When a node is rendered, depending on its mode, it'll return a node.
        virtual Parser::Node render(const Context& context, const Parser::Node& node, Variable& store) const = 0;
        virtual void optimize(const Context& context, Parser::Node& node, Variable& store) const { }


        Parser::Node (*renderFunction)(const Context& context, const Parser::Node& node, Variable& store) =
            +[](const Context& context, const Parser::Node& node, Variable& store) {
                return node.type->render(context, node, store);
            };
        void (*optimizeFunction)(const Context& context, Parser::Node& node, Variable& store) =
            +[](const Context& context, Parser::Node& node, Variable& store) {
                node.type->optimize(context, node, store);
            };
    };

    struct TagNodeType : NodeType {
        int minArguments;
        int maxArguments;

        TagNodeType(Type type, string symbol, int minArguments = -1, int maxArguments = -1) : NodeType(type, symbol, -1), minArguments(minArguments), maxArguments(maxArguments) { }
    };

    struct OperatorNodeType : NodeType {
        enum class Arity {
            NONARY,
            UNARY,
            BINARY,
            NARY
        };
        Arity arity;
        int priority;

        enum class Fixness {
            PREFIX,
            INFIX,
            AFFIX
        };
        Fixness fixness;

        OperatorNodeType(string symbol, Arity arity, int priority = 0, Fixness fixness = Fixness::INFIX) : NodeType(Type::OPERATOR, symbol), arity(arity), priority(priority), fixness(fixness) {
            switch (arity) {
                case Arity::NONARY:
                    maxChildren = 0;
                break;
                case Arity::UNARY:
                    maxChildren = 1;
                break;
                case Arity::BINARY:
                    maxChildren = 2;
                break;
                case Arity::NARY:
                    maxChildren = -1;
                break;
            }
        }
    };

    struct Context {
        struct ConcatenationNode : NodeType {
            ConcatenationNode() : NodeType(Type::OPERATOR) { }

            Parser::Node render(const Context& context, const Parser::Node& node, Variable& store) const {
                string s;
                for (auto it = node.children.begin(); it != node.children.end(); ++it) {
                    if ((*it)->type) {
                        s.append((*it)->type->render(context, *it->get(), store).getString());
                    } else {
                        s.append((*it)->getString());
                    }
                }
                return Parser::Node(s);
            }
        };

        struct OutputNode : NodeType {
            OutputNode() : NodeType(Type::OUTPUT) { }

            Parser::Node render(const Context& context, const Parser::Node& node, Variable& store) const {
                assert(node.children.size() == 1);

                auto& argumentNode = node.children.front();
                assert(argumentNode->children.size() == 1);
                auto it = argumentNode->children.begin();
                if ((*it)->type)
                    return (*it)->type->render(context, *it->get(), store);
                return *it->get();
            }
        };

        // These are purely for parsing purpose, and should not make their way to the rednerer.
        struct GroupNode : NodeType {
            GroupNode() : NodeType(Type::GROUP) { }
            Parser::Node render(const Context& context, const Parser::Node& node, Variable& store) const { assert(false); }
        };
        // Used exclusively for tags
        struct ArgumentNode : NodeType {
            ArgumentNode() : NodeType(Type::ARGUMENTS) { }
            Parser::Node render(const Context& context, const Parser::Node& node, Variable& store) const { assert(false); }
        };

        struct VariableNode : NodeType {
            VariableNode() : NodeType(Type::VARIABLE) { }

            Parser::Node render(const Context& context, const Parser::Node& node, Variable& store) const {
                auto& chain = node.children;
                Variable* storePointer = &store;
                for (auto it = chain.begin(); it != chain.end(); ++it) {
                    auto node = (*it)->type ? (*it)->type->render(context, *it->get(), store) : **it;
                    assert(!node.type);
                    switch (node.variant.type) {
                        case Parser::Variant::Type::INT:
                            if (!storePointer->getArrayVariable(storePointer, node.variant.i))
                                storePointer = nullptr;
                        break;
                        case Parser::Variant::Type::STRING:
                            if (!storePointer->getDictionaryVariable(storePointer, node.variant.s))
                                storePointer = nullptr;
                        break;
                        default:
                            storePointer = nullptr;
                        break;
                    }
                }
                if (!storePointer)
                    return Parser::Node(Parser::Variant());
                switch (storePointer->getType()) {
                    case Variable::Type::NIL: {
                        return Parser::Node(Parser::Variant());
                    } break;
                    case Variable::Type::FLOAT: {
                        double f;
                        storePointer->getFloat(f);
                        return Parser::Node(Parser::Variant(f));
                    } break;
                    case Variable::Type::STRING: {
                        std::string s;
                        storePointer->getString(s);
                        return Parser::Node(Parser::Variant(s));
                    } break;
                    case Variable::Type::INT: {
                        long long i;
                        storePointer->getInteger(i);
                        return Parser::Node(Parser::Variant(i));
                    } break;
                    default:
                        return Parser::Node(Parser::Variant(storePointer));
                    break;
                }

            }
        };

        struct NamedVariableNode : VariableNode {

        };

        unordered_map<string, unique_ptr<NodeType>> tagTypes;
        unordered_map<string, unique_ptr<NodeType>> operatorTypes;

        const NodeType* getConcatenationNodeType() const { static ConcatenationNode concatenationNodeType; return &concatenationNodeType; }
        const NodeType* getOutputNodeType() const { static OutputNode outputNodeType; return &outputNodeType; }
        const NodeType* getVariableNodeType() const { static VariableNode variableNodeType; return &variableNodeType; }
        const NodeType* getNamedVariableNodeType() const { static NamedVariableNode namedVariableNodeType; return &namedVariableNodeType; }
        const NodeType* getGroupNodeType() const { static GroupNode groupNodeType; return &groupNodeType; }
        const NodeType* getArgumentsNodeType() const { static ArgumentNode argumentNodeType; return &argumentNodeType; }

        struct CPPVariable : Variable {
            union {
                string s;
                double f;
                long long i;
                vector<unique_ptr<CPPVariable>> a;
                unordered_map<string, unique_ptr<CPPVariable>> d;
            };
            Type type;



            CPPVariable() :type(Type::NIL) { }
            CPPVariable(long long i) : i(i), type(Type::INT) { }
            CPPVariable(int i) : CPPVariable((long long)i) { }
            CPPVariable(double f) : f(f), type(Type::FLOAT) { }
            CPPVariable(const string& s) : s(s), type(Type::STRING) { }
            CPPVariable(const char* s) : CPPVariable(string(s)) { }
            CPPVariable(void* nil) : type(Type::NIL) { }
            ~CPPVariable() { clear(); }

            void clear() {
                switch (type) {
                    case Type::STRING:
                        s.~string();
                    break;
                    case Type::DICTIONARY:
                        d.~unordered_map<string, unique_ptr<CPPVariable>>();
                    break;
                    case Type::ARRAY:
                        a.~vector<unique_ptr<CPPVariable>>();
                    break;
                    default:
                    break;
                }
                type = Type::NIL;
            }

            void assign(long long i) { clear(); this->i = i; type = Type::INT; }
            void assign(int i) { assign((long long)i); }
            void assign(double f) { clear(); this->f = f; type = Type::FLOAT; }
            void assign(const string& s) { clear(); new(&this->s) string(); this->s = s; type = Type::STRING; }

            CPPVariable& operator[](const string& s) {
                if (type == Type::NIL) {
                    new(&this->d) unordered_map<string, unique_ptr<CPPVariable>>();
                    type = Type::DICTIONARY;
                }
                assert(type == Type::DICTIONARY);
                auto it = d.find(s);
                if (it != d.end())
                    return *it->second.get();
                return *d.emplace(s, make_unique<CPPVariable>()).first->second.get();
            }
            CPPVariable& operator[](size_t idx) {
                if (type == Type::NIL) {
                    new(&this->a) vector<unique_ptr<CPPVariable>>();
                    type = Type::ARRAY;
                }
                assert(type == Type::ARRAY);
                if (a.size() < idx+1) {
                    size_t oldSize = a.size();
                    a.resize(idx+1);
                    for (size_t i = oldSize; i < idx+1; ++i)
                        a[i] = make_unique<CPPVariable>();
                }
                return *a[idx].get();
            }

            CPPVariable& operator = (const std::string& s) { assign(s); return *this; }
            CPPVariable& operator = (const char* s) { assign(s); return *this; }
            CPPVariable& operator = (double f) { assign(f); return *this; }
            CPPVariable& operator = (long long i) { assign(i); return *this; }
            CPPVariable& operator = (int i) { assign(i); return *this; }


            Type getType() {
                return type;
            }

            bool getString(std::string& s) {
                if (type != Type::STRING)
                    return false;
                s = this->s;
                return true;
            }
            bool getInteger(long long& i) {
                if (type != Type::INT)
                    return false;
                i = this->i;
                return true;
            }
            bool getFloat(double& i) {
                if (type != Type::FLOAT)
                    return false;
                f = this->f;
                return true;
            }
            bool getDictionaryVariable(Variable*& variable, const std::string& key) {
                if (type != Type::DICTIONARY)
                    return false;
                variable = &(*this)[key];
                return true;
            }
            bool getArrayVariable(Variable*& variable, size_t idx) {
                if (type != Type::ARRAY)
                    return false;
                variable = &(*this)[idx];
                return true;
            }
        };

        void registerType(unique_ptr<NodeType> type) {
            switch (type->type) {
                case NodeType::Type::TAG_ENCLOSED:
                case NodeType::Type::TAG_FREE:
                    tagTypes[type->symbol] = move(type);
                break;
                case NodeType::Type::OPERATOR:
                    operatorTypes[type->symbol] = move(type);
                break;
                default:
                    assert(false);
                break;
            }

        }

        const TagNodeType* getTagType(string symbol) const {
            auto it = tagTypes.find(symbol);
            if (it == tagTypes.end())
                return nullptr;
            return static_cast<TagNodeType*>(it->second.get());
        }
        const OperatorNodeType* getOperatorType(string symbol) const {
            auto it = operatorTypes.find(symbol);
            if (it == operatorTypes.end())
                return nullptr;
            return static_cast<OperatorNodeType*>(it->second.get());
        }

        void render(const Parser::Node& ast, Variable& store, void (*)(const char* chunk, size_t size, void* data), void* data);
        void optimize(Parser::Node& ast, Variable& store);

        string render(const Parser::Node& ast, Variable& store) {
            string accumulator;
            render(ast, store, +[](const char* chunk, size_t size, void* data){
                string* accumulator = (string*)data;
                accumulator->append(chunk, size);
            }, &accumulator);
            return accumulator;
        }
    };
}

#endif
