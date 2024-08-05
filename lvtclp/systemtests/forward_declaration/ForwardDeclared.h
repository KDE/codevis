#pragma once

class ClassDefinition;

class ForwardDeclared {
  public:
    void setClassDefinition(ClassDefinition *f);
    void hello();

  private:
    ClassDefinition *d_f = nullptr;
};
