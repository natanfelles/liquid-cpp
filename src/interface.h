#ifndef INTERFACE_H
#define INTERFACE_H

#include <stdlib.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

    #define LIQUID_ERROR_MESSAGE_MAX_LENGTH 256

    enum ELiquidParserErrorType {
        LIQUID_PARSER_ERROR_TYPE_NONE,
        LIQUID_PARSER_ERROR_TYPE_UNEXPECTED_END,
        // Self-explamatory.
        LIQUID_PARSER_ERROR_TYPE_UNKNOWN_TAG,
        LIQUID_PARSER_ERROR_TYPE_UNKNOWN_OPERATOR,
        LIQUID_PARSER_ERROR_TYPE_UNKNOWN_OPERATOR_OR_QUALIFIER,
        LIQUID_PARSER_ERROR_TYPE_UNKNOWN_FILTER,
        // Weird symbol in weird place.
        LIQUID_PARSER_ERROR_TYPE_INVALID_SYMBOL,
        // Was expecting somthing else, i.e. {{ i + }}; was expecting a number there.
        LIQUID_PARSER_ERROR_TYPE_UNBALANCED_GROUP
    };
    typedef enum ELiquidParserErrorType LiquidParserErrorType;

    enum ELiquidLexerErrorType {
        LIQUID_LEXER_ERROR_TYPE_NONE,
        LIQUID_LEXER_ERROR_TYPE_UNEXPECTED_END
    };

    typedef enum ELiquidLexerErrorType LiquidLexerErrorType;

    struct SLiquidLexerError {
        LiquidLexerErrorType type;
        size_t row;
        size_t column;
        char message[LIQUID_ERROR_MESSAGE_MAX_LENGTH];
    };
    typedef struct SLiquidLexerError LiquidLexerError;

    struct SLiquidParserError {
        LiquidParserErrorType type;
        size_t row;
        size_t column;
        char message[LIQUID_ERROR_MESSAGE_MAX_LENGTH];
    };
    typedef struct SLiquidParserError LiquidParserError;



    enum ELiquidRenderErrorType {
        LIQUID_RENDER_ERROR_TYPE_NONE,
        LIQUID_RENDER_ERROR_TYPE_EXCEEDED_MEMORY,
        LIQUID_RENDER_ERROR_TYPE_EXCEEDED_TIME,
        LIQUID_RENDER_ERROR_TYPE_EXCEEDED_DEPTH
    };
    typedef enum ELiquidRenderErrorType LiquidRenderErrorType;

    struct SLiquidRenderError {
        LiquidRenderErrorType type;
        size_t row;
        size_t column;
        char message[LIQUID_ERROR_MESSAGE_MAX_LENGTH];
    };
    typedef struct SLiquidRenderError LiquidRenderError;


    enum ETagType {
        LIQUID_TAG_TYPE_ENCLOSING,
        LIQUID_TAG_TYPE_FREE
    };

    enum ELiquidOperatorArity {
        LIQUID_OPERATOR_ARITY_NONARY,
        LIQUID_OPERATOR_ARITY_UNARY,
        LIQUID_OPERATOR_ARITY_BINARY,
        LIQUID_OPERATOR_ARITY_NARY
    };

    enum ELiquidOperatorFixness {
        LIQUID_OPERATOR_FIXNESS_PREFIX,
        LIQUID_OPERATOR_FIXNESS_INFIX,
        LIQUID_OPERATOR_FIXNESS_AFFIX
    };

    struct SLiquidContext {
        void* context;
    };
    typedef struct SLiquidContext LiquidContext;
    struct SLiquidRenderer {
        void* renderer;
    };
    typedef struct SLiquidRenderer LiquidRenderer;
    struct SLiquidTemplate {
        void* ast;
    };
    typedef struct SLiquidTemplate LiquidTemplate;
    struct SLiquidNode {
        void* node;
    };
    typedef struct SLiquidNode LiquidNode;

    struct SLiquidTemplateRender {
        void* internal;
    };
    typedef struct SLiquidTemplateRender LiquidTemplateRender;

    enum ELiquidContextSettings {
        LIQUID_CONTEXT_SETTINGS_DEFAULT = 0,
        // Can do {% assign a[1] = ... %}.
        LIQUID_CONTEXT_SETTINGS_EXTENDED_ASSIGNMENT_SYNTAX = (1 << 1),
        // Can use parentheses, operators, and everything in all expressions, not just assignments.
        LIQUID_CONTEXT_SETTINGS_EXTENDED_EXPRESSION_SYNTAX = (1 << 2)
    };
    typedef enum ELiquidContextSettings LiquidContextSettings;

    enum ELiquidVariableType {
        LIQUID_VARIABLE_TYPE_NIL,
        LIQUID_VARIABLE_TYPE_FLOAT,
        LIQUID_VARIABLE_TYPE_INT,
        LIQUID_VARIABLE_TYPE_STRING,
        LIQUID_VARIABLE_TYPE_ARRAY,
        LIQUID_VARIABLE_TYPE_BOOL,
        LIQUID_VARIABLE_TYPE_DICTIONARY,
        LIQUID_VARIABLE_TYPE_OTHER
    };
    typedef enum ELiquidVariableType LiquidVariableType;

    // Convenience function to register a custom variable type.
    // Ownership model looks thusly:
    // Calling create creates a newly allocated pointer. In all cases, one of the two things must happen:
    //  1. It must be set as an array element, or a hash element.
    //  2. It must be freed with freeVariable.
    // For languages where the variables are garbage collected, like perl and ruby; freeVariable will be a no-op.
    // Whenever getArrayVariable or getDictionaryVariable are called, a pointer is given, but no allocaitons are made.
    struct SLiquidVariableResolver {
        LiquidVariableType (*getType)(void* variable);
        bool (*getBool)(void* variable, bool* target);
        bool (*getTruthy)(void* variable);
        bool (*getString)(void* variable, char* target);
        long long (*getStringLength)(void* variable);
        bool (*getInteger)(void* variable, long long* target);
        bool (*getFloat)(void* variable, double* target);
        bool (*getDictionaryVariable)(void* variable, const char* key, void** target);
        bool (*getArrayVariable)(void* variable, size_t idx, void** target);
        bool (*iterate)(void* variable, bool (*callback)(void* variable, void* data), void* data, int start, int limit, bool reverse);
        long long (*getArraySize)(void* variable);
        void* (*setDictionaryVariable)(LiquidRenderer renderer, void* variable, const char* key, void* target);
        void* (*setArrayVariable)(LiquidRenderer renderer, void* variable, size_t idx, void* target);
        void* (*createHash)(LiquidRenderer renderer);
        void* (*createArray)(LiquidRenderer renderer);
        void* (*createFloat)(LiquidRenderer renderer, double value);
        void* (*createBool)(LiquidRenderer renderer, bool value);
        void* (*createInteger)(LiquidRenderer renderer, long long value);
        void* (*createString)(LiquidRenderer renderer, const char* str);
        void* (*createPointer)(LiquidRenderer renderer, void* value);
        void* (*createNil)(LiquidRenderer renderer);
        void* (*createClone)(LiquidRenderer renderer, void* value);
        void (*freeVariable)(LiquidRenderer renderer, void* value);
        int (*compare)(void* a, void* b);
    };
    typedef struct SLiquidVariableResolver LiquidVariableResolver;

    LiquidContext liquidCreateContext(LiquidContextSettings settings);
    const char* liquidGetContextError(LiquidContext context);

    void liquidFreeContext(LiquidContext context);
    void liquidImplementStandardDialect(LiquidContext context);
    LiquidRenderer liquidCreateRenderer(LiquidContext context);
    void liquidFreeRenderer(LiquidRenderer context);

    LiquidTemplate liquidCreateTemplate(LiquidContext context, const char* buffer, size_t size, LiquidParserError* error);
    void liquidFreeTemplate(LiquidTemplate tmpl);

    LiquidTemplateRender liquidRenderTemplate(LiquidRenderer renderer, void* variableStore, LiquidTemplate tmpl, LiquidRenderError* error);
    void liquidFreeTemplateRender(LiquidTemplateRender render);

    const char* liquidTemplateRenderGetBuffer(LiquidTemplateRender render);
    size_t liquidTemplateRenderGetSize(LiquidTemplateRender render);

    const char* liquidGetError();
    void liquidClearError();
    void liquidSetError(const char* message);

    void liquidFilterGetOperand(void* targetVariable, LiquidRenderer renderer, LiquidNode filter, void* variableStore);
    void liquidGetArgument(void* targetVariable, LiquidRenderer renderer, LiquidNode node, void* variableStore, int idx);
    typedef void* (*LiquidRenderFunction)(LiquidRenderer renderer, LiquidNode node, void* variableStore);

    // Passing -1 to min/maxArguments means no min or max.
    void liquidRegisterTag(LiquidContext context, const char* symbol, enum ETagType type, int minArguments, int maxArguments, LiquidRenderFunction renderFunction);
    void liquidRegisterFilter(LiquidContext context, const char* symbol, int minArguments, int maxArguments, LiquidRenderFunction renderFunction);
    void liquidRegisterDotFilter(LiquidContext context, const char* symbol, LiquidRenderFunction renderFunction);
    void liquidRegisterOperator(LiquidContext context, const char* symbol, enum ELiquidOperatorArity airty, enum ELiquidOperatorFixness fixness, int priority, LiquidRenderFunction renderFunction);
    void liquidRegisterVariableResolver(LiquidContext context, LiquidVariableResolver resolver);

#ifdef __cplusplus
}
#endif

#endif
