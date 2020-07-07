#pragma once
#include <string>
#include <vector>

template <typename T>
struct SControlText {
    T Control;
    std::string Text;
};

class BlockableControls
{
public:
    static const std::vector<SControlText<int>>& GetList();

    static int GetIndex(int control);
};

