#include "ClassDefinition.h"
#include "ForwardDeclared.h"

int main()
{
    ClassDefinition cd;
    ForwardDeclared fd;
    fd.setClassDefinition(&cd);
    fd.hello();
}
