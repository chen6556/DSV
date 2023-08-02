#pragma once
#include "base/Memo.hpp"


class GlobalSetting
{
private:
    Memo _setting;
    static std::unique_ptr<GlobalSetting> _instance;
    
private:
    GlobalSetting(){}

public:
    static std::unique_ptr<GlobalSetting> &get_instance();

    Memo &setting();

    void load_setting();
    
    void save_setting();
};