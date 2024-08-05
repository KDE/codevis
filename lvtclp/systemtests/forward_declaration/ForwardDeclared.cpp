#include "ForwardDeclared.h"
#include "ClassDefinition.h"

void ForwardDeclared::setClassDefinition(ClassDefinition *f)
{
    d_f = f;
}

void ForwardDeclared::hello()
{
    d_f->hello();
}
