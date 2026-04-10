#include "io/context/TransformScopeContext.h"

TransformScopeContext::TransformScopeContext():
    transformContext(NULL),
    argumentList(NULL),
    argumentCount(0),
    iterateArgument()
{
    iterateArgument[0] = '-';
    iterateArgument[1] = 'i';
    iterateArgument[2] = '\0';
}

TransformScopeContext::~TransformScopeContext() {
    clearArguments();
}

void
TransformScopeContext::clearArguments() {
    delete[] argumentList;
    argumentList = NULL;
    argumentCount = 0;
}

int
TransformScopeContext::argumentCountFor(const TransformStackContext *context) const {
    return context == NULL ? 0 : context->xac;
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
    char **newArgumentList = NULL;

    if ( contextArgumentCount > 0 ) {
        newArgumentList = new char *[contextArgumentCount + 1];
        if ( newArgumentList == NULL ) {
            return false;
        }

        const int sourceStartIndex = argumentCount - contextArgumentCount;
        for ( int i = 0; i < contextArgumentCount; i++ ) {
            newArgumentList[i] = argumentList[sourceStartIndex + i];
        }
        newArgumentList[contextArgumentCount] = NULL;
    }

    delete[] argumentList;
    argumentList = newArgumentList;
    argumentCount = contextArgumentCount;
    return true;
}

void
TransformScopeContext::freeTransformContext(TransformStackContext *context) const {
    if ( context == NULL ) {
        return;
    }
    if ( context->ownedArgumentCopies != NULL ) {
        for ( int i = 0; i < context->ownedArgumentCount; i++ ) {
            delete[] context->ownedArgumentCopies[i];
        }
        delete[] context->ownedArgumentCopies;
    }
    if ( context->transformationArray != NULL ) {
        delete context->transformationArray;
    }
    delete context;
}
