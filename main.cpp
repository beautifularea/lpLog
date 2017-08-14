//
//  main.cpp
//  lpLog
//
//  Created by zhTian on 2017/8/14.
//  Copyright © 2017年 zhTian. All rights reserved.
//

#include <iostream>
#include "lpLog.h"

int main(int argc, const char * argv[]) {
    // insert code here...
    INIT_LOG("logs", "zhTian", error);
    for(int i=0;i<1000;i++) {
        ERR("This is my async log lib for cxx---beautifularea.");
    }
    
    return 0;
}
