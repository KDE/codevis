#ifndef PRJWHEEL_H
#define PRJWHEEL_H

namespace Codethink {
namespace prjvhc {

class Wheel{
public:
    Wheel();
    ~Wheel();

    void setBrand(char const *brandName);
private:
    char const *m_brandName;
};

}
}
#endif
