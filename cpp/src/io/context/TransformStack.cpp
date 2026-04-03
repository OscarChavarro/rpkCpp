#include "io/context/TransformStack.h"

TransformStack::TransformStack():
    transformContext(nullptr),
    argumentList(nullptr),
    argumentCount(0),
    iterateArgument{'-', 'i', '\0'}
{
}

TransformStack::~TransformStack() {
    clearArguments();
}

void
TransformStack::clearArguments() {
    delete[] argumentList;
    argumentList = nullptr;
    argumentCount = 0;
}

int
TransformStack::argumentCountFor(const TransformStackContext *context) const {
    return context == nullptr ? 0 : context->xac;
}

int
TransformStack::argumentStartIndexFor(const TransformStackContext *context) const {
    return argumentCount - argumentCountFor(context);
}

char **
TransformStack::argumentVectorFor(const TransformStackContext *context) const {
    return &argumentList[argumentStartIndexFor(context)];
}

bool
TransformStack::compactTo(const TransformStackContext *context) {
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
TransformStack::freeTransformContext(TransformStackContext *context) const {
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
