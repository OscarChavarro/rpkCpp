#include "io/context/TransformScopeContext.h"

TransformScopeContext::TransformScopeContext():
    transformContext(nullptr),
    argumentList(nullptr),
    argumentCount(0),
    iterateArgument{'-', 'i', '\0'}
{
}

TransformScopeContext::~TransformScopeContext() {
    clearArguments();
}

void
TransformScopeContext::clearArguments() {
    delete[] argumentList;
    argumentList = nullptr;
    argumentCount = 0;
}

int
TransformScopeContext::argumentCountFor(const TransformStackContext *context) const {
    return context == nullptr ? 0 : context->xac;
}

int
TransformScopeContext::argumentStartIndexFor(const TransformStackContext *context) const {
    return argumentCount - argumentCountFor(context);
}

char **
TransformScopeContext::argumentVectorFor(const TransformStackContext *context) const {
    return &argumentList[argumentStartIndexFor(context)];
}

bool
TransformScopeContext::compactTo(const TransformStackContext *context) {
    const int contextArgumentCount = argumentCountFor(context);
    char **newArgumentList = nullptr;

    if ( contextArgumentCount > 0 ) {
        newArgumentList = new char *[contextArgumentCount + 1];
        if ( newArgumentList == nullptr ) {
            return false;
        }

        const int sourceStartIndex = argumentCount - contextArgumentCount;
        for ( int i = 0; i < contextArgumentCount; i++ ) {
            newArgumentList[i] = argumentList[sourceStartIndex + i];
        }
        newArgumentList[contextArgumentCount] = nullptr;
    }

    delete[] argumentList;
    argumentList = newArgumentList;
    argumentCount = contextArgumentCount;
    return true;
}

void
TransformScopeContext::freeTransformContext(TransformStackContext *context) const {
    if ( context == nullptr ) {
        return;
    }
    if ( context->ownedArgumentCopies != nullptr ) {
        for ( int i = 0; i < context->ownedArgumentCount; i++ ) {
            delete[] context->ownedArgumentCopies[i];
        }
        delete[] context->ownedArgumentCopies;
    }
    if ( context->transformationArray != nullptr ) {
        delete context->transformationArray;
    }
    delete context;
}
